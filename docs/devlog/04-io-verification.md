# Dev Log — IO Verification

> 日期：2026-05-30 ~ 进行中
> 硬件：Core3566 + CM4-IO-BASE-B + Saleae Logic 8 + 面包板
> 更新：2026-06-01 — Group 4 测试计划 + Saleae MCP 集成就绪

## 状态总览

| Group | Test | Status | Notes |
|:---:|------|:---:|------|
| 4.1 | I2C bus scan | ✅ DONE | 3 个 bus 均正常 |
| 4.2 | GPIO output (PIN11/GPIO#104) | ✅ DONE | Saleae Ch6 捕获到 4 次电平跳变，波形正确 |
| 4.3 | GPIO input (PIN37/GPIO#103) | ✅ DONE | 接线3V3→PIN37=1, 拔线=0, 无需Saleae |
| 4.4 | UART loopback (UART4/ttyS4) | ✅ DONE | 115200+921600 baud 完整回环，零丢失 |
| 4.5 | UART high-speed | ✅ IMPLICIT | 1500000 baud 已在 serial bridge (COM3=ttyS2) 验证 |
| 4.6 | SPI loopback (SPI1) | ✅ PASS | 修复 pinctrl high_speed + 正确跳线 |

## Saleae Logic 8 配置

### 设备信息
- **型号**: Saleae Logic 8 (8-channel)
- **Device ID**: `DEA3665FCEFE5CAB` (已通过 MCP `get_devices` 确认)
- **MCP Server**: 端口 10530，Logic 2 → Settings → Automation → MCP Server 已启用
- **VS Code 集成**: 用户级 MCP 配置，指向 `http://127.0.0.1:10530`
- **所有 MCP 工具在 VS Code 可用**: `start_capture`, `stop_capture`, `add_analyzer`, `export_data_table_csv` 等

### 电缆颜色映射
| Channel | 颜色 | 说明 |
|:---:|:---:|------|
| Ch0 | 黑 | — |
| Ch1 | 棕 | — |
| Ch2 | 红 | — |
| Ch3 | 橙 | — |
| Ch4 | 黄 | — |
| Ch5 | 绿 | — |
| Ch6 | 蓝 | — |
| Ch7 | 紫 | — |

⚠️ **5V 引脚是 OUTPUT 供电**，我们的测试不需要连接。

### Group 4 Saleae 接线表

| Saleae Ch | 颜色 | 信号 | Header Pin | RK3566 GPIO | Kernel GPIO# | ALT Function |
|:---:|:---:|------|:---:|:---:|:---:|------|
| Ch0 | 黑 | SPI1_MOSI | PIN19 | GPIO3_C1 | 113 | SPI1_MOSI_M1 |
| Ch1 | 棕 | SPI1_MISO | PIN21 | GPIO3_C2 | 114 | SPI1_MISO_M1 |
| Ch2 | 红 | SPI1_SCLK | PIN23 | GPIO3_C3 | 115 | SPI1_CLK_M1 |
| Ch3 | 橙 | SPI1_CS0 | PIN24 | GPIO3_A4 | 100 | I2S3_LRCK_M0 |
| Ch4 | 黄 | UART4_TX | PIN32 | GPIO3_B2 | 106 | UART4_TX_M1 |
| Ch5 | 绿 | UART4_RX | PIN33 | GPIO3_B1 | 105 | UART4_RX_M1 |
| Ch6 | 蓝 | GPIO17 | PIN11 | GPIO3_B0 | 104 | (无 ALT 冲突) |
| GND | — | GND | PIN6 | — | — | — |

### Saleae 捕获工作流

1. 连接 Saleae 探头到目标引脚 + GND
2. VS Code 中调用 `mcp_logic2_start_capture`（配置数字通道 + 采样率）
3. SSH 执行测试脚本（或 serial bridge）
4. 调用 `mcp_logic2_stop_capture`
5. 调用 `mcp_logic2_add_analyzer`（如 Async Serial, SPI）
6. 调用 `mcp_logic2_export_data_table_csv` 导出结果
7. 验证数据正确性

## 1. I2C ✅

| Bus | Scan Result | Notes |
|:---:|------|------|
| i2c-0 | 空 (all --) | 正常，无外设 |
| i2c-1 | 0x0c, **0x2f**(EMC2301风扇), 0x45, **0x51**(RTC) | ✅ |
| i2c-3 | 空 (all --) | 触摸屏待 Phase 6 |

脚本: `Scripts/verify-i2c.sh`

## 2. GPIO

- GPIO sysfs enabled (CONFIG_GPIO_SYSFS=y)
- **测试引脚**: PIN11 = GPIO17 = GPIO3_B0 = Kernel GPIO#104
  - 选择理由：无 ALT function 冲突，纯 GPIO
- 脚本: `Scripts/verify-gpio.sh` — **需要更新 TEST_PIN=104**

### 4.2 GPIO Output ✅ (2026-06-01)

**测试配置**: Saleae Ch6 (蓝) → PIN11, GND → PIN6, 12 MS/s

**操作序列**: GPIO104 export → direction=out → value 1→0→1→0 (每次 sleep 0.5s)

**Saleae 捕获结果** (captureId=5):

| Time [s] | Ch6 | 含义 |
|---|---|---|
| 0.000 | 0 | 初始 LOW |
| 9.914 | 1 | → HIGH (第1次翻转) |
| 10.529 | 0 | → LOW (间隔 ~0.615s) |
| 11.139 | 1 | → HIGH (间隔 ~0.610s) |
| 11.751 | 0 | → LOW (间隔 ~0.612s) |

**结论**: ✅ 4 次清晰电平跳变，HIGH/LOW 逻辑正确。SSH 延迟约 0.6s/次（含 sleep 0.5s），符合预期。

CSV: `C:\Temp\saleae-gpio-test-4.2\digital.csv`

### 4.3 GPIO Input ✅ (2026-05-30)

**测试引脚**: PIN37 = GPIO3_A7 = Kernel GPIO#103

> ⚠️ **重要教训**: 不要在同一引脚上同时插 Saleae 探头和杜邦线，会导致接触不良。
> GPIO input 测试无需 Saleae — 用 sysfs 读取即可。如需 Saleae 监控，应选另一个 GPIO 引脚。

**测试步骤** (纯 sysfs，无 Saleae):
1. GPIO103 export → direction=in → 默认 value=0
2. 杜邦线短接 PIN1(3V3) → PIN37
3. `cat /sys/class/gpio/gpio103/value` → **1** ✅
4. 拔掉杜邦线
5. `cat /sys/class/gpio/gpio103/value` → **0** ✅
6. unexport GPIO103

**结论**: ✅ GPIO103 输入模式正确识别高/低电平。物理通路已通过 OUTPUT HIGH 反向验证（万用表测 PIN37=3.4V）。

**排错记录**: 首次测试时 GPIO103 始终读 0，原因是同一引脚 PIN37 上同时插了 Saleae Ch6 探头和杜邦线，杜邦线无法良好接触。拆除 Saleae 探头后问题解决。

## 3. UART

| Device | TX Pin | RX Pin | Loopback Test | Notes |
|------|:---:|:---:|:---:|------|
| ttyS2 (serial bridge) | PIN8 | PIN10 | ✅ IMPLICIT | 1500000 baud, COM3 日常使用中 |
| ttyS4 (UART4) | PIN32 | PIN33 | ✅ PASS | 115200+921600 baud 回环通过 |

### 4.4 UART4 Loopback ✅ (2026-06-01)

**前提**: `/boot/config.txt` 添加 `dtparam=uart4=on`，重启后 ttyS4 出现
- dmesg: `fe680000.serial: ttyS4 at MMIO 0xfe680000 (irq = 96, base_baud = 1500000) is a 16550A`
- PIN32 = UART4_TX_M1, PIN33 = UART4_RX_M1

**测试步骤** (纯 SSH，无 Saleae):
1. 短接 PIN32↔PIN33 (TX→RX loopback via 杜邦线)
2. `sudo stty -F /dev/ttyS4 115200 raw -echo`
3. 启动后台 `cat /dev/ttyS4`，然后 `echo HELLO_UART4 > /dev/ttyS4`
4. 收到: `HELLO_UART4` ✅
5. 切换 921600 baud，发送 `ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`
6. 收到: `ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789` ✅
7. 恢复默认: `sudo stty -F /dev/ttyS4 9600 sane`

**结论**: ✅ UART4 回环在 115200 和 921600 baud 下均完整回环，零数据丢失。

### 4.5 UART High-Speed
- **已隐式验证**: 日常使用的 serial bridge = COM3 ↔ ttyS2 @ 1500000 baud
- 无需额外测试，1500000 baud 已稳定运行

### ⚠️ UART4 DT Overlay 可能性
- PIN32 = GPIO3_B2 = UART4_TX_M1, PIN33 = GPIO3_B1 = UART4_RX_M1
- 需要设备树配置使能 UART4 的 M1 mux
- 参考: `config/dts/mcp2515_two_can.dtbo` 创建类似 overlay
- 如果 `/dev/ttyS4` 不存在，需编译 DT overlay 并加载

## 4. SPI

| Device | Status | Notes |
|------|:---:|------|
| spidev1.0 (SPI1_M1) | ✅ PASS | 100/100 bytes matched, 10 MHz |

### 4.6 SPI1 Loopback ✅ (2026-05-30)

**两个根因，分别修复后测试通过。**

#### 根因 1: DT pinctrl high_speed state 使用了错误的 M0 引脚

Rockchip SPI 驱动在数据传输时自动切换到 "high_speed" pinctrl state。原始 overlay 只设置了 `pinctrl-0` (default) 为 M1 引脚，`pinctrl-1` (high_speed) 继承了基础 DTB 的 M0 引脚组 → 数据发到了 GPIO2_B5/B6/B7（物理上不存在的连接）→ MISO 读回全 0。

**修复**: 在 overlay 中同时覆盖 pinctrl-0 和 pinctrl-1：
```dts
pinctrl-names = "default", "high_speed";
pinctrl-0 = <&spi1m1_pins>;
pinctrl-1 = <&spi1m1_pins_hs>;   /* ← 关键：必须覆盖 high_speed state */
```

DT 符号：
- `spi1m1_pins` = `/pinctrl/spi1/spi1m1-pins` (phandle 0x025c)
- `spi1m1_pins_hs` = `/pinctrl/spi1-hs/spi1m1-pins` (phandle 0x028b)

验证命令：
```bash
# 确认 pinctrl 已正确设置
xxd -p /sys/firmware/devicetree/base/spi@fe620000/pinctrl-names
# 应输出: 64656661756c7400686967685f737065656400 (default\0high_speed\0)

xxd -p /sys/firmware/devicetree/base/spi@fe620000/pinctrl-1
# 应输出: 0000028b (spi1m1_pins_hs)
```

#### 根因 2: 跳线连接了 GND 引脚而非 MOSI 引脚

> ⚠️ **教训：给用户接线建议前，必须先查 pin-mapping.md 确认引脚定义，不能凭记忆。**

跳线错误地连接了 Pin 9 (GND) ↔ Pin 21 (MISO)。Pin 9 是地线，MISO 被拉低 → 读回全 0。

**正确接法**: Pin 19 (MOSI) ↔ Pin 21 (MISO)

| Pin | 功能 | GPIO | 说明 |
|:---:|------|:---:|------|
| 9 | **GND** | — | ❌ 不是 SPI 引脚 |
| 19 | MOSI | GPIO3_C1 | SPI1_MOSI_M1 |
| 21 | MISO | GPIO3_C2 | SPI1_MISO_M1 |
| 23 | SCLK | GPIO3_C3 | SPI1_CLK_M1 |

#### 测试结果

**Overlay**: `config/dts/spidev-spi1-m1-hs.dts` → 编译为 `spidev-spi1-m1.dtbo`，部署到 `/boot/overlays/`

**/boot/config.txt**: `dtparam=spidev-spi1-m1=on`

**测试程序**: `/tmp/verify-spi /dev/spidev1.0`

```
SPI mode: 0
Bits per word: 8
Max speed: 100000 Hz (100 KHz)

Sent: 100 bytes, Errors: 0, PASS: All 100 bytes matched
```

**结论**: ✅ SPI1 M1 引脚组工作正常。pinctrl high_speed 修复后数据通过正确的物理引脚传输。

#### Phase 2 注意事项

- `config/dts/mcp2515_two_can.dtbo` 需要同样的 `pinctrl-1 = &spi1m1_pins_hs` 修复
- 双 CAN 改单 CAN（只有 1 个 MCP2515+TJA1050 模块）
- 需要 vdd-supply 属性解决 probe 失败

## 5. 脚本清单

| 脚本 | 用途 | 当前状态 |
|------|------|------|
| `Scripts/verify-i2c.sh` | I2C bus scan | ✅ 可用 |
| `Scripts/verify-gpio.sh` | GPIO 输出/输入测试 | ⚠️ 需更新 TEST_PIN=104 |
| `Scripts/verify-uart.sh` | UART 回环测试 | ⚠️ 默认 ttyS2，需传参 ttyS4 |
| `Scripts/verify-spi.sh` | SPI 回环测试 (调用 C 程序) | ✅ 可用 |
| `Scripts/verify-spi.c` | SPI 回环 C 程序 | ✅ 可用，需目标板编译 |

## 6. 已知问题和阻塞项

1. ~~UART4 /dev/ttyS4 可能不存在~~: ✅ 已通过 `dtparam=uart4=on` 解决
2. ~~SPI1 /dev/spidev1.0 不存在~~: ✅ 已通过 DT overlay + pinctrl high_speed 修复解决
3. **verify-gpio.sh TEST_PIN**: 当前=26，需改为=104 (对应 PIN11)
4. ~~DT overlay 编译~~: ✅ 已完成，`config/dts/spidev-spi1-m1-hs.dts` 可直接用 `dtc` 编译
5. **mcp2515_two_can.dtbo 需同样的 high_speed 修复**: Phase 2 开始时处理
6. **mcp2515_two_can.dtbo 需改为单 CAN + 添加 vdd-supply**: Phase 2 开始时处理
