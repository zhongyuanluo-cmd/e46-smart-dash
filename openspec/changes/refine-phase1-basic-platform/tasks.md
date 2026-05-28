## 1. Armbian 烧录与系统初始配置

- [ ] 1.1 从 Luckfox Wiki 下载 Core3566 Armbian 镜像，用 balenaEtcher 烧录到 32GB SD 卡
- [ ] 1.2 插入 SD 卡到 Core3566，连接 CH343 USB-UART 到开发机，PuTTY 打开 1500000-8-N-1 串口
- [ ] 1.3 上电启动，确认 U-Boot SPL → U-Boot → Kernel → rootfs 启动流程完整，到达 login 提示符
- [ ] 1.4 运行 `armbian-config` 配置 WiFi（AP6256, 2.4G/5G）和 locale（en_US.UTF-8 + zh_CN.UTF-8）
- [ ] 1.5 `apt update && apt upgrade` 更新所有包到最新
- [ ] 1.6 安装开发必备包：`build-essential cmake git vim i2c-tools spi-tools can-utils pkg-config`
- [ ] 1.7 确认内核版本和关键驱动：`zcat /proc/config.gz | grep -E "PANFROST|ROCKCHIP|MIPI_DSI|GPIO_SYSFS|SPI_SPIDEV|I2C_CHARDEV"`
- [ ] 1.8 记录 `docs/devlog/01-armbian-bringup.md` 中遇到的任何问题及 workaround

## 2. MIPI DSI 屏幕点亮

- [ ] 2.1 将 Waveshare 7" DSI LCD FPC 排线连接到 CM4-IO-BASE-B DSI1 接口，确认方向正确
- [ ] 2.2 检查 `/boot/armbianEnv.txt` 或设备树 overlay 目录是否有 Waveshare 7" DSI 的 overlay
- [ ] 2.3 若官方提供 overlay，在 `/boot/armbianEnv.txt` 中启用；若否，手写 DTS overlay（参考 https://github.com/waveshare/7inch-DSI-LCD 和 Luckfox DTS 模板）
- [ ] 2.4 编译并部署 DTS overlay：`dtc -@ -I dts -O dtb -o /boot/overlays/xxx.dtbo xxx.dts`
- [ ] 2.5 重启，确认 DRM connector 出现：`ls /sys/class/drm/` 含 `card*-DSI-*`，`modetest -M rockchip` 列出 DSI connector
- [ ] 2.6 确认 framebuffer console 在屏幕上显示 boot log 和 login 提示符
- [ ] 2.7 运行 `modetest -M rockchip -s <connector_id>@<crtc_id>:800x480@RG16` 做 mode setting 测试
- [ ] 2.8 记录 `docs/devlog/02-dsi-display.md` 中 panel init sequence、timing 参数和遇到的问题

## 3. Qt 6 交叉编译工具链搭建

- [ ] 3.1 下载 Linaro aarch64-linux-gnu 工具链（gcc 13+），解压到 `C:\toolchains\aarch64-linux-gnu\`
- [ ] 3.2 下载 Qt 6.8.2 源码，解压到工作目录
- [ ] 3.3 在 Core3566 上安装 Qt 运行时依赖：`apt install libfontconfig1-dev libfreetype6-dev libxkbcommon-dev libgbm-dev libdrm-dev libegl1-mesa-dev libgles2-mesa-dev`
- [ ] 3.4 创建 Qt 交叉编译配置（`qt6-aarch64.cmake`）：指定 `CMAKE_TOOLCHAIN_FILE`、`QT_HOST_PATH`（宿主编译好的 Qt）、EGL/GLES 库路径
- [ ] 3.5 编译 Qt Base + Quick + QuickControls2 + EglFSDeviceIntegration：`cmake --build . --parallel 8`
- [ ] 3.6 将编译产物（`.so` 文件）scp 到 Core3566 的 `/usr/local/qt6/`
- [x] 3.7 创建 `cmake/toolchain-aarch64.cmake`，设置 `CMAKE_PREFIX_PATH` 指向 Qt 6 安装路径
- [x] 3.8 写一个 Qt eglfs Hello World：`QGuiApplication` + `QQuickView` + 红色 Rectangle，交叉编译
- [ ] 3.9 scp 二进制到 Core3566，运行 `./qt-hello -platform eglfs`，确认屏幕上显示红色方块
- [ ] 3.10 记录 `docs/devlog/03-qt6-cross-compile.md` 中工具链版本、configure 参数和 linking 问题

## 4. 外设 IO 验证

- [x] 4.1 对照 CM4-IO-BASE-B 原理图，整理 40-pin 排针的 GPIO/UART/SPI/I2C 物理引脚 → 芯片引脚映射表
- [ ] 4.2 GPIO 输出测试：选择一组空闲 GPIO，`echo N > /sys/class/gpio/export`，设置 output，逻辑分析仪验证高低电平
- [ ] 4.3 GPIO 输入测试：配置 GPIO 为 input + pull-up，短接 GND 读取变化
- [ ] 4.4 UART 回环测试：用杜邦线短接 UART2 的 TX/RX，`stty -F /dev/ttyS2 115200`，发送数据并验证回读一致性
- [ ] 4.5 UART 高速测试：确认 UART 控制台 1500000 baud 稳定工作
- [ ] 4.6 SPI 回环测试：短接 SPI0 的 MOSI/MISO，编写 spidev C 测试程序，发送 0x00-0xFF 并验证回读
- [ ] 4.7 I2C 扫描测试：确认 I2C0 总线上有上拉电阻，`i2cdetect -y 0` 扫描设备地址（前期无 I2C 设备时确认 xx 表示无设备而非总线错误，FF 表示无上拉）
- [x] 4.8 将验证脚本归档到 `scripts/verify-gpio.sh`、`scripts/verify-uart.sh`、`scripts/verify-spi.sh`、`scripts/verify-i2c.sh`
- [ ] 4.9 记录 `docs/devlog/04-io-verification.md` 中 CM4-IO-BASE-B 引脚映射和 IO 测试结果

## 5. 冷启动时间基线测量

- [ ] 5.1 运行 `systemd-analyze` 获取 firmware、loader、kernel、userspace 各阶段耗时
- [ ] 5.2 运行 `systemd-analyze blame` 列出初始化最慢的 10 个 service
- [ ] 5.3 运行 `systemd-analyze critical-chain` 找关键路径链
- [ ] 5.4 人工测量 U-Boot 时间：上电时启动秒表，记录 U-Boot 打印第一条日志的秒数
- [ ] 5.5 人工测量 kernel 到 login 时间：记录 kernel 启动到 getty login 提示符的总秒数
- [ ] 5.6 人工测量 Qt eglfs 首帧时间：启动秒表，运行 Qt eglfs test app，记录从回车到屏幕出画面的秒数
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
