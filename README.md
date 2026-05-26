# E46 智能车机 / Smart Dash

BMW E46 智能车机仪表系统 — 替代烟灰缸位，6.9" MIPI 异形屏，赛道遥测 + CAN 诊断 + 语音助手 + 无线 CarPlay。

## 硬件平台

- **主控**: Core3566 (RK3566, 4×Cortex-A55 + Mali-G52 + 0.8T NPU)
- **屏幕**: 6.9" IPS 280×1424 MIPI DSI (ER-TFT069-1)
- **总线**: PT-CAN (TH1123/SPI)、K-Bus (L9637D/UART)、ADC 电压监控
- **传感器**: GPS ATGM336H、IMU ICM-42688-P、TPMS CC1101
- **音频**: 双 MEMS → ES7210 → I2S → PCM5102A DAC
- **连接**: WiFi 无线 CarPlay / BlueZ A2DP + HFP

## 软件栈

Armbian → Qt6/QML + Mali-G52 + SocketCAN + PulseAudio + DBus

## 文档导航

| 文档 | 说明 |
|------|------|
| [开发流程](0%20开发流程.md) | 9 个 Phase 开发计划 |
| [硬件总体框架](硬件%20-%20总体框架.md) | 系统架构、IO 分配 |
| [软件总体框架](软件与SDK%20-%20总体框架.md) | 进程架构、中间件设计 |
| [硬件配置](2%20硬件配置.md) | 芯片选型细节 |
| [BOM](BOM.md) | 物料清单 |
| [屏幕](硬件%20-%20屏幕.md) | 屏幕参数与驱动 |
| [散热](硬件%20-%20散热管理.md) | 热管理方案 |
| [UI框架](系统与UI%20-%20UI框架.md) | Qt6 vs LVGL 对比 |
| [赛道模式](系统与UI%20-%20赛道模式%20Track%20mode.md) | 弯道检测、圈速归因算法 |
| [语音交互](系统与UI%20-%20语音交互.md) | NPU ASR + NLU + TTS |

## License

MIT
