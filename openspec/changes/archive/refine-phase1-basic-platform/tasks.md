## 1. Armbian 烧录与系统初始配置

- [x] 1.1 从 Luckfox Wiki 下载 Core3566 Armbian 镜像，用 RKDevTool 烧录到 eMMC
- [x] 1.2 插入 SD 卡到 Core3566，连接 CH343 USB-UART 到开发机，PuTTY 打开 1500000-8-N-1 串口
- [x] 1.3 上电启动，确认 U-Boot SPL → U-Boot → Kernel → rootfs 启动流程完整，到达 login 提示符
- [x] 1.4 配置 WiFi：安装 IPEX 天线后，AP6256 2.4GHz 可正常连接（wpa_supplicant + dhclient），无需 armbian-config
- [x] 1.5 `apt update` 更新所有包索引 ✅（WiFi 已通，时间已修正）
- [x] 1.6 安装开发必备包：build-essential(12.6), cmake(3.13.4), git, vim, i2c-tools(4.1), spi-tools(0.8.1), can-utils(2018.02), pkg-config ✅
- [x] 1.7 确认内核版本和关键驱动：`zcat /proc/config.gz | grep -E "PANFROST|ROCKCHIP|MIPI_DSI|GPIO_SYSFS|SPI_SPIDEV|I2C_CHARDEV"`
- [x] 1.8 记录 `docs/devlog/01-armbian-bringup.md` 中遇到的任何问题及 workaround

## 2. MIPI DSI 屏幕点亮

- [x] 2.1 将 Waveshare 7" DSI LCD FPC 排线连接到 CM4-IO-BASE-B DSI1 接口，确认方向正确
- [x] 2.2 检查设备树 overlay 是否生效——DSI1 镜像已预配 `card0-DSI-1`
- [x] 2.3 DSI1 镜像已预配 overlay，无需手动配置
- [x] 2.4 DSI1 镜像已内置 dtbo，无需手动编译
- [x] 2.5 重启，确认 DRM connector 出现：`ls /sys/class/drm/` 含 `card0-DSI-1`，status=connected
- [x] 2.6 确认 framebuffer console 在屏幕上显示 boot log 和 login 提示符
- [x] 2.7 运行 `modetest -M rockchip` 验证 DRM pipeline：DSI-1 connector 状态 connected，800x480@RG16 ✅
- [x] 2.8 记录 `docs/devlog/02-dsi-display.md` 中 panel init sequence、timing 参数和遇到的问题

## 3. Qt 6 交叉编译工具链搭建

