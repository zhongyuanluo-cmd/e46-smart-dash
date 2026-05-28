
| 层级 | 选型 | 理由 |
|------|------|------|
| **OS (开发)** | Armbian (Debian 12) | 微雪官方支持，apt 快速开发 |
| **OS (产品)** | Buildroot | 镜像 ~150MB，启动 5s |
| **UI 框架** | Qt 6 + QML (eglfs) | Mali-G52 GPU 加速，车载仪表开发效率高 |
| **CAN 协议栈** | SocketCAN + 自研解码 | Linux 原生，can-utils 调试方便 |
| **K-Bus** | 自研 UART 收发 | 9600bps 简单协议，自研即可 |
| **音频** | PulseAudio + BlueZ | A2DP/AVRCP/HFP 全覆盖，混音成熟 |
| **降噪** | RNNoise | 开源，引擎噪声抑制极好 |
| **ASR** | NPU(RKNN) + CPU 解码 | 唤醒词+命令词，NPU 不占CPU |
| **NLU** | 规则 + 轻量分类器 | 车内固定领域，无需LLM |
| **TTS** | Piper TTS | 免费，音质好，15% CPU |
| **GPS/IMU** | C/C++ daemon | 直接读写 UART/SPI，最轻量 |
| **TPMS** | CC1101 (315MHz) + BLE scan | 双模兼容 |
| **赛道日志** | C daemon → CSV/MoTeC | 高频低延迟写入 |
| **电源管理** | systemd + GPIO | Suspend-to-idle / 自动关机 |
| **CarPlay** | Autokit SDK (可选) | 商业方案，兼容性好 |
