# 审查报告：OpenSpec 规范一致性

> 审查日期: 2026-05-31 | 审查人: Copilot | 范围: openspec/ vs src/ 代码实现

---

## 整体评价：⭐⭐⭐ (3/5)

规范与代码之间存在 **3 个功能缺口**和若干文档不一致。核心问题是 K-Bus 的两个 spec 需求未在代码中实现（空闲检测、帧超时），以及 DBus 发布仍是占位符。

---

## 1. Spec vs Code 一致性逐项对比

### 1.1 can-interface (spec.md vs CanInterface.cpp)

| Spec Requirement | Code Status | 判定 |
|-----------------|-------------|------|
| DTBO 编译成功、单 CAN | ✅ 已完成，mcp2515_can0.dtbo 部署 | ✅ 一致 |
| SocketCAN 网络接口 | ✅ CanInterface::open/bringUp/recv/send | ✅ 一致 |
| SPI1 引脚映射 | ✅ Pin 19/21/23/24/16 正确 | ✅ 一致 |
| High-speed pinctrl | ✅ spi1m1_pins_hs 已加入 DT | ✅ 一致 |

### 🟡 SPEC-5 — spec 写 "vdd-supply" 但设计决策选了 "regulator-fixed"

**现象**: spec.md 要求 "including a vdd-supply reference"，但 `design.md` D1 明确选择了 `regulator-fixed` 方案。代码实现也是 `regulator-fixed`。

**建议**: 更新 spec.md 将 "vdd-supply" 改为 "regulator-fixed"，与 design 决策和代码实现保持一致。

---

### 1.2 can-decoder (spec.md vs CanDecoder.cpp)

| Spec Requirement | Code Status | 判定 |
|-----------------|-------------|------|
| VehicleData 结构体（atomic） | ✅ types.h 中实现 | ✅ 一致 |
| 解码 RPM/车速/水温/油门 | ✅ 已实现 | ✅ 一致 |
| 未知 ARBID 静默忽略 | ✅ 节流日志 | ✅ 一致 |
| 注册/分发模式 | ✅ handler map | ✅ 一致 |
| 时间戳记录 | ✅ mark_can_update() | ✅ 一致 |

✅ can-decoder 规范与代码完全一致。

---

### 1.3 can-gateway-daemon (spec.md vs GatewayDaemon.cpp)

| Spec Requirement | Code Status | 判定 |
|-----------------|-------------|------|
| Epoll 多路复用 | ✅ 已实现 | ✅ 一致 |
| 信号处理 SIGTERM/SIGINT | ✅ 已实现 | ✅ 一致 |
| JSON 配置读取 | ✅ Config 类 | ✅ 一致 |
| vcan 模式 | ✅ --vcan 参数 | ✅ 一致 |
| Systemd service | ✅ can-gateway.service 已创建 | ✅ 一致 |

### 🔴 SPEC-3 — DBus 数据发布：spec 要求完整 DBus，代码只是 log 占位符

**现象**:

Spec 要求：
```
publish decoded vehicle data on the session DBus at interface com.e46.can1.VehicleData
- DBus property query returns RPM as uint32
- PropertiesChanged signal on value change
```

代码实现：
```cpp
void GatewayDaemon::publishVehicleData()
{
    // Placeholder: DBus publishing (Tasks 5.3-5.4)
    E46_LOG_DEBUG("VehicleData: RPM=%u SPD=%u ...", ...);
}
```

**考量**: DBus 集成是 Phase 3 的工作（UI 层使用 DBus 数据），所以在 Phase 2 作为占位符是合理的。但 spec 将其列为 Phase 2 的 ADDED Requirement，不匹配当前实现状态。

**建议**: 
- 方案 A（推荐）: 在 spec 中将 DBus 相关 requirement 标注为 "Phase 3" 或拆分为独立的 spec
- 方案 B: 实现最小 DBus 发布（只注册接口名 + 属性，不做信号推送）

### 🟡 SPEC-6 — spec 说 `can_status`，代码是 `can_connected`

**现象**: spec.md 写道 "VehicleData.can_status SHALL indicate 'disconnected'"，但代码中字段名是 `std::atomic<bool> can_connected`，类型是 bool 而非 status 枚举。

**建议**: 统一命名。推荐改为 spec 中的 `can_status`（未来可扩展为 enum: disconnected/error-active/error-passive/bus-off），或保持 `can_connected` 并更新 spec。

---

