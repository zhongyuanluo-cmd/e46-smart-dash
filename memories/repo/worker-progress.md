# Worker Progress Log

## 2026-05-31: src/common/ 审查修复

审查文档: `docs/review/01-common-library.md` (由 Monitor Agent 出具)

### 修复清单

| ID | 级别 | 模块 | 判断 | 操作 |
|----|------|------|------|------|
| L1 | 🔴 | logging | ✅ 合理 | 已修复: `log()` 加 `static std::mutex` + `lock_guard` |
| L2 | 🟡 | logging | ✅ 合理 | 已修复: 新增 `log(LogLevel, fmt, va_list)` 重载，`debug/info/warn/error` 直接传 `va_list`，消除双重格式化 |
| L3 | 🟡 | logging | ⚠️ L1 自动覆盖 | L1 的 mutex 已解决 stderr 原子性问题，无需单独处理 |
| L4 | 🟢 | logging | ✅ 合理（顺手修） | 已修复: `init()` 加 `else { useSyslog_ = false; }` |
| L5 | 🟢 | logging | ❌ 误报 | `levelString()` 原本已有 `default: return "????"`，无需修改 |
| C1 | 🟡 | config | ✅ 合理 | 已修复: `navigate()` 加注释说明不支持数组索引 |
| C2 | 🟢 | config | ✅ 合理（顺手修） | 已修复: `load()` 加注释说明 Logger 需先初始化 |
| T1 | 🔴 | types | ✅ 合理 | 已修复: 加 4 个 `static_assert` 确保 `atomic<T>` lock-free |
| T2 | 🔴 | types | ✅ 合理 | 已修复: 删除 `rw_mutex` 和 `#include <shared_mutex>` |
| T3 | 🟡 | types | ✅ 合理 | 已修复: `speed_kmh` 改为 `uint16_t`（0-65535，覆盖赛道模式） |
| R1 | 🟡 | ring_buffer | ✅ 合理 | 已修复: 加 SPSC 文档注释 |
| R2 | 🟡 | ring_buffer | ✅ 合理 | 已修复: `size()` 加注释说明返回值是近似值 |
| R3 | 🟢 | ring_buffer | ✅ 合理（保留） | Phase 3 前瞻代码，不动 |
| B1 | 🟡 | CMakeLists | ✅ 合理 | 已修复: 删除冗余 `include/nlohmann` 目录 |

### 合理性判断汇总

- **全部采纳**: 14/15 条建议（L1, L2, L4, C1, C2, T1, T2, T3, R1, R2, B1 + R3 保留 + L3 自动覆盖）
- **误报**: 1 条 (L5 — `levelString()` 已有 default 分支)
- **有疑问**: 0 条

### 修改的文件

- `src/common/include/e46/common/types.h` — T1, T2, T3
- `src/common/include/e46/common/logging.h` — L2 (新增 va_list 重载)
- `src/common/src/logging.cpp` — L1, L2, L4
- `src/common/include/e46/common/config.h` — C1, C2
- `src/common/include/e46/common/ring_buffer.h` — R1, R2
- `src/common/CMakeLists.txt` — B1

### Monitor 审查质量评价

审查文档质量很高，问题描述准确、修复方案可行。唯一的误报 L5 是因为 `levelString()` 原本已正确实现了 default 分支。整体审查物有所值，尤其是 T2（rw_mutex 冗余）和 L2（双重格式化）是实际会出问题但不易发现的设计缺陷。

---

## 2026-05-31: CAN 路径审查修复 (src/can-gateway/)

审查文档: `docs/review/02-can-path.md` (由 Monitor Agent 出具)

### 修复清单

