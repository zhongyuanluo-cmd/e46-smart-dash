# 车机助手 — 当前开发状态

> 最后更新: 2026-06-20 (MFi 芯片调试暂停, 待 0.1µF电容+4.7kΩ电阻)

## 当前 Phase

**Phase 2: 车辆接口** — 🟡 **57/73 tasks 完成**
- OpenSpec change: `phase2-vehicle-interface`
- 任务清单: `openspec/changes/phase2-vehicle-interface/tasks.md`

## 已完成 (Phase 2 关键里程碑)

### MCP2515 CAN — ✅ 台上通过
- can0 UP, ERROR-ACTIVE, 500kbps, 8MHz
- 接线: SCK=23, SI=19, SO=21, CS=24, INT=16, VCC=17, GND=20
- DTBO: `mcp2515_can0.dtbo` (单CAN, vdd-supply, high_speed pinctrl)

### K-Bus TH3122.4 — ✅ 台上通过
- UART4 (ttyS4, Pin 32/33), 9600 8E1
- 独立测试: 12V供电, 收发验证通过
- 与 RK3566 联调: 发送 0xFF 成功

### 软件主体 — ✅ 全部编译通过
- 公共库: logging, config, types (VehicleData, atomic+RW lock), ring-buffer
- CAN 解码器: 12个 E46 PT-CAN ARBID
- SocketCAN: CanInterface + Mock模式 (Unix socket pair, vcan不可用)
- Daemon: epoll主循环, CAN+K-Bus+ADC集成, systemd service
- 板端 g++ 编译 + 交叉编译双验证

### Qt6 交叉编译 — ✅
- 6.8.2, GCC 10.3 aarch64, 50 .so, Mali-G52 eglfs
- PCH disabled, static-libstdc++/libgcc

### CarPlay MFi337S3959 调试 — 🔴 受阻

**接线** (已确认):
```
MFi Pin 1 (GND)  → 40-pin Pin 9
MFi Pin 2 (SDA)  → 40-pin Pin 3 + 2.2kΩ→Pin 1 (3.3V)
MFi Pin 6 (SCL)  → 40-pin Pin 5 + 2.2kΩ→Pin 1 (3.3V)
MFi Pin 7 (RST)  → 40-pin Pin 7 (GPIO111, GPIO3_B7)
MFi Pin 8 (VCC)  → 40-pin Pin 1 (3.3V)
NC (Pin 3-5)     → 悬空
```

**已排查通过的项**:
- ✅ 7-bit I2C 地址确认: RST=GND→0x10, RST=VCC→0x11 (不是 0x20/0x22!)
- ✅ SDA/SCL 上拉 (1.6kΩ: 2.2kΩ∥底板 0.9kΩ), 空闲 3.2V
- ✅ VCC 3.3V, NC 引脚悬空, 连接全通
- ✅ 排除 IMX219 DT 冲突 (imx219@10 无驱动绑定)
- ✅ I2C 总线验证: i2c-0/1/3, 其他设备正常 (0x0c, 0x2f, 0x51)

**仍缺失**:
- ❌ 0.1µF 去耦电容 (VCC↔GND, 规格书 Fig 2-2 强制)
- ❌ 4.7kΩ 下拉电阻 (RST↔GND, GPIO启动浮空导致上电时序问题)
- ❌ SCL 速率未确认 (RK3566默认100kHz, 在10-400kHz范围内, 但未验证)

**Warm reset 时序** (已验证, 规格书 Fig 3-2):
```
RST LOW >10µs → 等≥10ms (t_STARTUP) → I2C通信
```

**测试结果**: i2cdetect/i2cget/i2ctransfer/i2cdump 全 `XX`, 20次NACK重试无一次响应。
三颗芯片行为一致。疑似假片，等电容/电阻到货后最终验证。

### 已排查排除的问题
- ❌ 不是 8-bit vs 7-bit 地址问题 (已用 0x10/0x11)
- ❌ 不是 RST 悬空 (GPIO已驱动)
- ❌ 不是 I2C 上拉缺失 (已加 2.2kΩ)
- ❌ 不是 NC 引脚误接
- ❌ 不是 I2C 总线冲突 (IMX219 无驱动)
- ❌ 不是 warm reset 时序
- ❌ 不是 NACK 单次漏判 (20次全 --)

## 硬件环境

| 组件 | 状态 |
|------|------|
| Core3566 (RK3566) + eMMC 32GB | ✅ Armbian Debian 10, kernel 4.19.232 |
| CM4-IO-BASE-B V4 底板 | ✅ 已安装 |
| 7" DSI LCD (800x480) | ✅ FPC 连 DSI1 |
| CH343 USB-UART (COM3) | ✅ 1500000 baud |
| WiFi AP6256 | ✅ SSID ChinaNet-zxd, IP 192.168.1.161 |
| MCP2515+TJA1050 CAN | ✅ can0 ERROR-ACTIVE 500kbps |
| TH3122.4 K-Bus | ✅ UART4 9600 8E1, 台上测试通过 |
| MFi337S3959 CarPlay (×3) | 🔴 I2C 不响应, 待电容+电阻 |
| ADS1115 ADC | ⏳ 未到货 |

## IKE X11175 接线 (已确认, 待上车)

| 信号 | IKE Pin | 线色 |
|------|:------:|------|
| PT-CAN High | 9 | GE/RT (黄/红) |
| PT-CAN Low | 10 | GE/BR (黄/棕) |
| K-Bus | 14 | WS/RT/GE (白/红/黄) |
| KL15 (12V IGN) | 5 | GN/BL (绿/蓝) |
| GND | 1 | BR/SW (棕/黑) |

## 剩余任务

1. ⏳ **MFi 最终验证**: 等 0.1µF 电容 + 4.7kΩ 电阻到货
2. ⏳ **ADS1115 ADC**: 到货后接线+台上测试
3. ⏳ **实车集成**: CAN/K-Bus/ADC 全部上车, 接 IKE X11175

## 关键参数 (新终端 catch up)

- SSH: `linaro@192.168.1.161` (sudo 无密码)
- 串口: COM3, 1500000 baud
- WiFi: SSID "ChinaNet-zxd"
- 内核: 4.19.232, Debian 10
- RTC: NTP+hwclock 临时方案 (hwclock-sync.service)
- 交叉编译: `cmake/toolchain-aarch64.cmake`
- MFi 调试记录: `参考 - mfi芯片debug.md`
- 规格书: `参考资料文档/3530714577MFI337S3959规格书.pdf`