### 1.4 kbus-interface (spec.md vs KBusInterface.cpp / KBusDecoder.cpp)

| Spec Requirement | Code Status | 判定 |
|-----------------|-------------|------|
| UART 9600 8E1 | ✅ 已实现 | ✅ 一致 |
| 帧解析 (src→len→dst→data→checksum) | ✅ 状态机实现 | ✅ 一致 |
| XOR 校验 (结果为 0xFF) | ✅ 已实现 | ✅ 一致 |
| 设备地址表 | ✅ GM5/IKE/LCM/MFL 等定义 | ✅ 一致 |

### 🔴 SPEC-1 — K-Bus 部分帧超时（50ms）未实现

**Spec 要求**:
```
Partial frame timeout:
WHEN the start of a K-Bus frame is received but remaining bytes do not arrive within 50ms
THEN the system SHALL discard the partial frame and reset to idle state
```

**代码现状**: `KBusDecoder::feedByte()` 状态机没有超时机制。状态机进入 IDLE→SRC→LEN→DST→DATA→CHECKSUM 后，如果总线中断不再发送后续字节，解码器会**永久停留在当前状态**，后续正确帧也会被误解析。

**建议**: 在 `feedByte()` 或 `GatewayDaemon::run()` 中加入超时逻辑：
```cpp
// 在每次 feedByte 调用前检查
if (state_ != State::IDLE) {
    auto now = std::chrono::steady_clock::now();
    if (now - lastByteTime_ > std::chrono::milliseconds(50)) {
        reset();  // 丢弃部分帧，回到 IDLE
        E46_LOG_WARN("K-Bus partial frame timeout");
    }
}
lastByteTime_ = std::chrono::steady_clock::now();
```

---

### 🔴 SPEC-2 — K-Bus 总线空闲检测/冲突避免未实现

**Spec 要求**:
```
Bus idle detection:
WHEN the system needs to transmit a K-Bus message
THEN it SHALL wait for bus idle (no bytes received for >2.1ms at 9600 baud)
THEN it SHALL then transmit the complete frame
```

**代码现状**: `KBusInterface::sendByte()` 直接写 UART，没有检查总线状态。虽然 K-Bus 是低速总线（9600 baud）且当前以被动监听为主，但一旦开始主动发送（如请求 IKE 数据），缺少冲突检测会导致总线冲突。

**建议**: 
```cpp
bool KBusInterface::sendByte(uint8_t byte, int timeoutMs)
{
    // Wait for bus idle (no RX activity for >2.1ms)
    if (!waitForBusIdle(/* 2100μs */)) return false;
    // ... existing write logic
}
```

---

### 1.5 adc-monitor (spec.md vs AdcSampler.cpp)

| Spec Requirement | Code Status | 判定 |
|-----------------|-------------|------|
| ADS1115 I2C 读取 | ✅ 已实现 | ✅ 一致 |
| 分压电路 10kΩ+3.3kΩ | ✅ 已实现 | ✅ 一致 |
| 周期性采样（1Hz） | ✅ timerfd 1s | ✅ 一致 |
| 低电量告警 11.5V/13.0V | ✅ BATTERY_LOW_* 常量 | ✅ 一致 |
| battery_low / charging_fault 标志 | ✅ 已实现 | ✅ 一致 |

✅ adc-monitor 规范与代码完全一致。

---

## 2. tasks.md 审查

### 2.1 🔴 SPEC-4 — 重复任务 ID：5.8 和 5.9 各出现两次

```
第 48-49 行: [x] 5.8 更新 CMakeLists.txt: can-gateway 子目录 + 顶层启用
             [x] 5.9 板端编译验证: g++ 链接通过, 生成 e46-can-gateway
第 50-51 行: [ ] 5.8 更新 src/can-gateway/CMakeLists.txt 和顶层 src/CMakeLists.txt
             [ ] 5.9 交叉编译验证: cmake + ninja，部署到板端
```

**分析**: 第 48-49 行是板端原生编译（已做），第 50-51 行是交叉编译（Windows→aarch64）。实际上 CMakeLists.txt 已经更新过了（前几轮审查已验证），交叉编译也应该已完成。这两个 unchecked 项应该合并或标记完成。

**建议**: 
- 合并 5.8: 删除重复项，或重命名为 5.8a/5.8b 区分板端和交叉编译
- 合并 5.9: 同上。如果交叉编译已验证，标记 `[x]`

---