| ID | 级别 | 模块 | 判断 | 操作 |
|----|------|------|------|------|
| AS-1 | 🔴 | AdcSampler | ✅ 合理 | 已修复: GatewayDaemon 中集成 AdcSampler，`init()` 初始化，`processAdcTimer()` 调用 `sample()` |
| CD-1 | 🔴 | CanDecoder | ✅ 合理 | 已修复: `CAN_EFF_MASK` → `CAN_SFF_MASK`（E46 使用 11-bit 标准帧） |
| KI-1 | 🔴 | KBusInterface | ✅ 合理 | 已修复: `configureUart()` 根据 baud 参数设置 `speed_t`（支持 9600/19200/38400/57600/115200） |
| CI-1 | 🔴 | CanInterface | ✅ 合理 | 已修复: `bringUp()` 前通过 `validIfname()` 校验接口名（防命令注入） |
| CD-4 | 🟡 | CanDecoder | ✅ 合理 | 已修复: `decode()` 匹配到 handler 时设置 `data.can_connected = true` |
| KD-1 | 🟡 | KBusDecoder | ✅ 合理 | 已修复: wildcard src 从 `0x00` 改为 `0xFE`（避免与 GM5 真实地址冲突） |
| GD-2 | 🟡 | GatewayDaemon | ✅ 合理 | 已修复: 析构函数先关闭 epollFd_ 再关闭被监听 fd |
| CI-2 | 🟡 | CanInterface | ✅ 合理 | 已修复: `recv()` 中 `select()` → `poll()`（无 fd 数量限制） |
| KI-2 | 🟡 | KBusInterface | ✅ 合理 | 已修复: `recvByte()` 中 `select()` → `poll()` |
| CD-2 | 🟡 | CanDecoder | ✅ 合理 | 已修复: `engine_running` 阈值 100→200 RPM，加 TODO 注释待 candump 验证 |
| GD-3 | 🟡 | GatewayDaemon | ✅ 合理 | 已修复: `publishVehicleData()` 改为时间节流（每 200ms 最多一次） |
| GD-1 | 🟡 | GatewayDaemon | ✅ 合理 | 已修复: publish 日志间隔 100→600（约每 10 秒一条） |
| KD-2 | 🟡 | KBusDecoder | ✅ 合理 | 已修复: IDLE 状态收到无效字节时重置 `checksumCalc_` |
| CI-3 | 🟢 | CanInterface | ✅ 合理（顺手修） | 已修复: `close()` 中 `if (!ifname_.empty())` 登录日志 |
| CF-1 | 🟢 | can-gateway.json | ✅ 合理（顺手修） | 已修复: `i2c_addr` 从字符串 `"0x48"` 改为整数 `72` |
| GD-4 | 🟢 | GatewayDaemon | ✅ 合理（顺手修） | 已修复: `dbusFd_` 加 `[[maybe_unused]]` 抑制编译器警告 |
| CD-3 | 🟡 | CanDecoder | ⏸️ 保留 | `decodeTorqueBrake()` 空占位符，Phase 2 接受 |
| KD-3 | 🟢 | KBusDecoder | ⏸️ 保留 | `decodeMflButtons()` 占位符，已有日志输出 |
| AS-2 | 🟡 | AdcSampler | ⏸️ 保留 | 固定 `usleep(10000)` 在 128 SPS 下足够 |
| AS-3 | 🟢 | AdcSampler | ❌ 误报 | review 结论本身认定 TOCTOU 用局部快照是正确的 |
| KI-3 | 🟢 | KBusInterface | ✅ 合理（保留） | VTIME=5 设置合理 |
| CM-1 | 🟡 | CMakeLists | ⏸️ 保留 | `kbus_quick_test.cpp` 手动测试文件 |

### 合理性判断汇总

- **全部采纳**: 16/23 条建议
- **保留不修**: 6 条（CD-3/KD-3/AS-2 占位符合理，KI-3 正确，CM-1 手动测试）
- **误报**: 1 条 (AS-3 — review 自己结论认定正确)

### 修改的文件

