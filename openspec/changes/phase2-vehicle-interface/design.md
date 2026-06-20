## Context

Phase 2 将 Core3566 开发板连接到宝马 E46 的车辆总线。当前 SPI1 (fe620000.spi) 已通过 loopback 测试验证，40-pin 排针上有明确的 MCP2515 接线位置。板上已有双 CAN 的 DTBO (`mcp2515_two_can.dtbo`)，但需要改为单 CAN 并修复 vdd-supply 和 high_speed pinctrl 问题。

硬件约束：
- 只有 1 个 MCP2515+TJA1050 模块（不是 2 个）
- MCP2515 通过 SPI1 M1 引脚连接（MOSI=Pin19, MISO=Pin21, SCLK=Pin23, CS=Pin24, INT=Pin16）
- PT-CAN 通过 IKE X11175 插头 Pin 9 (CAN_H) / Pin 10 (CAN_L) 接入
- K-Bus 通过 IKE X11175 Pin 14 接入，使用 TH3122.4 收发器
- MCP2515 模块上的 120Ω 终端电阻必须拆除（原车 PT-CAN 两端已有终端电阻）
- RK3566 内部 SARADC 通道不在 40-pin 上，需使用外部 I2C ADC (ADS1115)
- BOM 已确认：MCP2515+TJA1050 (1个)、TH3122.4 (2个+转接板)、ADC 分压电阻均已采购

软件栈：
- Linux kernel 4.19.232，MCP251x 驱动已存在（需 vdd-supply 修复）
- SocketCAN + can-utils (cansend/candump/cangen)
- C++17 + CMake 交叉编译 (GCC 10.3, aarch64)
- DBus 用于进程间通信
- Qt6 6.8.2 + QML（UI 层，Phase 3 使用本阶段的数据）

## Goals / Non-Goals

**Goals:**
- MCP2515 驱动成功 probe，can0 网络接口可用
- 在 vcan 上验证 SocketCAN 收发和 CAN 网关 daemon 核心逻辑
- 定义 E46 PT-CAN 报文结构体（基于社区资料），硬编码解码器
- K-Bus 基础帧收发框架（UART 9.6kbps 8E1）
- I2C ADS1115 电压采样
- CAN 网关 daemon 1.0：采集→解码→DBus 发布
- 公共基础库 (logging, config, ring-buffer) 落地

**Non-Goals:**
- 实车 CAN 报文精确解码（需要硬件安装后实车抓包验证）
- K-Bus 高层协议交互（如控制 CD 碟盒、读取故障码等）
- ADC 多通道（Phase 2 只做电瓶电压一路）
- DBus 安全策略（Phase 2 用 session bus，无鉴权）
- UI 集成（Phase 3+ 才使用 DBus 数据）

## Decisions

### D1: DTBO — regulator-fixed 而非引用 vcc5v0_sys

**选择**: 在 overlay 中创建 `regulator-fixed` (`mcp2515_vdd`)

**理由**: 
- 引用 `vcc5v0_sys` 需要确认 base DTB 中的 phandle 名称在 overlay 上下文中可解析，风险较高
- `regulator-fixed` 是 DT overlay 中的标准做法，更可移植
- MCP2515 模块实际由 3.3V 供电（非 5V），`vcc5v0_sys` 语义不完全匹配

**备选方案**: 引用 `vcc5v0_sys` — 更简洁但依赖 base DTB 的符号导出

### D2: CAN 报文解码 — 硬编码 + 后续 DBC

**选择**: Phase 2 硬编码 E46 已知 ARBID 和字节解析公式，之后再编写 DBC 文件

**理由**:
- 互联网上未找到权威的 E46 MS43 DBC 文件
- loopybunny.co.uk 的 CAN ID 列表基于 E84+ 平台，ID 分配与 E46 不同
- MCP2515 物理连接后可通过 `candump` 实车抓包，反向推导各 ID 含义
- 硬编码解码器结构清晰，易于后续迁移到 DBC 自动生成

