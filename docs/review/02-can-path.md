# 审查报告：CAN 路径代码 (src/can-gateway/)

> 审查日期: 2026-05-31 | 审查人: Copilot | 模块: src/can-gateway/

---

## 整体评价：⭐⭐⭐ (3.5/5)

CAN 路径代码整体架构合理，SocketCAN + epoll 事件循环 + 状态机解码的设计方向正确。7 个源文件覆盖了 CAN 硬件层、K-Bus 串口层、协议解码和主循环。主要问题集中在 **AdcSampler 未集成**、**CAN ID 掩码错误**、**hardcode 波特率** 等几个方面。

---

## 1. CanInterface — CAN Socket 封装 ⭐⭐⭐

### 1.1 设计概览

- 封装 Linux SocketCAN (`PF_CAN`, `SOCK_RAW`, `CAN_RAW`)
- 非阻塞模式 + `select()` 超时读取
- Mock 模式 (`socketpair`) 用于无硬件测试
- CAN FD 支持（向后兼容）

### 1.2 🔴 CI-1 — `bringUp()` 使用 `system()` 调用 ip 命令

**现象**:
```cpp
bool CanInterface::bringUp(int bitrate)
{
    std::string cmd = "ip link set " + ifname_ + " type can bitrate " +
                      std::to_string(bitrate) +
                      " && ip link set up " + ifname_;
    int ret = system(cmd.c_str());
    ...
}
```

**考量**:
1. **安全风险**: `system()` 启动 shell，如果 `ifname_` 来自用户输入且未校验，可能导致命令注入（如 `can0; rm -rf /`）
2. **依赖外部工具**: 需要 `iproute2` 包，而 `GatewayDaemon::init()` 已经有一个 fallback 检查 `ip link show ... | grep -q UP`，造成双重依赖
3. **错误处理**: `system()` 返回 shell 退出码，难以区分"ip 命令失败"和"shell 启动失败"
4. **更好的方案**: 使用 Netlink (`RTM_NEWLINK`) 或 `ioctl(SIOCSIF*...)` 来配置 CAN 接口，不依赖外部命令

**建议**: 用 Netlink `libnl` 或直接 `ioctl` 替代 `system()`。Phase 2 最低要求：至少对 `ifname_` 做输入校验（只允许字母数字和短横线）。

```cpp
// 最小防护：校验 ifname_
bool validIfname(const std::string& name) {
    return !name.empty() && name.size() < 16 &&
           name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-") == std::string::npos;
}
```

---

### 1.3 🟡 CI-2 — `recv()` 中 `select()` 的 `nfds` 参数可能越界

**现象**:
```cpp
int ret = select(fd_ + 1, &fds, nullptr, nullptr, ptv);
```

**考量**: `select()` 要求 `nfds` = 最大 fd + 1。Linux 上 fd 由内核分配，理论上可能 > 1024。如果 `fd_` 是 2000，写入 `fds` 的 `FD_SET(2000, &fds)` 会越界（`fd_set` 默认 1024 bits）。

**建议**: 两种方案：
- **方案 A**: 改用 `poll()` — 没有 fd 数量限制，且更简洁
- **方案 B**: 加 `assert(fd_ < FD_SETSIZE)` 确保编译期捕获（但这不够，因为 fd 是运行时分配的）

**推荐方案 A**（连 KBusInterface 一起改）：
```cpp
struct pollfd pfd;
pfd.fd = fd_;
pfd.events = POLLIN;
int ret = poll(&pfd, 1, timeoutMs);
```

---

### 1.4 🟢 CI-3 — `close()` 无条件记录日志

**现象**:
```cpp
void CanInterface::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    ...
    E46_LOG_INFO("CAN interface %s closed", ifname_.c_str());  // 始终输出
}
```

**考量**: 如果从未调用 `open()`，析构函数会输出 "CAN interface  closed"（`ifname_` 为空）。虽然无害但日志不够干净。