### 2.2 🟡 SPEC-7 — 任务 6.6 标记不一致

```
[ ] 6.6 验证 graceful shutdown: kill -TERM <pid> → 3秒内退出 ✅ (已测)
```

checkbox 是 `[ ]` 但描述说 "✅ (已测)"。根据 6.4 的验证结果（"Mock 启动测试: ... SIGTERM 优雅关闭 ✅"），graceful shutdown 已通过测试。

**建议**: 改为 `[x]`。

---

### 2.3 任务完成率统计

| Section | Total | Done | Pending | Completion |
|---------|-------|------|---------|------------|
| 1. MCP2515 DTBO | 5 | 5 | 0 | 100% |
| 2. 公共基础库 | 6 | 6 | 0 | 100% |
| 3. CAN 解码器 | 4 | 4 | 0 | 100% |
| 4. SocketCAN | 4 | 4 | 0 | 100% |
| 5. Daemon 主程序 | 9→11* | 7 | 2→4* | ~64% |
| 6. Mock 测试 | 6 | 4 | 2 | 67% |
| 7. K-Bus | 5 | 5 | 0 | 100% |
| 8. ADC | 5 | 5 | 0 | 100% |
| 9. MCP2515 硬件 | 6 | 6 | 0 | 100% |
| 10. K-Bus 硬件 | 5 | 4 | 1 | 80% |
| 11. ADC 硬件 | 5 | 0 | 5 | 0% |
| 12. 实车集成 | 2 | 0 | 2 | 0% |

> \* 含重复 ID

---

## 3. openspec 目录结构

```
openspec/
├── changes/
│   ├── phase2-vehicle-interface/     ← 活跃 change
│   │   ├── .openspec.yaml
│   │   ├── proposal.md
│   │   ├── design.md
│   │   ├── tasks.md
│   │   └── specs/                    ← 5 个 capability spec
│   └── archive/
│       └── refine-phase1-basic-platform/  ← Phase 1 已完成
└── specs/                            ← ⚠️ 空目录！
```

### 🟡 SPEC-8 — `openspec/specs/` 为空

Phase 1 已归档但未将 spec 提升到 `openspec/specs/`。OpenSpec 标准流程是：change 归档后，其 spec 应合并到顶层 `specs/` 目录。

**建议**: Phase 1 归档时执行 `openspec archive` 流程，将 refine-phase1-basic-platform 的 spec 提升到 `openspec/specs/`。Phase 2 完成后同理。

---

## 📊 汇总

| ID | 级别 | 问题 | 说明 |
|----|------|------|------|
| SPEC-1 | 🔴 | K-Bus 部分帧超时未实现 | 状态机可能永久卡住 |
| SPEC-2 | 🔴 | K-Bus 空闲检测未实现 | 主动发送会冲突 |
| SPEC-3 | 🔴 | DBus 发布仅为 log 占位符 | spec 要求完整 DBus，但留到 Phase 3 |
| SPEC-4 | 🔴 | tasks.md 重复任务 5.8/5.9 | 两个 unchecked 项应是 done 或合并 |
| SPEC-5 | 🟡 | spec 写 "vdd-supply" 但用 "regulator-fixed" | 文档落后设计决策 |
| SPEC-6 | 🟡 | spec 用 `can_status`，代码用 `can_connected` | 字段名不一致 |
| SPEC-7 | 🟡 | tasks.md 6.6 标记 `[ ]` 但描述 "✅ (已测)" | checkbox 与状态矛盾 |
| SPEC-8 | 🟡 | `openspec/specs/` 为空 | Phase 1 归档时未提升 spec |

### 修复优先级

1. **立即修复 (🔴)**:
   - **SPEC-1**: K-Bus 帧超时 → `KBusDecoder` 加 50ms 超时重置
   - **SPEC-2**: K-Bus 空闲检测 → `KBusInterface` 加 `waitForBusIdle()`
   - **SPEC-4**: tasks.md 合并重复 5.8/5.9，将已完成项标记为 `[x]`

2. **本 Phase (🟡)**:
   - SPEC-5: spec.md 更新 vdd-supply → regulator-fixed
   - SPEC-6: 统一 can_connected ↔ can_status 命名
   - SPEC-7: 修正 6.6 checkbox

3. **Phase 3 (🟡)**:
   - SPEC-3: DBus 集成（已在 Phase 3 规划中）
   - SPEC-8: `openspec/specs/` 目录维护