- `src/can-gateway/src/CanDecoder.cpp` — CD-1, CD-2, CD-4
- `src/can-gateway/src/CanInterface.cpp` — CI-1, CI-2, CI-3
- `src/can-gateway/src/KBusInterface.cpp` — KI-1, KI-2
- `src/can-gateway/src/KBusDecoder.cpp` — KD-1, KD-2
- `src/can-gateway/include/GatewayDaemon.h` — AS-1, GD-4
- `src/can-gateway/src/GatewayDaemon.cpp` — AS-1, GD-1, GD-2, GD-3
- `src/can-gateway/can-gateway.json` — CF-1

### Monitor 审查质量评价

审查文档质量高，尤其是 AS-1（AdcSampler 未集成）是关键功能遗漏，CD-1（EFF→SFF mask）和 KI-1（硬编码波特率）是语义/逻辑错误，及时修复避免了后续排查困难。唯一误报 AS-3 是审查过程中自我纠正的。

---

## 2026-05-31: Gateway 完整流程审查修复 (src/can-gateway/)

审查文档: `docs/review/03-gateway-flow.md` (由 Monitor Agent 出具)

### 修复清单

| ID | 级别 | 问题 | 判断 | 操作 |
|----|------|------|------|------|
| GD-5 | 🔴 | init()中system()不受validIfname保护 | ✅ 合理 | 已修复: 在CanInterface中新增`isUp()`(ioctl SIOCGIFFLAGS)，init()改用canIf_->isUp()，彻底消除system()调用 |
| GD-6 | 🔴 | run()退出不关闭K-Bus/ADC | ✅ 合理 | 已修复: shutdown时`kbusIf_->close()` + `adcSampler_->shutdown()` |
| GD-8 | 🔴 | timerfd_create返回值未检查 | ✅ 合理 | 已修复: 失败时LOG_WARN + return true（ADC非致命） |
| GD-10 | 🟡 | processCanFrame drain无上限 | ✅ 合理 | 已修复: 加batch<64帧上限 |
| GD-9 | 🟡 | processAdcTimer read()未检查 | ✅ 合理 | 已修复: 加`adcTimerFd_<0`前置守卫 + read返回值检查 |
| GD-12 | 🟡 | publishCount static跨实例共享 | ✅ 合理 | 已修复: 改为成员变量`publishCount_` + `lastPublishMs_` |
| GD-7 | 🟡 | fd值分发不够健壮 | ⏸️ Phase 3+ | 多路CAN时再改data.ptr |
| GD-11 | 🟡 | kbusFd_双重关闭隐患 | ⏸️ Phase 3+ | GD-6已通过kbusIf_->close()规避了主要风险 |

### 合理性判断汇总

- **全部采纳**: 6/8 条建议
- **Phase 3+ 延后**: 2 条（GD-7 fd分发, GD-11 fd所有权）
- **误报**: 0 条

### 修改的文件

- `src/can-gateway/include/CanInterface.h` — GD-5 (isUp声明)
- `src/can-gateway/src/CanInterface.cpp` — GD-5 (isUp实现)
- `src/can-gateway/include/GatewayDaemon.h` — GD-12 (publishCount_/lastPublishMs_成员)
- `src/can-gateway/src/GatewayDaemon.cpp` — GD-5/6/8/9/10/12

### 亮点

- GD-5 通过`ioctl(SIOCGIFFLAGS)`替代`system()`不仅消除命令注入风险，还移除了对iproute2的依赖
- GD-12 publishCount→成员变量，同时消除static局部变量的隐式全局状态
- GD-6 补全了shutdown清理流程，防止UART残留数据

---

## 2026-05-31: Build/Config 审查修复

审查文档: `docs/review/04-build-config.md` (由 Monitor Agent 出具)

### 修复清单

