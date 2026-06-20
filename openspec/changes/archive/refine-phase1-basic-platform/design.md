## Context

Phase 1 是 E46 智能车机项目的起点，目标是在 Core3566 (RK3566) + CM4-IO-BASE-B 底板上建立可用的开发环境。硬件平台：RK3566 四核 A55 + Mali-G52 GPU + 2GB RAM + 32GB eMMC，外接 7" DSI LCD（开发屏）和 CH343 USB-UART 调试器。

当前状态：物料已购齐，镜像未烧录，代码仓库为空。需从零搭建。

## Goals / Non-Goals

**Goals:**
- Core3566 成功启动 Armbian，通过 UART 控制台可交互
- 7" DSI LCD 正常显示（DRM framebuffer / modetest 验证）
- Qt 6.8+ 交叉编译通过，eglfs "Hello Triangle" 渲染到屏幕
- GPIO / UART / SPI / I2C 四个接口逐个验证可用
- 冷启动时间从上电到 UI 首帧被精确测量
- 项目代码仓库骨架建立（CMake + 目录结构）

**Non-Goals:**
- CAN 总线 / K-Bus（Phase 2）
- GPS / IMU / TPMS 传感器（Phase 3）
- 音频 / 蓝牙（Phase 4）
- 语音引擎（Phase 5）
- QML UI 开发（Phase 6）
- 赛道记录 / 电源管理（Phase 7）
- CarPlay（Phase 8）
- Buildroot 产品化镜像（Phase 9）
- ER-TFT069-1 异形屏适配（Phase 6）

## Decisions

### 1. OS: Armbian (Rockchip BSP 6.1 kernel)

**选择**: Armbian community image for RK3566，基于 Linux 6.1 LTS + Rockchip BSP 补丁。

**理由**:
- Luckfox 官方提供 Core3566 Armbian 镜像，开箱即用
- apt 包管理，快速安装 can-utils / i2c-tools / spi-tools / qt6 依赖
- 社区活跃，Rockchip 主线支持持续改进

**替代方案**: Buildroot（Phase 9 才切换，开发阶段太重）

### 2. UI 框架: Qt 6 + QML (eglfs)

**选择**: Qt 6.8 LTS，eglfs 平台插件，直接走 DRM/KMS。

**渲染管线**: `QML Scene Graph → Qt Quick Renderer → OpenGL ES 3.2 (Panfrost) → DRM/KMS → MIPI DSI`

**理由**:
- eglfs 无 X11/Wayland 开销，CPU 占用最低
- QML 声明式做仪表盘 UI 效率远高于 LVGL C 手写
- Mali-G52 GPU 60fps 渲染 CPU < 15%

**替代方案**: LVGL v9（开发效率低）、Flutter Embedded（生态不够成熟）

### 3. 构建系统: CMake + aarch64 交叉工具链

**选择**: CMake 3.22+，`aarch64-linux-gnu-*` 工具链，Qt 通过 CMake `find_package` 集成。

**工具链获取**: Linaro 或 Arm 官方预编译 `aarch64-linux-gnu-*` 工具链，安装在 Windows 开发机。

**理由**:
- Qt 6 官方 CMake 支持，`qt_add_qml_module` 等
- 与后续 CI/CD 兼容
- 工具链预编译免去 crosstool-ng 构建时间

### 4. 仓库结构: Monorepo

**选择**: 单仓库 `src/`，按 daemon 分层：

```
src/
├── CMakeLists.txt              # 顶层 cmake
├── cmake/
│   └── toolchain-aarch64.cmake # 交叉编译工具链
├── common/                     # 共享库 (IPC, logging, config)
├── can-gateway/                # Phase 2
├── sensor-collector/           # Phase 3
├── audio-manager/              # Phase 4
├── voice-engine/               # Phase 5
├── track-logger/               # Phase 7
├── ui-app/                     # Phase 6 (Qt/QML)
└── carplay/                    # Phase 8
config/
├── dts/                        # 设备树源文件
└── systemd/                    # systemd unit 文件
scripts/
├── verify-gpio.sh
├── verify-uart.sh
├── verify-spi.sh
├── verify-i2c.sh
└── measure-boot-time.sh
docs/
└── devlog/                     # 开发日志
```

**理由**: 所有 daemon 共享 IPC 协议和公共代码，单仓库避免版本同步问题。

### 5. 交叉编译策略: 宿主编译 + scp 部署

**选择**: Windows 开发机上用 aarch64 工具链编译 → scp 二进制到 Core3566。

**理由**: Core3566 2GB RAM 编译 Qt 不够；eMMC 写入次数宝贵。

## Risks / Trade-offs

| 风险 | 严重度 | 缓解措施 |
|------|:---:|------|
| RK3566 MIPI DSI 2-lane 与 7" 屏不兼容（屏可能是 4-lane） | 🟡 中 | 先用 4-lane 模式点亮，后续 Phase 6 时验证 ER-TFT069-1 的 2-lane 兼容性 |
| Panfrost GPU 驱动不稳定导致 eglfs 渲染异常 | 🟡 中 | 回退到 Lima 驱动或软件渲染（llvmpipe）作为 fallback |
| aarch64 工具链在 Windows 上的路径/权限问题 | 🟢 低 | 使用 WSL2 内的 Linux 工具链作为备选方案 |
| eMMC 反复烧录损耗 | 🟢 低 | SD 卡启动开发，eMMC 仅用于最终部署 |
| 7" 屏分辨率 (800×480) 与目标屏 (280×1424) 差异大 | 🟢 低 | 开发屏仅验证渲染管线，Phase 6 再做布局适配 |

## Open Questions

1. **Luckfox Core3566 Armbian 镜像**是否已包含 Panfrost GPU 驱动？需确认内核 config 中 `CONFIG_DRM_PANFROST=y`。（烧录后第一时间检查）
2. **Qt 6.8 eglfs** 在 Rockchip 平台上的 Mali-G52 兼容性——是否需要特定 EGL/GLES 库版本？（Phase 1 验证的核心问题）
3. **CM4-IO-BASE-B 底板的 40-pin 排针引脚映射**——SPI0/SPI1/UART1-4/I2C0 的实际物理位置？（需对照底板原理图）
4. **7" DSI LCD 的初始化命令序列**——Waveshare 是否提供设备树 overlay？（是的话直接复用，否则需自己写）
