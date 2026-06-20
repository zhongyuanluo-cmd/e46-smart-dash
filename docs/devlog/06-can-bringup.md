# Devlog 06 — CAN Bus Bringup (Phase 2)

> 日期: 2026-05-30

## 概述

Phase 2 车辆接口开发启动。MCP2515 模块尚未物理安装，已完成的全部工作均为离线可推进部分。

## DTBO 适配

- 从 `mcp2515_two_can.dtbo`（双 CAN）改为 `mcp2515_can0.dtbo`（单 CAN）
- 晶振频率: 8 MHz（从 16 MHz 修正，模块丝印 1551k18 确认）
- 添加 `vdd-supply` regulator-fixed (3.3V)，修复驱动 probe 依赖
- 添加 `pinctrl-1 = &spi1m1_pins_hs`（high_speed 状态修复，与 SPIDEV loopback 一致）
- 引脚: MOSI=Pin19, MISO=Pin21, SCLK=Pin23, CS=Pin24, INT=Pin16
- DTBO 已部署到 `/boot/overlays/`，当前设为 `off`（硬件未安装）
- 旧 `mcp2515_two_can` 已从 `dtbo-loader.sh` 注释

## 公共基础库 (src/common)

- logging: syslog + 文件日志 + stderr，级别过滤
- config: nlohmann/json 解析，点分隔键导航
- types: VehicleData 原子结构体 (RPM/Speed/Coolant/Voltage)，monotonic 时间戳
- ring_buffer: 无锁 SPSC 环形缓冲 (power-of-2 容量)

## CAN 解码器 (src/can-gateway)

- CanDecoder: ARBID 字典注册/分发模式
- 12 个已知 E46 PT-CAN ARBID (基于 loopybunny.co.uk 社区数据)
- 解码函数: RPM (÷6.4), 水温 (-48°C), 车速, 电压 (0.1V→mV)
- ⚠️ 解码公式为社区最佳推测，需实车 `candump` 后验证修正

## SocketCAN 接口

- CanInterface: PF_CAN socket 封装，CAN FD 兼容，O_NONBLOCK
- bringUp(): `ip link set` 方式配置 bitrate
- Mock 模式: Unix socket pair 替代物理 CAN (vcan 不可用: CONFIG_CAN_VCAN not set)

## Daemon 主程序

- GatewayDaemon: epoll 主循环 + CAN/K-Bus/ADC 三路复用
- 信号处理: SIGTERM/SIGINT 优雅关闭
- `--mock` 参数无硬件测试模式
- 配置文件: `can-gateway.json`
- systemd: `can-gateway.service`

## 编译状态

全部模块板端 g++ (aarch64, GCC 8.3) 编译通过：
- `src/common/` → libe46-common.a (logging, config, types)
- `src/can-gateway/` → e46-can-gateway (CanDecoder, CanInterface, KBusInterface, KBusDecoder, AdcSampler, GatewayDaemon)

## 下一步

待硬件安装后 (Group 9-12):
1. MCP2515+TJA1050 接线 (需拆除 120Ω 终端电阻)
2. 启用 DTBO: `dtparam=mcp2515_can0=on`
3. can0 上线，实车 candump 抓包
4. 比对解码器 ARBID，修正字节偏移

## 实车测试指南

### 换机器继续开发

整个工作区是纯本地文件，复制 `D:\My Trunk\车机助手\` 到笔记本即可。
板端编译不需要交叉工具链（我们已验证过板端 `g++ -std=c++17` 直接编译）。

### 无网络连接方案

三条后路，按可靠性排列：

| 方式 | 命令 | 适用 |
|------|------|------|
| **串口桥** | `python Scripts\serial-bridge.py COM3 1500000` | 始终可用，物理连接 |
| **板端开热点** | AP6256 AP 模式 | 短距离 WiFi |
| **网线直连** | 板端: `sudo ip addr add 192.168.100.1/24 dev eth0`<br>笔记本: 设 192.168.100.2 | 最稳定，推荐 |

**推荐**：串口桥 + 网线双通道。串口桥保底不掉线，网线传大文件。

### 实车数据验证

daemon 的 stderr 日志会直接打印解码结果：
```
[2026-05-31] [DEBUG] VehicleData: RPM=2500 SPD=60 CTMP=85 THR=32 BAT=12600mV
```

不需要 UI 界面也能实时看到数据。后续可加 QML 简易仪表盘（板端 Qt6 已就绪）。

### 接线快速参考

- MCP2515→40-pin: MOSI=19, MISO=21, SCLK=23, CS=24, INT=16, VCC=17, GND=20
- TJA1050→IKE X11175: CAN_H=Pin9(黄/红), CAN_L=Pin10(黄/棕) — 双绞线
- IKE 供电: KL30=Pin4(常电), KL15=Pin5(钥匙电), GND=Pin1