| ID | 级别 | 问题 | 判断 | 操作 |
|----|------|------|------|------|
| JSON-1 | 🟡 | can.bitrate 未被代码使用 | ✅ 合理 | 已修复: init()从Config读取`can.bitrate`，传递给bringUp() |
| JSON-2 | 🟡 | adc.interval_sec 未被使用 | ✅ 合理 | 已修复: init()从Config读取，存储为`adcIntervalSec_`成员，setupEpoll()使用 |
| JSON-4 | 🟡 | logging.level 未被使用 | ✅ 合理 | 已修复: init()从Config读取，存储为`logLevel_`成员 |
| RC-1 | 🟡 | 未设置默认CMAKE_BUILD_TYPE | ✅ 合理 | 已修复: 根CMakeLists加`if(NOT CMAKE_BUILD_TYPE) set(Release)` |
| CGW-1 | 🟡 | 缺少显式pthread链接 | ✅ 合理 | 已修复: can-gateway CMakeLists加`pthread` |
| TC-1 | 🟡 | -static-libstdc++在CMAKE_CXX_FLAGS中冗余 | ✅ 合理 | 已修复: 提取`E46_STATIC_LINK_FLAGS`变量，从CXX_FLAGS移除，统一在LINKER_FLAGS中使用 |
| JSON-3 | 🟡 | dbus配置段未实现 | ⏸️ Phase 3 | DBus功能未到阶段，JSON保留 |
| RC-2 | 🟡 | Phase注释可能过期 | ⏸️ 低优先 | 无需修改代码 |
| TC-2 | 🟡 | 绝对路径硬编码 | ⏸️ 低优先 | 单人开发可接受 |
| UI-1 | 🟡 | ui-app有源码但CMakeLists是占位符 | ⏸️ Phase 6 | 届时完整构建 |
| TC-4 | 🟢 | -lrt冗余 | ⏸️ 保留 | glibc≥2.17已不需要但无害 |

### 合理性判断汇总

- **全部采纳**: 6/11 条
- **Phase延后/保留**: 5 条
- **误报**: 0 条

### 修改的文件

- `src/can-gateway/include/GatewayDaemon.h` — JSON-1/2/4 (新增配置成员)
- `src/can-gateway/src/GatewayDaemon.cpp` — JSON-1/2/4 (Config读取+应用)
- `src/CMakeLists.txt` — RC-1 (默认Release)
- `src/can-gateway/CMakeLists.txt` — CGW-1 (显式pthread)
- `cmake/toolchain-aarch64.cmake` — TC-1 (flag去重)

### 亮点

- JSON-1/2/4 一次性对齐了所有 Config 字段，消除3处硬编码
- TC-1 将4处重复的`-static-libstdc++ -static-libgcc`提取为变量，降低维护成本
- RC-1 防止忘记`-DCMAKE_BUILD_TYPE=Release`导致嵌入式性能下降

---

## 2026-05-31: OpenSpec 一致性审查修复

审查文档: `docs/review/05-openspec-consistency.md` (由 Monitor Agent 出具)

### 修复清单

| ID | 级别 | 问题 | 判断 | 操作 |
|----|------|------|------|------|
| SPEC-1 | 🔴 | K-Bus 部分帧超时（50ms）未实现 | ✅ 合理 | 已修复: `KBusDecoder` 加 `lastByteTime_` + `reset()`，`feedByte()` 入口检查超时 |
| SPEC-2 | 🔴 | K-Bus 总线空闲检测未实现 | ✅ 合理 | 已修复: `KBusInterface` 加 `waitForBusIdle()`(poll+2.1ms)，`sendByte()` 先检测再发送 |
| SPEC-3 | 🔴 | DBus 发布仅为 log 占位符 | ⏸️ Phase 3 | DBus 集成属于 Phase 3 UI 阶段，当前占位符合理 |
| SPEC-4 | 🔴 | tasks.md 重复 5.8/5.9 | ✅ 合理 | 已修复: 合并为一条，标记已完成 |
| SPEC-7 | 🟡 | tasks.md 6.6 checkbox `[ ]` 但 "✅ (已测)" | ✅ 合理 | 已修复: 改为 `[x]` |
| SPEC-5 | 🟡 | spec 写 "vdd-supply" 但用 "regulator-fixed" | ⏸️ 文档 | spec.md 文档更新，不影响代码 |
| SPEC-6 | 🟡 | spec用`can_status`，代码用`can_connected` | ⏸️ 低优先 | 命名差异不影响功能，待统一 |
| SPEC-8 | 🟡 | `openspec/specs/` 为空 | ⏸️ 归档时 | Phase 完成后通过 openspec archive 流程处理 |

