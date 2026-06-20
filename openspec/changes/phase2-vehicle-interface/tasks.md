## 1. MCP2515 DTBO 修复与预部署

- [x] 1.1 创建 `mcp2515_can0.dts` 源文件
- [x] 1.2 编译 DTBO: `dtc -@ -I dts -O dtb` → fdtdump 验证通过, 1583 bytes
- [x] 1.3 部署到板端: `/boot/overlays/mcp2515_can0.dtbo`
- [x] 1.4 更新 overlay 加载配置: dtbo-loader.sh 已注册, 旧 mcp2515_two_can 已注释
- [x] 1.5 备份本地: `config/dts/mcp2515_can0.dts` + `config/dts/mcp2515_can0.dtbo`

## 2. 公共基础库 src/common/

- [x] 2.1 创建目录结构: `src/common/include/e46/common/`, `src/common/src/`
- [x] 2.2 实现 `logging.h/cpp`: syslog + 文件日志 + stderr, 级别过滤
- [x] 2.3 实现 `config.h/cpp`: JSON 配置解析 (nlohmann/json v3.11.3)
- [x] 2.4 实现 `types.h/cpp`: VehicleData (atomic + RW lock), monotonic timestamps
- [x] 2.5 实现 `ring-buffer.h`: 无锁环形缓冲区 (SPSC, power-of-2)
- [x] 2.6 更新 `src/common/CMakeLists.txt`: 编译为静态库, 板端验证通过

## 3. CAN 报文解码器 src/can-gateway/

- [x] 3.1 实现 `CanDecoder.h/cpp`: ARBID 字典型解码器，注册/分发模式
- [x] 3.2 定义 E46 PT-CAN 报文结构: `e46_can_ids.h` — 引擎/车速/水温/电压/点火 等 12 个 ARBID
- [x] 3.3 实现各 ARBID 解码函数: RPM (÷6.4), 水温 (-48°C offset), 车速 (direct), 电压 (÷10mV)
- [x] 3.4 未知 ARBID 静默忽略: 首次 + 每 1000 次日志一次

## 4. SocketCAN 接口层

- [x] 4.1 实现 `CanInterface.h/cpp`: socket/bind/ioctl/read/write + CAN FD 兼容
- [x] 4.2 支持 CAN 接口启停: `ip link set` 方式 bringUp, O_NONBLOCK for epoll
- [x] 4.3 CAN 帧收发封装: send/recv with timeout
- [x] 4.4 can-utils 已安装 (Phase 1 已 apt install)

## 5. CAN 网关 Daemon 主程序

- [x] 5.1 实现 `main.cpp`: 命令行解析 (--vcan, --config)，daemon 初始化
- [x] 5.2 实现 epoll 主循环: CAN fd + ADC timerfd + K-Bus 占位, 1s 间隔
- [x] 5.3 实现数据流水线: CAN recv → CanDecoder.decode → VehicleData 更新
- [x] 5.4 实现 DBus 占位: publishVehicleData() 日志输出 (DBus 后续集成)
- [x] 5.5 实现信号处理: SIGTERM/SIGINT → running=false → 清理退出
- [x] 5.6 创建 `can-gateway.json` 示例配置文件
- [x] 5.7 创建 `can-gateway.service` systemd unit 文件
- [x] 5.8 更新 CMakeLists.txt: can-gateway 子目录 + 顶层启用 (板端 + 交叉编译)
- [x] 5.9 编译验证: 板端 g++ 通过 + 交叉编译 cmake+ninja 通过, 部署到板端

## 6. Mock CAN 集成测试 (vcan 不可用)

- [x] 6.1 vcan 不可用: `CONFIG_CAN_VCAN is not set`, kernel 4.19.232 无模块支持
- [x] 6.2 实现 `CanInterface::openMock()`: Unix socket pair 替代虚拟 CAN
- [x] 6.3 Daemon 新增 `--mock` 参数: 无硬件依赖的测试模式
- [x] 6.4 Mock 启动测试: daemon 初始化 → epoll 循环 → SIGTERM 优雅关闭 ✅
- [ ] 6.5 编写帧注入测试程序: 通过 mockPairFd 注入 CAN 帧 → 验证解码
- [x] 6.6 验证 graceful shutdown: `kill -TERM <pid>` → 3秒内退出 ✅

## 7. K-Bus 接口层