**建议**: 加 `if (!ifname_.empty())` 或改为 `E46_LOG_DEBUG`。

---

## 2. CanDecoder — CAN 帧解码器 ⭐⭐⭐⭐

### 2.1 设计概览

- 基于 ARBID 的 handler 注册表 (`unordered_map`)
- Lambda 委托到私有解码函数
- 未知 ARBID 的节流日志（第一条 + 每 1000 条）

### 2.2 🔴 CD-1 — CAN ID 掩码使用 `CAN_EFF_MASK` 而非 `CAN_SFF_MASK`

**现象**:
```cpp
auto it = handlers_.find(frame.can_id & CAN_EFF_MASK);
```

**考量**: E46 PT-CAN 使用 **标准帧（11-bit）** CAN ID，不是扩展帧（29-bit）。`CAN_EFF_MASK` = `0x1FFFFFFF`（29 bits），`CAN_SFF_MASK` = `0x7FF`（11 bits）。

虽然当前 ARBID 都 < 0x7FF，`& CAN_EFF_MASK` 不会丢失数据，但语义上是错误的。如果 CAN 总线上收到一个扩展帧（29-bit），`& CAN_EFF_MASK` 会保留完整 29 bits，与 handler 表中的 11-bit key 不匹配。

**建议**: 改为 `CAN_SFF_MASK`：
```cpp
auto it = handlers_.find(frame.can_id & CAN_SFF_MASK);
```

如果未来需要支持扩展帧，可以检测 `CAN_EFF_FLAG` 后分别处理：
```cpp
uint32_t id = frame.can_id & (frame.can_id & CAN_EFF_FLAG ? CAN_EFF_MASK : CAN_SFF_MASK);
```

---

### 2.3 🟡 CD-2 — `decodeEngineRpm()` 中 `engine_running` 阈值过低

**现象**:
```cpp
data.engine_running.store(rpm > 100, std::memory_order_relaxed);
```

**考量**: 
- 起动机带动发动机转速一般在 **200-300 RPM**（冷启动可能更低但不会低到 100）
- 阈值 100 可能将"起动机刚啮合"误判为"引擎已启动"
- 更合理的阈值是 **200-250 RPM**

但这是依赖真实 CAN 数据的微调参数，当前作为初始值合理。

**建议**: 改为 `rpm > 200`，同时确认：BMW DME 在引擎未启动时输出的是 RPM=0 还是 RPM=某个非零值？这需要 `candump` 验证。先加注释标记为待验证：
```cpp
// TODO: Verify with candump — what RPM does DME report when engine is off?
data.engine_running.store(rpm > 200, std::memory_order_relaxed);
```

---

### 2.4 🟡 CD-3 — `decodeTorqueBrake()` 是空实现占位符

**现象**: 函数签收了参数但什么都不做（已标记 placeholder）。

**考量**: ARBID `0x0A8` 已注册 handler，decode 时会调用此函数，然后调用 `mark_can_update()`。这意味着即使收到扭矩/刹车数据，也不会更新 `VehicleData`。

**建议**: 当前 Phase 2 不需要完整扭矩解码，但至少应该标记 `can_connected = true`（见 CD-4）。或者：在实现前不注册此 handler，避免静默丢弃。

---

### 2.5 🟡 CD-4 — 缺少 `can_connected` 状态更新

**现象**: `CanDecoder::decode()` 成功匹配 handler 后调用了 `mark_can_update()`，但从未设置 `data.can_connected = true`。

**考量**: `VehicleData` 中有 `std::atomic<bool> can_connected{false}`，但整个 CAN 路径代码从未将其设为 `true`。这意味着 UI 层无法判断 CAN 总线是否在线。

**建议**: 在 `decode()` 中匹配到 handler 时：
```cpp
data.can_connected.store(true, std::memory_order_relaxed);
```

同理，如果一段时间没收到 CAN 帧，应该设为 `false`。可以在 GatewayDaemon 中增加超时检测。

---

### 2.6 🟢 CD-5 — `unknownCount_` 的类型与格式化