### 合理性判断汇总

- **全部采纳**: 4/8 条
- **Phase 3 延后**: 1 条（SPEC-3 DBus）
- **文档/归档延后**: 3 条（SPEC-5/6/8）

### 修改的文件

- `src/can-gateway/include/KBusDecoder.h` — SPEC-1 (lastByteTime_ + reset())
- `src/can-gateway/src/KBusDecoder.cpp` — SPEC-1 (timeout check + reset)
- `src/can-gateway/include/KBusInterface.h` — SPEC-2 (waitForBusIdle)
- `src/can-gateway/src/KBusInterface.cpp` — SPEC-2 (bus idle detection)
- `openspec/changes/phase2-vehicle-interface/tasks.md` — SPEC-4 + SPEC-7

### 关键决策：SPEC-3 (DBus)

Monitor 建议讨论是否推迟 DBus。我的判断：**同意推迟到 Phase 3**。
理由：DBus 集成需要 UI 层确定数据消费模式后才能正确定义接口。当前 log 占位符足以验证 CAN 解码正确性，Phase 3 一起实现 DBus 推送+UI 订阅更合理。
---

## 2026-05-31: 文档准确性审查修复

审查文档: `docs/review/06-document-accuracy.md` (由 Monitor Agent 出具)

### SPI0→SPI1 查证

Monitor 指出的 DOC-2（SPI0→SPI1错误）经三重验证确认：
- `pin-mapping.md` 40-pin 表: Pin 19/21/23 = **SPI1_M1**，SPI0 引脚不在排针上
- `mcp2515_can0.dts`: `target = <&spi1>;`, pinctrl `spi1m1_pins`
- Phase 1 SPI1 loopback 已验证: 100/100 bytes ✅
- **结论: 正确是 SPI1，DOC-2 已修正。不会导致硬件受损。**

### 修复清单

| ID | 级别 | 文件 | 操作 |
|----|------|------|------|
| DOC-1 | 🔴 | reference-index.md | 已修复: 4处源码状态更新（common/can-gw 完成，CMake 启用） |
| DOC-2 | 🔴 | 外部实体接线一览.md | 已修复: SPI0→SPI1，并标注具体引脚 |
| DOC-3 | 🔴 | pin-mapping.md | 已修复: MCP2515 状态更新（已安装,单CAN,can0上线） |
| DOC-4 | 🔴 | pin-mapping.md | 已修复: UART 分配表重写（UART2→CH343, UART4→K-Bus） |
| DOC-5 | 🔴 | reference-index.md | 已修复: 新增 devlog/06-can-bringup.md |
| DOC-6 | 🟡 | 0 开发流程.md | 已修复: SPI 状态更新（SPI1 loopback 100/100 ✅） |
| DOC-11 | 🟡 | pin-mapping.md | 已修复: Phase 2 待办列表删除（4项全部完成） |
| DOC-12 | 🟡 | 外部实体接线一览.md | 已修复: K-Bus UART4 (ttyS4, Pin 32/33) |
| DOC-9 | 🟡 | reference-index.md | 已修复: DTBO 双CAN→单CAN+旧版归档 |

### 延后/未修

| ID | 原因 |
|----|------|
| DOC-7 | ADS1115 采购状态需用户确认 |
| DOC-8 | BOM GPS 序号，不影响功能 |
| DOC-10 | devlog/07 需新建文件，可后续补充 |
| DOC-13 | README 屏幕描述，Phase 6 再更新 |

### 修改的文件

- `硬件 - 外部实体接线一览.md` — DOC-2, DOC-12
- `docs/pin-mapping.md` — DOC-3, DOC-4, DOC-11
- `docs/reference-index.md` — DOC-1, DOC-5, DOC-9
- `0 开发流程.md` — DOC-6