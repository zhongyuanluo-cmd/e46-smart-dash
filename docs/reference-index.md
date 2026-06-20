# 项目资料清单

> 最后更新: 2026-06-09
> **硬件相关查询请优先查阅本文件，再查对应源文件，避免盲目上网搜索。**
> 新增外部资料后请更新本文件，保持索引完整。

---

## 零、外部参考资料 (URL)

| 资源 | URL | 用途 |
|------|-----|------|
| Raspberry Pi 40-pin 定义 | https://pinout.xyz/ | 底板 40-pin GPIO/SPI/I2C/UART 引脚快速查询 |
| Luckfox Core3566 Wiki | https://wiki.luckfox.com/Core3566 | 官方文档、固件下载 |
| Waveshare CM4-IO-BASE-B Wiki | https://www.waveshare.com/wiki/CM4-IO-BASE-B | 底板规格 |
| BMW E46 IKE 接口 | https://www.bmwgm5.com/E46_IKE_Connections.htm | X11175/X11176 引脚定义 |
| BMW K-CAN ID 参考 | https://www.loopybunny.co.uk/CarPC/k_can.html | CAN ID 列表（E84+平台，E46 部分兼容） |
| BMW E46 K-Bus 破解 | https://curious.ninja/project/bmw-e46/e46-k-bus/ | K-Bus Arduino 接口教程 |
| Firefly Linux 5.10 SDK | https://github.com/Firefly-rk-linux/manifests.git (branch: master, -m rk356x_linux5.10_bsp_release.xml) | 未来内核升级（PCF85063a 驱动） |

---

## 一、数据手册 / 硬件规格 (PDF)

