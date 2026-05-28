## Why

Phase 1（基础平台）是 E46 智能车机项目的起点，现有文档只有 5 行概要，缺少可执行的步骤级分解。在购入 ~¥1,162 物料后，需要一份精确到命令行和配置文件的具体执行计划，确保 Armbian 烧录、MIPI DSI 屏幕点亮、Qt6 交叉编译和 IO 验证这四项核心任务一次通过，为后续 Phase 2-9 打下稳定基础。

## What Changes

- 新增详细的 Armbian 烧录与内核配置步骤（含设备树修改指南）
- 新增 7" DSI 开发屏的点亮流程（DRM/KMS 配置、modetest 验证）
- 新增 Qt 6.8+ 交叉编译工具链搭建与 eglfs 渲染验证
- 新增 GPIO/UART/SPI/I2C 逐接口验证脚本
- 新增冷启动时间基线测量与优化目标设定
- 确立项目代码仓库的目录结构（monorepo，CMake 构建）
- 记录 Phase 1 期间发现的硬件/驱动问题及 workaround

## Capabilities

### New Capabilities
- `dev-environment`: Armbian 镜像烧录、Rockchip BSP 内核编译、Qt6 交叉编译工具链搭建、CMake 项目骨架
- `display-pipeline`: MIPI DSI 面板初始化、DRM/KMS 模式设置、Qt6 eglfs 后端 60fps 渲染验证
- `peripheral-io`: RK3566 GPIO 控制、UART 回环测试、SPI loopback、I2C 设备扫描——逐接口验证脚本
- `boot-performance`: 冷启动时间基线测量（U-Boot → kernel → UI 首帧）、systemd-analyze 分析、优化目标设定

### Modified Capabilities
<!-- 项目首个 change，无已有 spec 需修改 -->

## Impact

- 新建代码仓库 `src/` 目录，含 `CMakeLists.txt` 项目骨架
- 设备树文件（`.dts`）归档到 `config/` 目录
- 验证脚本归档到 `scripts/` 目录
- 开发日志/问题记录归档到 `docs/devlog/`
- 需开发机（Windows）安装 aarch64 交叉编译工具链
- 需 Core3566 + CM4-IO-BASE-B + 7" DSI LCD 硬件就位