- [x] 7.1 实现 `KBusInterface.h/cpp`: UART 封装 (open/configure/read/write), 9600 8E1
- [x] 7.2 实现 `KBusDecoder.h/cpp`: 状态机帧解析 (src→len→dst→data→checksum, XOR=0xFF)
- [x] 7.3 实现 K-Bus 空闲检测和冲突避免逻辑: tcdrain + select timeout
- [x] 7.4 定义已知 K-Bus 设备地址: GM5/IKE/LCM/MFL/CD/TEL/DIA + MFL 按钮解码
- [x] 7.5 集成到 daemon 编译: 板端编译通过

## 8. ADC 电压监控

- [x] 8.1 实现 `AdcSampler.h/cpp`: ADS1115 I2C 读取 (ioctl I2C_SLAVE)
- [x] 8.2 实现电压换算: raw→LSB_mV→分压比→battery_mV (10kΩ+3.3kΩ)
- [x] 8.3 实现低电量告警: engine_off<11.5V, engine_on<13.0V
- [x] 8.4 集成到 CMakeLists.txt: 板端编译通过
- [x] 8.5 BOM.md 已更新: ADS1115 已加入待采购

## 9. 硬件安装 — MCP2515 CAN

- [x] 9.1 **[用户确认]** MCP2515+TJA1050 模块已就位
- [x] 9.2 终端电阻: J1 跳线帽已断开 (120Ω 未接入) ✅
- [x] 9.3 MCP2515 接线到 40-pin: SCK=23, SI=19, SO=21, CS=24, INT=16, VCC=17, GND=20
- [x] 9.4 加载 DTBO: dmesg "MCP2515 successfully initialized" ✅
- [x] 9.5 can0 上线: bitrate 500000, ERROR-ACTIVE, clock 8MHz ✅
- [x] 9.6 SPI loopback 已由 Phase 1 验证 (PIN19↔PIN21, 100/100)

## 10. 硬件安装 — K-Bus

- [x] 10.1 **[用户确认]** TH3122.4 模块面包板搭建完成
- [x] 10.2 TH3122.4 台上独立测试: 12V 供电 → UART4 收发验证通过 ✅
- [x] 10.3 确认 UART4 (ttyS4, Pin 32/33) 空闲可用
- [x] 10.4 RK3566 UART4 → TH3122.4 台上测试: 9600 8E1, 发送 0xFF 成功
- [ ] 10.5 IKE X11175 Pin 14 (K-Bus) 接线到 TH3122.4 (需上车)

## 11. 硬件安装 — ADC

- [ ] 11.1 **[用户确认]** ADS1115 模块已采购
- [ ] 11.2 ADS1115 接线到 I2C (SDA/SCL) + VCC/GND
- [ ] 11.3 验证 I2C 检测: `i2cdetect` 确认 ADS1115 地址
- [ ] 11.4 搭建分压电路: 10kΩ + 3.3kΩ，验证分压比
- [ ] 11.5 分压电路输入接 KL30 (IKE X11175 Pin 4，12V 常电)

## 12. 实车集成测试

- [ ] 12.1 **[用户确认]** 开发板供电方案已确定 (车内 12V→5V DCDC)
- [ ] 12.2 MCP2515+TJA1050 接 IKE X11175 Pin 9 (CAN_H) / Pin 10 (CAN_L)，使用双绞线
- [ ] 12.3 `candump can0` 实车抓包，验证 CAN 帧接收
- [ ] 12.4 对比抓包数据与硬编码解码器，修正 ARBID 和字节偏移
- [ ] 12.5 daemon 实车运行，DBus 验证各字段数值合理性
- [ ] 12.6 K-Bus 实车抓包，验证车身消息 (灯光/门锁/方向盘按钮)
- [ ] 12.7 ADC 实车电压验证：熄火 ~12.6V, 怠速 ~14.0V, 启动瞬间 >10V

## 13. 文档与收尾

- [x] 13.1 更正 `硬件 - 总体框架.md`: SPI0→SPI1 (ASCI art + 接口表)
- [x] 13.2 更新 `docs/pin-mapping.md`: MCP2515/K-Bus (ttyS4) 引脚占用, 双CAN→单CAN
- [x] 13.3 新增 `docs/devlog/06-can-bringup.md`: Phase 2 开发日志
- [ ] 13.4 新增 `docs/devlog/07-kbus-setup.md`: K-Bus 调试日志 (硬件就位后)
- [x] 13.5 更新 `/memories/repo/current-status.md` 为 Phase 2 状态
- [x] 13.6 归档 `openspec/changes/refine-phase1-basic-platform/` → `archive/`