**现象**:
```cpp
uint64_t unknownCount_ = 0;
// ...
E46_LOG_DEBUG("... (count: %llu)", static_cast<unsigned long long>(unknownCount_));
```

**考量**: aarch64 上 `uint64_t` = `unsigned long`，`%llu` 期望 `unsigned long long`。虽然 cast 后正确，但不够简洁。

**建议**: 使用 `<cinttypes>` 的 `PRIu64` 宏：
```cpp
#include <cinttypes>
E46_LOG_DEBUG("... (count: %" PRIu64 ")", unknownCount_);
```

---

## 3. KBusInterface — K-Bus 串口封装 ⭐⭐⭐

### 3.1 设计概览

- `/dev/ttyS4` UART 操作，9600 8E1
- `tcdrain()` 确保发送完成
- `select()` 超时读取单字节

### 3.2 🔴 KI-1 — `configureUart()` 硬编码 B9600，忽略 `baud` 参数

**现象**:
```cpp
bool KBusInterface::open(const std::string& device, int baud)
{
    ...
    if (!configureUart(baud)) { ... }
}

bool KBusInterface::configureUart(int baud)
{
    ...
    cfsetospeed(&tty, B9600);   // ← 硬编码
    cfsetispeed(&tty, B9600);   // ← 忽略 baud 参数
}
```

**考量**: E46 K-Bus 确实是 9600 baud，但函数签名支持其他速率（如 `open(device, 19200)`），实际却忽略参数。如果未来有其他车系需要 19200 baud，这个 bug 会很难发现。

**建议**: 根据 `baud` 参数设置正确的 `speed_t`：
```cpp
speed_t speed;
switch (baud) {
    case 9600:  speed = B9600;  break;
    case 19200: speed = B19200; break;
    case 38400: speed = B38400; break;
    default:    E46_LOG_ERROR("Unsupported baud: %d", baud); return false;
}
cfsetospeed(&tty, speed);
cfsetispeed(&tty, speed);
```

---

### 3.3 🟡 KI-2 — `recvByte()` 同样有 `select()` nfds 问题

与 CI-2 相同，改用 `poll()`。

---

### 3.4 🟢 KI-3 — `VTIME=5` 超时合理性

**现象**: `tty.c_cc[VTIME] = 5`（500ms 超时）。

**考量**: K-Bus 一个完整帧长度约 4-20 字节，9600 baud 下约 4-21ms 传输时间。500ms 远大于此，但 `VTIME` 仅在 `VMIN=0` 时生效，表示字节间超时。如果总线长时间静默，`recvByte` 内部的 `select()` 超时和 `VTIME` 可以共同工作。**当前设置合理**。

---

## 4. KBusDecoder — K-Bus 帧解码器 ⭐⭐⭐⭐

### 4.1 设计概览

- 状态机解析 K-Bus 帧：`IDLE → SRC → LEN → DST → DATA → CHECKSUM`
- XOR 校验（结果应为 0xFF）
- `deque` 帧队列（已从 vector O(n) 修复）
- 3 级 handler 匹配：精确 → 广播 → 任意源

### 4.2 🟡 KD-1 — `decode()` 中 `HandlerKey{0, dst}` 与 `KBUS_ADDR_GM5` 冲突

**现象**:
```cpp
// Try any source to this destination
key.src = 0;
key.dst = frame.dst;
it = handlers_.find(key);
```

**考量**: `0x00` 是 `KBUS_ADDR_GM5`（General Module 车身电脑）的合法地址。如果用 `src=0` 作为"匹配任意源"的 wildcard，那 `GM5 → 某地址` 和 `任意源 → 某地址` 无法区分。

**建议**: 使用一个不会被实际设备使用的值作为 wildcard（如 `0xFE`，不在设备地址表中）：
```cpp
constexpr uint8_t KBUS_SRC_ANY = 0xFE;  // Wildcard for "any source"

// decode() 中:
key.src = KBUS_SRC_ANY;
```