- [x] 3.1 下载 Arm GNU Toolchain 10.3-2021.07 (mingw-w64-i686, aarch64-none-linux-gnu)，解压到 `C:\toolchains\gcc-arm-10.3-2021.07-mingw-w64-i686-aarch64-none-linux-gnu\`
- [x] 3.2 下载 Qt 6.8.2 源码，解压到 `C:\toolchains\qt6-src\qt-everywhere-src-6.8.2\`
- [x] 3.3 在 Core3566 上安装 Qt 运行时依赖：`apt install libfontconfig1-dev libfreetype6-dev libxkbcommon-dev libgbm-dev libdrm-dev libegl1-mesa-dev libgles2-mesa-dev`；同步 sysroot 到 Windows
- [x] 3.4 创建 Qt 交叉编译配置（`toolchain-aarch64.cmake`）：指定 `CMAKE_TOOLCHAIN_FILE`、`QT_HOST_PATH`、sysroot、EGL/GLES 库路径；修复 multiarch 扁平化、c++config.h 宏、CMAKE_FIND_ROOT_PATH
- [x] 3.5 编译 qtbase(1052步)+qtshadertools(80步)+qtdeclarative(3062步)，共 4194 步，0 失败；关键：PCH 禁用、static-libstdc++/libgcc
- [x] 3.6 将 50 个 .so + 8 个插件目录 + 7 个 QML 目录 scp 到 Core3566 的 `/usr/local/lib|plugins|qml/`；安装 Mali-G52 驱动
- [x] 3.7 创建 `cmake/toolchain-aarch64.cmake`，设置 `CMAKE_PREFIX_PATH` 指向 Qt 6 安装路径
- [x] 3.8 写一个 Qt eglfs Hello World：`QGuiApplication` + `QQmlApplicationEngine` + 内联 QML，交叉编译成功
- [x] 3.9 scp 二进制到 Core3566，运行 `QT_QPA_PLATFORM=eglfs ./qt6-test`，Mali-G52 OpenGL ES 3.2 硬件加速渲染 ✅
- [x] 3.10 记录 `docs/devlog/03-qt6-cross-compile.md`：工具链版本、configure 参数、7 个坑和 workaround

## 4. 外设 IO 验证

- [x] 4.1 对照 CM4-IO-BASE-B 原理图，整理 40-pin 排针的 GPIO/UART/SPI/I2C 物理引脚 → 芯片引脚映射表
- [x] 4.2 GPIO 输出测试：GPIO104 output，Saleae 捕获 4 次清晰电平跳变 ✅
- [x] 4.3 GPIO 输入测试：GPIO103 input，短接 3V3 读 1、悬空读 0 ✅
- [x] 4.4 UART 回环测试：UART4 (ttyS4) PIN32↔PIN33 短接，115200+921600 baud 回环 ✅
- [x] 4.5 UART 高速测试：ttyS2 serial bridge 日常 1500000 baud 稳定运行 ✅
- [x] 4.6 SPI 回环测试：SPI1 (spidev1.0) PIN19↔PIN21 短接，100/100 字节回环 ✅（修复 pinctrl high_speed M0→M1）
- [x] 4.7 I2C 三总线扫描完成 ✅：i2c-0=0x0c,0x20; i2c-1=EMC2301@0x2f, PCF85063@0x51, 未知@0x0c,0x38,0x45; i2c-3=空
- [x] 4.8 将验证脚本归档到 `scripts/verify-gpio.sh`、`scripts/verify-uart.sh`、`scripts/verify-spi.sh`、`scripts/verify-i2c.sh`
- [x] 4.9 记录 `docs/devlog/04-io-verification.md`：CM4-IO-BASE-B 引脚映射 + I2C 三总线扫描结果 + 内核驱动编译状态

## 5. 冷启动时间基线测量

- [x] 5.1 运行 `systemd-analyze` 获取 firmware、loader、kernel、userspace 各阶段耗时
- [x] 5.2 运行 `systemd-analyze blame` 列出初始化最慢的 10 个 service
- [x] 5.3 运行 `systemd-analyze critical-chain` 找关键路径链
- [x] 5.4 人工测量 U-Boot 时间：上电时启动秒表，记录 U-Boot 打印第一条日志的秒数 ⚠️ 无串口，用估算值 ~1.8s
- [x] 5.5 人工测量 kernel 到 login 时间：记录 kernel 启动到 getty login 提示符的总秒数
- [x] 5.6 人工测量 Qt eglfs 首帧时间：启动秒表，运行 Qt eglfs test app，记录从回车到屏幕出画面的秒数
- [x] 5.7 将上述数据填入 `docs/devlog/boot-baseline.md`
- [x] 5.8 设定 Buildroot 冷启动目标：U-Boot ≤ 1.5s / Kernel ≤ 2.5s / Userspace+UI ≤ 4s / Total ≤ 8s
- [x] 5.9 列出当前 Armbian 镜像中可以在 Buildroot 裁掉的服务/驱动清单（初步裁剪建议）

## 6. 项目仓库骨架搭建

- [x] 6.1 在 `src/` 下创建 `CMakeLists.txt` 顶层文件，设置 `cmake_minimum_required(VERSION 3.22)`、C++17、project 名称
- [x] 6.2 创建 `cmake/toolchain-aarch64.cmake`，指定交叉编译器路径、sysroot、Qt6 路径
- [x] 6.3 创建 `src/common/CMakeLists.txt` 空骨架（Phase 2 起填充日志/配置/IPC 共享代码）
- [x] 6.4 创建各 daemon 子目录空骨架：`src/can-gateway/`、`src/sensor-collector/`、`src/audio-manager/`、`src/voice-engine/`、`src/track-logger/`、`src/ui-app/`、`src/carplay/`
- [x] 6.5 在顶层 CMake 中添加 `add_subdirectory` 引用各 daemon（Phase 1 只有 ui-app 有实际代码）
- [x] 6.6 创建 `config/dts/` 和 `config/systemd/` 目录
- [x] 6.7 创建 `.gitignore`（排除 build/、.vscode/ 除外、*.o、.DS_Store）
- [x] 6.8 `git init && git add -A && git commit -m "init: project skeleton with CMake build system"`
