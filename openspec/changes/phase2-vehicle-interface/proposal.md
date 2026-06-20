## Why

Phase 1（基础开发环境 + UI 框架）52/52 任务已全部完成，Core3566 开发板具备稳定的 Qt6 交叉编译环境、WiFi 持久化连接、SPI/I2C/UART/GPIO 全部验证通过。现在需要将开发板连接到宝马 E46 的车辆总线系统，为后续的仪表盘显示、驾驶数据采集、赛道模式等功能提供数据基础。

## What Changes

- **MCP2515 CAN 接口**: 通过 SPI1 连接 MCP2515+TJA1050 模块，对接宝马 E46 PT-CAN (500kbps)，通过 IKE 仪表盘 X11175 插头 Pin 9/10 接入
- **SocketCAN 集成**: 编译内核 MCP2515 驱动，创建单 CAN DTBO overlay（修复 vdd-supply、high_speed pinctrl），上线 can0 网络接口
- **PT-CAN 报文解码**: 解析引擎转速、车速、水温、油门位置等核心动力总成数据
- **K-Bus 通信**: 通过 UART + TH3122.4 收发器接入车身 K-Bus (IKE X11175 Pin 14)
- **ADC 电压监控**: 通过 I2C ADS1115 采集电瓶电压（KL30），替代不可用的内部 SARADC
- **CAN 网关 Daemon 1.0**: 统一从 CAN/K-Bus/ADC 采集数据，解码后通过 DBus 广播给 UI 和其他模块
- **公共库扩展**: `src/common/` 加入 logging、config、CAN DBC 解析、环形缓冲等基础组件
- **设计文档更正**: SPI0→SPI1 修正，EPD 屏幕引用清理

## Capabilities

### New Capabilities

- `can-interface`: MCP2515 SPI-CAN 硬件驱动、DTBO overlay、SocketCAN 网络接口
- `can-decoder`: E46 PT-CAN 报文解码，引擎转速/车速/水温/油门等数据提取
- `kbus-interface`: TH3122.4 K-Bus 收发器驱动，UART 帧收发，车身总线通信
- `adc-monitor`: I2C ADS1115 电瓶电压采样与低电量告警
- `can-gateway-daemon`: 统一数据采集、解码、DBus 发布的后台服务

### Modified Capabilities

<!-- 无现有 specs 需要修改，Phase 1 未创建 spec 级别的 artifact -->

## Impact

- **内核**: 新 DTBO `mcp2515_can0.dtbo`，替换旧的 `mcp2515_two_can.dtbo`
- **设备树**: SPI1 pinctrl + vdd-supply 修复
- **源码**: `src/can-gateway/` 实现 CAN 网关 daemon
- **公共库**: `src/common/` 扩展 logging/config/dbc/ring-buffer
- **构建**: `src/CMakeLists.txt` 启用 can-gateway 子目录
- **硬件**: MCP2515+TJA1050 模块安装（拆除 120Ω）、TH3122.4 K-Bus 电路搭建、ADS1115 ADC 模块、分压电路
- **文档**: `硬件 - 总体框架.md` SPI0→SPI1 更正，新增 `docs/devlog/06-can-bringup.md`