---

### 4.3 🟡 KD-2 — `feedByte()` 校验和计算逻辑需确认

**现象**:
```cpp
void KBusDecoder::feedByte(uint8_t byte)
{
    checksumCalc_ ^= byte;   // 每个字节都 XOR
    ...
    case State::CHECKSUM:
        currentFrame_.valid = (checksumCalc_ == 0xFF);
```

**考量**: K-Bus 标准校验：所有字节（含校验和自身）XOR 结果应为 0xFF。如果发送端计算 `checksum = 0xFF ^ src ^ len ^ dst ^ data[0] ^ ...`，接收端 XOR 所有字节（含 checksum）得 0xFF。

当前实现：`checksumCalc_` 从 0 开始，逐字节 XOR。最后判断 `== 0xFF`。逻辑正确，但有一个细节：**在校验和字节到达时**，`checksumCalc_` 已经包含了它（`case CHECKSUM` 之前执行了 `checksumCalc_ ^= byte`），然后判断 `checksumCalc_ == 0xFF`。这等价于"所有字节 XOR 应等于 0xFF"。

✅ 逻辑正确。

**但有一个潜在问题**：如果帧中途出现无效字节（IDLE 状态下 byte==0），`checksumCalc_` 已经被 XOR 了无效值，但状态机没有回到 IDLE。当前实现中 IDLE 会跳过 byte==0，但 `checksumCalc_` 已经在 switch 之前被污染。

**建议**: 在 IDLE 状态下读到无效字节时，重置 `checksumCalc_`：
```cpp
case State::IDLE:
    if (byte > 0) {
        currentFrame_ = KBusFrame{};
        currentFrame_.src = byte;
        state_ = State::LEN;
    }
    // If byte == 0: stay IDLE, but checksumCalc_ already XORed — need reset
    break;
```
改为：
```cpp
case State::IDLE:
    if (byte > 0) {
        currentFrame_ = KBusFrame{};
        currentFrame_.src = byte;
        state_ = State::LEN;
    } else {
        checksumCalc_ = 0;  // Reset — this byte wasn't part of a frame
    }
    break;
```

---

### 4.4 🟢 KD-3 — `decodeMflButtons()` 是占位符

与 CD-3 类似，可接受。但至少应记录按钮值到日志（当前已经做了），在 VehicleData 中增加按钮字段后启用。

---

## 5. AdcSampler — ADS1115 ADC 采样器 ⭐⭐⭐⭐

### 5.1 设计概览

- I2C 操作 ADS1115，读取电池电压
- 电压分压器：10kΩ / 3.3kΩ，比例 0.24812
- 低电量/充电故障检测

### 5.2 🔴 AS-1 — `AdcSampler` 已编译但未在 `GatewayDaemon` 中实例化

**现象**: 
- `CMakeLists.txt` 编译了 `AdcSampler.cpp`
- `GatewayDaemon.h` 没有 `#include "AdcSampler.h"`
- `GatewayDaemon::processAdcTimer()` 只做了 `mark_adc_update()`，**没有创建 AdcSampler 实例，没有调用 `sample()`**

```cpp
void GatewayDaemon::processAdcTimer()
{
    uint64_t expirations;
    read(adcTimerFd_, &expirations, sizeof(expirations));
    vehicleData_.mark_adc_update();   // ← 只标记时间戳，没采样！
}
```

**考量**: 这是根本性的集成遗漏。ADC 采样器代码写好了、编译了，但从未被主循环使用。`battery_voltage_mv` 永远不会被 ADC 更新，只能被 CAN 消息（`ARBID_BATTERY_VOLTAGE`）更新。

**建议**: 在 `GatewayDaemon` 中集成 AdcSampler：
1. 添加 `#include "AdcSampler.h"`
2. 添加成员 `std::unique_ptr<adc::AdcSampler> adcSampler_;`
3. 在 `init()` 中初始化（非致命失败）
4. 在 `processAdcTimer()` 中调用 `adcSampler_->sample(vehicleData_)`