| 文件 | 路径 | 用途 |
|------|------|------|
| Core3566 Datasheet | `参考资料文档/Core3566-datasheet.pdf` | Core3566 模块规格、40-pin 映射、SO-DIMM 引脚定义 |
| RK3566 数据手册 | `参考资料文档/Rockchip-RK3566_V1.1.pdf` | RK3566 SoC 寄存器、SARADC、引脚功能 |
| CM4-IO-BASE-B 原理图 | `参考资料文档/CM4-IO-BASE-B_V4_SchDoc.pdf` | 底板原理图、电路连接 |
| CM4-IO-BASE-B 布局图 | `参考资料文档/CM4-IO-BASE-B_page1.png` | 底板布局 |
| 7inch DSI LCD 规格 | `参考资料文档/7inch-DSI-LCD.pdf` | Phase 1 开发屏 |
| 7inch DSI Touch 规格 | `参考资料文档/7inch-DSI-TOUCH-DS.pdf` | 触摸屏 |
| ER-TFT069-1 规格 | `参考资料文档/ER-TFT069-1_Datasheet.pdf` | Phase 6 目标屏 (6.9" IPS, 280×1424) |
| CH343 数据手册 | `参考资料文档/CH343DS1.PDF` | USB-UART 芯片 |
| CH343 模块原理图 | `参考资料文档/CH343_USB_UART_Board_sch.pdf` | CH343 模块 |
| Core3566 3D 模型 | `参考资料文档/core3566_3D.zip` | 结构设计 |

---

## 二、项目设计文档 (Markdown)

### 硬件

| 文件 | 路径 | 关键内容 |
|------|------|------|
| 总体框架 | `硬件 - 总体框架.md` | 系统架构图、芯片选型总览、接口规划（⚠ SPI0→SPI1 待修正） |
| 芯片选型 | `硬件 - 不同阶段的芯片选型.md` | 各 Phase 芯片方案 |
| 外部接线 | `硬件 - 外部实体接线一览.md` | 全部物理连接汇总 |
| GPS 定位 | `硬件 - GPS定位.md` | GPS 天线、走线 |
| RTC 备份供电 | `硬件 - RTC备份供电.md` | 超级电容方案 |
| 屏幕 | `硬件 - 屏幕.md` | 屏幕选型和驱动 |
| 稳压 | `硬件 - 稳压.md` | 电源方案 |
| 散热管理 | `硬件 - 散热管理.md` | 导热方案 |
| 工作温度审计 | `硬件 - 工作温度审计.md` | 车规温度要求 |
| 蓝牙通话 HFP | `硬件 - 蓝牙通话（HFP）.md` | HFP 方案 |
| 胎压监控 | `硬件 - 胎压监控.md` | TPMS |
| 松下滚轮编码器 | `硬件 - 松下滚轮编码器.md` | 中控旋钮 |
| Board Monitor 音频 | `硬件 - Board Monitor车型音频传输.md` | BM54 音频桥 |

### 软件与 SDK

| 文件 | 路径 | 关键内容 |
|------|------|------|
| 总体框架 | `软件与SDK - 总体框架.md` | 软件架构 |
| 技术选型 | `软件与SDK - 技术选型.md` | 编程语言、框架选择 |
| 操作系统 | `软件与SDK - 操作系统.md` | Armbian/Buildroot 方案 |
| 中间件设计 | `软件与SDK - 中间件设计.md` | CAN/K-Bus 中间件 |

### 系统与 UI

| 文件 | 路径 | 关键内容 |
|------|------|------|
| UI 框架 | `系统与UI - UI框架.md` | Qt6/QML 方案 |
| 屏幕布局 | `系统与UI - 屏幕布局.md` | UI 布局设计 |
| 滚轮交互 | `系统与UI - 滚轮交互.md` | 滚轮操作逻辑 |
| 语音交互 | `系统与UI - 语音交互.md` | ASR/TTS |
| 赛道模式 | `系统与UI - 赛道模式 Track mode.md` | Track mode 功能 |
| 软件功能模块 | `系统与UI - 软件功能模块.md` | 功能列表 |
| 输入方式 | `系统与UI - 输入方式.md` | 输入方式 |
| 冷启动 | `系统与UI - 冷启动快速开机.md` | 开机优化 |
| 内存占用 | `系统与UI - 内存占用预估.md` | RAM 预算 |

### 宝马 E46 参考

| 文件 | 路径 | 关键内容 |
|------|------|------|
| IKE 接口定义 | `参考 - 宝马 IKE 接口定义.md` | X11175/X11176 引脚表，PT-CAN Pin 9/10，K-Bus Pin 14，KL15 Pin 5，KL30 Pin 4 |
| K/I-Bus 定义 | `参考 - 宝马K、I-Bus 定义.md` | K-Bus 协议参考链接 |
| Business CD | `参考 - 宝马 Business CD.md` | CD 机接口 |
| 后备箱 CD 碟盒 | `参考 - 宝马后备箱CD碟盒.md` | CD 碟盒协议 |
| 中控按钮模块 | `参考 - 宝马 中控按钮模块.md` | MFL 方向盘按钮 |
| IHKA 接口 | `归档/参考 - 宝马 IHKA 接口定义.md` | 空调面板 |
| IKE 液晶屏 | `归档/参考 - 宝马 IKE 液晶屏显示.md` | 仪表盘 LCD |
| OBD 接口 | `归档/参考 - 宝马 OBD 接口定义.md` | OBD-II 引脚 |
| WDS 总览 | `归档/参考 - 宝马 WDS 总览.md` | BMW 维修系统 |

### 开发板 / 模块参考

| 文件 | 路径 | 关键内容 |
|------|------|------|
| Core-3566 | `参考 - Core-3566.md` | Luckfox Core3566 资料链接 |
| CM4-IO-BASE-B | `参考 - CM4-IO-BASE-B 底板.md` | Waveshare 底板参考 |
| CH343 UART | `参考 - CH343 USB UART Board.md` | USB-UART 模块 |
| 7inch DSI LCD | `参考 - 7inch DSI LCD 屏幕.md` | 开发屏参考 |
| GPS 天线 | `参考 - GPS天线和阻抗控制.md` | GPS 天线设计 |
| 麦克风 | `参考 - 麦克风走线和抗干扰.md` | 麦克风布线 |
| 旧版硬件配置 | `参考 - 旧版硬件配置.md` | 历史配置 |

### 归档

| 文件 | 路径 | 说明 |
|------|------|------|
| 语音识别 ASR | `归档/参考 - 语音识别 ASR.md` | 已淘汰方案 |
| 语音生成 TTS | `归档/参考 - 语音生成 TTS.md` | 已淘汰方案 |
| 语音拼接实现 | `归档/技术 - 语音拼接实现.md` | 已淘汰方案 |

---

## 三、开发产物

| 文件/目录 | 路径 | 说明 |
|------|------|------|
| BOM 物料清单 | `BOM.md` | 全部物料采购状态 |
| 引脚映射表 | `docs/pin-mapping.md` | 40-pin 完整 GPIO 映射、SPI1 引脚、MCP2515 规划 |
| 开发日志 01 | `docs/devlog/01-armbian-bringup.md` | Armbian 烧录 |
| 开发日志 02 | `docs/devlog/02-dsi-display.md` | DSI 屏幕点亮 |
| 开发日志 03 | `docs/devlog/03-qt6-cross-compile.md` | Qt6 交叉编译 |
| 开发日志 04 | `docs/devlog/04-io-verification.md` | GPIO/SPI/I2C/UART 验证 |
| 开发日志 05 | `docs/devlog/05-wifi-autoconnect.md` | WiFi 持久化 |
| 开发日志 06 | `docs/devlog/06-can-bringup.md` | CAN 离线开发与 bringup |
| 冷启动基线 | `docs/devlog/boot-baseline.md` | 启动耗时分析 |
| DTBO: SPIDEV M1 HS | `config/dts/spidev-spi1-m1-hs.dts` | SPI1 high_speed 修复版源码 |
| DTBO: SPIDEV M1 | `config/dts/spidev-spi1-m1.dts` | SPI1 原始版 |
| DTBO: MCP2515 单 CAN | `config/dts/mcp2515_can0.dts` | 单CAN overlay (已部署, can0上线) |
| DTBO: MCP2515 双 CAN (旧) | `config/dts/mcp2515_two_can.dtbo` | 旧版双CAN overlay (已弃用, 归档) |
| 工具链 CMake | `cmake/toolchain-aarch64.cmake` | 交叉编译配置 |
| 串口桥 v2 | `Scripts/serial-bridge.py` | Python 串口桥 |
| 命令发送 | `Scripts/send-cmd.py` | 辅助命令脚本 |
| GPIO 验证 | `Scripts/verify-gpio.sh` | GPIO 测试 |
| SPI 验证 | `Scripts/verify-spi.c` | SPI 回环测试 C 源码 |
| I2C 验证 | `Scripts/verify-i2c.sh` | I2C 扫描 |
| UART 验证 | `Scripts/verify-uart.sh` | UART 回环测试 |
| DTBO 解析 | `Scripts/parse-dtbo.py` | DTBO 解码工具 |
| WiFi 配置 | `scripts/wifi-connect.sh` | WiFi 连接脚本 |
| systemd WiFi | `systemd/wifi-autoconnect.*` | WiFi 自启动服务 |
| NTP-RTC 脚本 | `Scripts/setup-ntp-rtc.sh` | RTC 同步 |
| Qt 时序 | `Scripts/qt-timing.sh` | Qt 启动耗时 |

---

## 四、源码 (src/)

| 目录 | 路径 | 状态 |
|------|------|------|
| 公共库 | `src/common/` | ✅ Phase 2 完成 (logging/config/types/ring_buffer) |
| CAN 网关 | `src/can-gateway/` | ✅ Phase 2 完成 (7源文件, SocketCAN+K-Bus+ADC) |
| 传感器采集 | `src/sensor-collector/` | 待实施 (Phase 3) |
| 音频管理 | `src/audio-manager/` | 待实施 (Phase 4) |
| 语音引擎 | `src/voice-engine/` | 待实施 (Phase 5) |
| UI 应用 | `src/ui-app/` | 待实施 (Phase 6) |
| 赛道记录 | `src/track-logger/` | 待实施 (Phase 7) |
| CarPlay | `src/carplay/` | 待实施 (Phase 8) |
| CarPlay 实现方案调研 | `参考 - carplay实现.md` | 开源协议栈、iAP2 详解、自研评估 |
| 顶层 CMake | `src/CMakeLists.txt` | common + can-gateway 启用 |

---

## 五、Memory 文件 (/memories/repo/)

| 文件 | 用途 |
|------|------|
| `current-status.md` | 当前开发状态摘要、Phase 进度、阻塞项 |
| `serial-bridge-tips.md` | 串口桥使用技巧、接线教训 |
| `workflow-rule.md` | 工作流规范 (硬件确认原则、OpenSpec 流程) |

---

## 六、OpenSpec 变更

| 变更 | 路径 | 状态 |
|------|------|------|
| Phase 1 基础平台 | `openspec/changes/refine-phase1-basic-platform/` | ✅ 52/52 完成 |
| Phase 2 车辆接口 | `openspec/changes/phase2-vehicle-interface/` | 🟡 提案完成，待实施 |

---

## 🔍 快速查阅指南

| 想查什么 | 先看哪个 |
|------|------|
| 板端当前状态、阻塞项 | `/memories/repo/current-status.md` |
| 40-pin 引脚映射 | `docs/pin-mapping.md` |
| Core3566 模块引脚 (SO-DIMM) | `参考资料文档/Core3566-datasheet.pdf` |
| RK3566 SoC 寄存器/功能 | `参考资料文档/Rockchip-RK3566_V1.1.pdf` |
| 底板电路 | `参考资料文档/CM4-IO-BASE-B_V4_SchDoc.pdf` |
| 宝马 E46 线路定义 | `参考 - 宝马 IKE 接口定义.md` |
| 宝马 K-Bus 协议 | `参考 - 宝马K、I-Bus 定义.md` |
| 物料采购状态 | `BOM.md` |
| 软件架构 | `软件与SDK - 总体框架.md` |
| 系统架构 | `硬件 - 总体框架.md` |
| 开发日志 | `docs/devlog/0*.md` |
| Workflow 规范 | `/memories/repo/workflow-rule.md` |
