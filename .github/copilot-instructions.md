# 车机助手项目 — 工作区规则

## 硬件资料查询规则（最高优先级）

**涉及硬件问题时（引脚、接线、规格书、芯片能力、驱动问题），必须按以下顺序：**
1. 先查 `docs/reference-index.md` 找到对应资料路径
2. 阅读本地文件（PDF、Markdown、原理图），不要跳过直接上网搜
3. 仅当本地资料确实没有对应信息时，才通过网络搜索

适用范围：GPIO 引脚、SPI/I2C/UART、SARADC、芯片规格、板子布局、宝马 E46 线束、接插件引脚。

## 资料索引

- 项目资料清单: `docs/reference-index.md`（所有文档、PDF、URL 索引）
- 当前状态: `/memories/repo/current-status.md`
- 工作流规范: `/memories/repo/workflow-rule.md`
- 串口桥技巧: `/memories/repo/serial-bridge-tips.md`

## 项目关键参数

- **核心板**: Core3566 (Luckfox), RK3566 SoC, CM4 SO-DIMM 封装
- **底板**: Waveshare CM4-IO-BASE-B V4
- **系统**: Armbian Debian 10, kernel 4.19.232
- **40-pin**: 兼容树莓派引脚, 参考: https://pinout.xyz/
- **SSH**: linaro@192.168.1.161 (sudo 无密码)
- **串口**: COM3, 1500000 baud
- **WiFi**: AP6256, SSID "ChinaNet-zxd", wpa_supplicant systemd 持久化
- **Qt6**: 6.8.2 交叉编译 (GCC 10.3, aarch64), 50 个 .so, Mali-G52 eglfs
- **交叉编译**: `cmake/toolchain-aarch64.cmake`, PCH 禁用, 静态 libstdc++/libgcc
- **当前阶段**: Phase 2 (车辆接口), Phase 1 已全部完成 (52/52)

## 硬件交互规则

1. **先问再做**: 不要假设硬件已连接。任何物理接线、测试、修改前先问用户。
2. **遵守 Phase 边界**: 不提前推进未到阶段的硬件工作。
3. **引脚建议**: 必须引用 `docs/pin-mapping.md` 或官方文档，禁止凭记忆。

## CarPlay 方案红线

**绝对不使用 Carlinkit 商业模块。** CarPlay 实现走纯软件/裸 MFi 芯片方案。禁止提及 Carlinkit。

## 代码规范

- C++17, CMake 3.22+
- 交叉编译用 `cmake/toolchain-aarch64.cmake`
- 文件编辑用 `replace_string_in_file`（保留 3-5 行上下文）
- 任务追踪: `tasks.md` 中的 checkbox
- OpenSpec 流程: `/opsx:propose` → proposal → design → specs → tasks → `/opsx:apply`