---

### 5.3 🟡 AS-2 — `readRaw()` 用固定延迟替代轮询

**现象**: `usleep(10000)` 等待转换完成，而非轮询 config 寄存器的 OS 位。

**考量**: ADS1115 在 128 SPS 下转换时间约 8ms。`usleep(10000)` 固定等待，代码简单。但：
- 如果 I2C 总线繁忙导致实际转换延迟 > 10ms → 读到旧数据或卡住
- 如果将来改为 8 SPS（125ms 转换时间）→ 10ms 不够

**建议**: 当前 128 SPS 下 10ms 足够。如果后续提高精度降低 SPS，改为轮询 OS 位：
```cpp
// Poll OS bit (bit 15 of config register)
for (int retry = 0; retry < 20; ++retry) {
    uint8_t buf[2];
    // Read config reg
    if (i2c read returns bit 15 == 1) break;
    usleep(1000);
}
```

---

### 5.4 🟢 AS-3 — `sample()` 中 TOCTOU 竞态

**现象**:
```cpp
bool engineRunning = data.engine_running.load();  // 读一次
// ...
if (mv < threshold) {
    if (engineRunning) {  // 用局部变量
        data.charging_fault.store(true, ...);
    }
}
```

**考量**: `engine_running` 在 `sample()` 调用期间可能改变（CAN 线程更新），但这里使用局部变量快照。对于电压判断，用快照是正确的——判断"采样时刻"引擎状态，而非"写入时刻"状态。✅ 实现正确。

---

## 6. GatewayDaemon — 主循环和初始化 ⭐⭐⭐

### 6.1 设计概览

- 信号处理（SIGTERM/SIGINT）→ `g_running`
- 命令行参数解析 + JSON 配置文件
- Epoll 事件循环（CAN fd + K-Bus fd + timerfd）
- K-Bus 非致命失败

### 6.2 🟡 GD-1 — `publishVehicleData()` 节流日志不够精细

**现象**:
```cpp
static int publishCount = 0;
if (++publishCount <= 3 || publishCount % 100 == 0) {
    E46_LOG_DEBUG(...);
}
```

**考量**: 前 3 次 + 每 100 次。CAN 总线在发动机运行时可能每秒 100+ 帧，意味着大约每秒一条 DEBUG 日志。在 syslog 中不算多，但文件日志可能增长较快。

**建议**: 将 `% 100` 改为 `% 600`（约每 10 秒一条），或只在值变化时记录。

---

### 6.3 🟡 GD-2 — 析构函数关闭 fd 的顺序

**现象**:
```cpp
GatewayDaemon::~GatewayDaemon()
{
    running_ = false;
    if (adcTimerFd_ >= 0) close(adcTimerFd_);
    if (kbusFd_ >= 0) close(kbusFd_);
    if (epollFd_ >= 0) close(epollFd_);
}
```

**考量**: 应先关闭 epollFd_（停止事件监听），再关闭被监听的 fd。否则在关闭 epoll 前，被监听 fd 如果被内核复用，可能收到意外的 epoll 事件。

**建议**: 调整顺序：
```cpp
if (epollFd_ >= 0) { close(epollFd_); epollFd_ = -1; }
if (adcTimerFd_ >= 0) { close(adcTimerFd_); adcTimerFd_ = -1; }
if (kbusFd_ >= 0) { close(kbusFd_); kbusFd_ = -1; }
```

---

### 6.4 🟡 GD-3 — `processCanFrame()` 中的 `publishVehicleData()` 调用条件不精确

**现象**:
```cpp
void GatewayDaemon::processCanFrame()
{
    struct can_frame frame;
    while (canIf_->recv(frame, 0)) {
        decoder_->decode(frame, vehicleData_);
        if (vehicleData_.rpm.load() > 0 || vehicleData_.speed_kmh.load() > 0) {
            publishVehicleData();
        }
    }
}
```