**备选方案**: 先编写 DBC 再自动生成 C 代码 — 更规范但依赖未知数据

### D3: ADC — I2C ADS1115 而非内部 SARADC

**选择**: 使用外部 I2C ADS1115 模块 (16-bit, 4-channel)

**理由**:
- RK3566 SARADC (10-bit, 1.8V ref) 的模拟输入不在 40-pin 排针上
- 内部已用通道: ADC_IN0 饱和 (~1.8V), ADC_IN1 recovery 键检测
- ADS1115 16-bit 精度远优于内部 10-bit SARADC
- I2C 地址灵活（0x48-0x4B），与现有 i2c-1 设备不冲突 (EMC2301@0x2f, PCF85063@0x51)
- 可编程增益放大器(PGA)可直接适配分压后的电压范围

**备选方案**: 寻找 Core3566 SO-DIMM 上未使用的 SARADC 引脚 — 需焊接飞线，风险高

### D4: CAN 网关架构 — 单进程多路复用

**选择**: 单进程 daemon，epoll 监听 CAN socket + UART (K-Bus) + ADC timer

**理由**:
- 数据源之间互不依赖，epoll 可高效并发
- 单进程简化 DBus 注册和 VehicleData 一致性
- 所有数据写入共享的 VehicleData 结构（atomic + RW lock）

**备选方案**: 多进程（每个接口独立 daemon） — 更模块化但增加 IPC 复杂度和延迟

### D5: K-Bus UART — 先选 UART 再接硬件

**选择**: 先不指定具体 ttySx，Phase 2 中通过排查空闲 UART 确定

**理由**:
- ttyS2 (UART2) 已被 CH343 串口终端占用
- ttyS4 (UART4) 已验证回环但位置（Pin 32/33）需确认是否空闲
- 需要先确认 TH3122.4 的电气要求（12V K-Bus ↔ 3.3V UART）和接线方案

### D6: vcan 先行验证策略

**选择**: 所有 SocketCAN 用户态代码在 vcan (虚拟 CAN) 上先完成集成测试

**理由**:
- vcan 无需硬件，可立即开始开发和测试
- 可在 Windows 交叉编译后直接部署到板端验证
- 测试覆盖: cansend/cangen 模拟报文 → daemon 解码 → DBus 属性验证
- MCP2515 硬件就位后只需切换 can0 接口名

## Risks / Trade-offs

- **[MCP2515 probe 仍失败]**: 即使 DTBO 正确，也可能因 SPI 时序、中断触发沿、晶振频率等问题导致 probe 失败 → 分步排查：dmesg 日志、SPI loopback 再验证、示波器检查信号
- **[CAN ID 解码不准确]**: 硬编码的 ARBID 可能不匹配实际 E46 DME 输出 → 用 `candump -x` 抓包，对比已知的 MS43 CAN ID 社区资料逐字节验证
- **[K-Bus 12V 电平风险]**: TH3122.4 是 12V 总线收发器，接线错误可能烧毁 RK3566 UART 引脚 → 先面包板独立测试 TH3122.4，验证 12V→3.3V 电平转换后再接 RK3566
- **[终端电阻冲突]**: 忘记拆除 MCP2515 模块上的 120Ω → CAN 总线阻抗不匹配导致通信失败 → 安装前物理检查并拍照确认
- **[ADS1115 未采购]**: 当前 BOM 中未包含 ADS1115 → Phase 2 先写驱动代码（基于 Linux IIO hwmon 或 ADS1x15 内核驱动），硬件采购可并行推进

## Open Questions

1. MCP2515 模块的晶振频率是多少？(8MHz 还是 16MHz？) — 影响 DTBO 中的 `clock-frequency` 参数
2. K-Bus 使用哪个 UART？— 需要确认 40-pin 上空闲可用的 UART TX/RX 对
3. ADS1115 模块是否加入 BOM 采购清单？