**考量**: 每条有 RPM 或速度的 CAN 帧都会触发 `publishVehicleData()`。如果同时收到 RPM 帧和速度帧，会发布两次。在高频 CAN 流量下，`publishVehicleData()` 被调用过于频繁。

**建议**: 使用时间节流（而非条件节流）：
```cpp
static int64_t lastPublish = 0;
int64_t now = common::VehicleData::now_ms();
if (now - lastPublish > 200) {  // 每 200ms 最多发布一次
    publishVehicleData();
    lastPublish = now;
}
```

---

### 6.5 🟡 GD-4 — `dbusFd_` 占位符未使用

与 AS-1 类似，这是 Phase 2 的已知待实现项。可以保留，但建议加 `[[maybe_unused]]` 抑制编译器警告。

---

## 7. main.cpp ⭐⭐⭐⭐⭐

简单、干净，没有可审查的问题。

---

## 8. CMakeLists.txt ⭐⭐⭐

### 8.1 🟡 CM-1 — `kbus_quick_test.cpp` 未加入构建

已在之前的 `monitor-corrections.md` 中记录。这是一个手动测试文件，不阻塞功能。如果希望自动化测试，可以添加为单独的 executable。

### 8.2 🟢 CM-2 — `AdcSampler.cpp` 被编译但未被主程序使用

这是 AS-1 的派生问题。修复 AS-1 后自动解决。

---

## 9. can-gateway.json ⭐⭐⭐⭐

### 9.1 🟢 CF-1 — `i2c_addr` 是字符串而非整数

```json
"i2c_addr": "0x48"
```

`Config::get<uint8_t>("adc.i2c_addr")` 能正确解析吗？nlohmann/json 的 `get<uint8_t>()` 期望整数而非字符串 `"0x48"`。如果写 `config.get<uint8_t>("adc.i2c_addr", 0x48)` 且 JSON 是字符串，会触发 catch(...) 返回默认值 0x48。

**建议**: 改为 `"i2c_addr": 72`（十进制）或直接用整数 `0x48` 不被 JSON 标准禁止（实际是 72）。

---

## 📊 汇总

| 模块 | 🔴 必须修复 | 🟡 建议修复 | 🟢 可选 |
|------|-----------|-----------|--------|
| CanInterface | 1 (CI-1 system调用) | 1 (CI-2 select/poll) | 1 (CI-3 空日志) |
| CanDecoder | 1 (CD-1 EFF→SFF mask) | 3 (CD-2/3/4) | 1 (CD-5) |
| KBusInterface | 1 (KI-1 硬编码波特率) | 1 (KI-2 select/poll) | 1 (KI-3) |
| KBusDecoder | 0 | 2 (KD-1/2) | 1 (KD-3) |
| AdcSampler | 1 (AS-1 未集成) | 1 (AS-2 固定延迟) | 1 (AS-3) |
| GatewayDaemon | 0 | 3 (GD-1/2/3) | 1 (GD-4) |
| CMakeLists | 0 | 1 (CM-1) | 1 (CM-2) |
| config JSON | 0 | 0 | 1 (CF-1) |

### 修复优先级

1. **立即修复 (🔴)**:
   - **AS-1**: AdcSampler 集成到 GatewayDaemon（阻断了 ADC 功能）
   - **CD-1**: `CAN_EFF_MASK` → `CAN_SFF_MASK`（当前无实际 bug 但语义错误）
   - **KI-1**: `configureUart` 根据 baud 参数设置波特率
   - **CI-1**: `bringUp()` 添加输入校验（防命令注入）

2. **本 Phase 修复 (🟡)**:
   - CD-4: `can_connected` 状态更新
   - KD-1: wildcard src 避免与 GM5 冲突
   - GD-2: 析构函数 fd 关闭顺序
   - CI-2/KI-2: `select()` → `poll()`
   - CD-2: `engine_running` 阈值调高到 200
   - GD-3: `publishVehicleData()` 时间节流

3. **可选 (🟢)**: 其余
