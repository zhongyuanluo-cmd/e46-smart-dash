# HW Pin Mapping: CM4-IO-BASE-B 40-pin Header → RK3566 GPIO

> 来源：Core3566 Datasheet Page 4 + Waveshare CM4-IO-BASE-B 原理图 + 内核 pinctrl-maps + DTBO 解码
> 最后更新：2026-05-30

## 40-pin Header Physical Layout

```
                    ┌──────────────────────┐
PIN1  3.3V  ←  [ 1][ 2]  →  5V            PIN2
PIN3  GPIO2 ←  [ 3][ 4]  →  5V            PIN4
PIN5  GPIO3 ←  [ 5][ 6]  →  GND           PIN6
...
PIN39 GND   ←  [39][40]  →  GPIO21         PIN40
                    └──────────────────────┘
```

## Complete 40-pin Header GPIO Table

> ✅ 来源：Core3566 Datasheet Page 4 "Alternative function assignments"
> RK3566 GPIO 编码规则：bank=A/B/C/D→0/1/2/3, pin_number=bank*8+偏移 (A0=0, B0=8, C0=16, D0=24)

| Header Pin | GPIO# | RK3566 GPIO | Kernel GPIO# | Pull | ALT0 | ALT1 | ALT2 | ALT3 |
|:---:|:---:|:---:|:---:|:---:|------|------|------|------|
| PIN 1 | — | 3.3V | — | — | Power | | | |
| PIN 2 | — | 5V | — | — | Power | | | |
| PIN 3 | GPIO2 | GPIO3_B4 | 108 | Low | SDA5 | I2C5_SDA_M0 | | |
| PIN 4 | — | 5V | — | — | Power | | | |
| PIN 5 | GPIO3 | GPIO3_B3 | 107 | Low | SCL5 | I2C5_SCL_M0 | | |
| PIN 6 | — | GND | — | — | | | | |
| PIN 7 | GPIO4 | GPIO3_B7 | 111 | Low | — | PWM12_M0 | UART3_TX_M1 | |
| PIN 8 | GPIO14 | GPIO0_D1 | 25 | Low | — | UART2_TX_M0 | | **CH343 RXD** |
| PIN 9 | — | GND | — | — | | | | |
| PIN 10 | GPIO15 | GPIO0_D0 | 24 | Low | — | UART2_RX_M0 | | **CH343 TXD** |
| PIN 11 | GPIO17 | GPIO3_B0 | 104 | Low | — | | | |
| PIN 12 | GPIO18 | GPIO4_C6 | 150 | Low | — | PWM13_M1 | SPI3_CS0_M1 | |
| PIN 13 | GPIO27 | GPIO3_C4 | 116 | Low | — | PWM14_M0 | UART7_TX_M1 | |
| PIN 14 | — | GND | — | — | | | | |
| PIN 15 | GPIO22 | GPIO3_C5 | 117 | Low | — | PWM15_IR_M0 | UART7_RX_M1 | |
| PIN 16 | GPIO23 | GPIO3_A1 | 97 | Low | — | SPI1_CS0_M1 | | **✅P2: MCP2515 INT (单CAN)** |
| PIN 17 | — | 3.3V | — | — | Power | | | |
| PIN 18 | GPIO24 | GPIO3_A2 | 98 | Low | — | I2S3_MCLK_M0 | | |
| PIN 19 | **GPIO10** | **GPIO3_C1** | **113** | Low | MOSI | **SPI1_MOSI_M1** | UART5_TX_M1 | |
| PIN 20 | — | GND | — | — | | | | |
| PIN 21 | **GPIO9** | **GPIO3_C2** | **114** | Low | MISO | **SPI1_MISO_M1** | UART5_TX_M1 | |
| PIN 22 | GPIO25 | GPIO3_A3 | 99 | Low | — | I2S3_SCLK_M0 | | ✅P2: 空闲 (原双CAN CAN1 INT, 已弃用) |
| PIN 23 | **GPIO11** | **GPIO3_C3** | **115** | Low | SCLK | **SPI1_CLK_M1** | UART5_RX_M1 | |
| PIN 24 | GPIO8 | GPIO3_A4 | 100 | Low | CE0 | I2S3_LRCK_M0 | | **✅P2: MCP2515 CS (单CAN)** |
| PIN 25 | — | GND | — | — | | | | |
| PIN 26 | GPIO7 | GPIO3_A5 | 101 | Low | CE1 | I2S3_SDO_M0 | | ✅P2: 空闲 (原双CAN CAN1 CS, 已弃用) |
| PIN 27 | GPIO0 | GPIO3_B6 | 110 | Low | SDA3 | I2C3_SDA_M1 | PWM11_IR_M0 | |
| PIN 28 | GPIO1 | GPIO3_B5 | 109 | Low | SCL3 | I2C3_SCL_M1 | PWM10_M0 | |
| PIN 29 | GPIO5 | GPIO3_C0 | 112 | Low | — | PWM13_M0 | UART3_RX_M1 | |
| PIN 30 | — | GND | — | — | | | | |
| PIN 31 | GPIO6 | GPIO4_C4 | 148 | Low | — | I2S3_LRCK_M1 | | |
| PIN 32 | GPIO12 | GPIO3_B2 | 106 | Low | — | UART4_TX_M1 | PWM9_M0 | | **✅P2: K-Bus TX (ttyS4)** |
| PIN 33 | GPIO13 | GPIO3_B1 | 105 | Low | — | UART4_RX_M1 | PWM8_M0 | | **✅P2: K-Bus RX (ttyS4)** |
| PIN 34 | — | GND | — | — | | | | |
| PIN 35 | GPIO19 | GPIO4_C5 | 149 | Low | — | PWM12_M1 | SPI3_MISO_M1 | |
| PIN 36 | GPIO16 | GPIO3_A6 | 102 | Low | — | I2S3_SDI_M0 | | |
| PIN 37 | GPIO26 | GPIO3_A7 | 103 | Low | — | | | |
| PIN 38 | GPIO20 | GPIO4_C3 | 147 | Low | — | SPI3_MOSI_M1 | PWM15_IR_M1 | |
| PIN 39 | — | GND | — | — | | | | |
| PIN 40 | GPIO21 | GPIO4_C2 | 146 | Low | — | SPI3_CLK_M1 | PWM14_M1 | |

> 📝 PIN1 识别：底板 40PIN 排针旁有丝印标号，PIN1 附近有▼三角或白点标记。
> 方向：RJ45/USB 朝下，40PIN 在右侧，PIN1 在上方（靠近 Core3566 模块）。

## Previously Verified Assignments — Phase 1

> ✅ 来源：Luckfox Core3566 官方文档 https://wiki.luckfox.com/Core3566/#52-串口登录

| Header Pin | Function | Connected To | Verified |
|:---:|------|------|:---:|
| PIN 4 | 5V | 7" DSI LCD 红色电源线 | ✅ |
| PIN 6 | GND | 7" DSI LCD 黑色电源线 | ✅ |
| PIN 8 | UART2_TX (GPIO14) | CH343 RXD（交叉） | ✅ |
| PIN 10 | UART2_RX (GPIO15) | CH343 TXD（交叉） | ✅ |
| PIN 14 | GND | CH343 GND | ✅ |

## SPI1 M1 Pin Mapping (for MCP2515 CAN controller — ✅ Phase 2, 已安装)

> ✅ DTBO: `mcp2515_can0.dtbo` (单CAN, vdd-supply via regulator-fixed)
> ✅ 硬件: MCP2515+TJA1050 已接线, can0 已上线 (ERROR-ACTIVE, 500kbps)
> ✅ 引脚: SPI1_M1 (Pin 19/21/23), CS=Pin24, INT=Pin16

| Signal | Header Pin | RK3566 GPIO | GPIO# | Status |
|------|:---:|:---:|:---:|:---:|
| MOSI | 19 | GPIO3_C1 | GPIO10 | ✅ 已接线 |
| MISO | 21 | GPIO3_C2 | GPIO9 | ✅ 已接线 |
| SCLK | 23 | GPIO3_C3 | GPIO11 | ✅ 已接线 |
| CS (sw) | 24 | GPIO3_A4 | GPIO8 | ✅ 已接线 |
| INT | 16 | GPIO3_A1 | GPIO23 | ✅ 已接线 |

> 📝 SPI1 loopback 已于 Phase 1 验证通过 (Pin19↔Pin21, 100/100 bytes)。

## SPI0 Pin Mapping (not available on 40-pin header)

> SPI0 的两组 mux 引脚均未路由到 Core3566 的 40-pin header：
> - m0: GPIO0_B5/B6/C5/C6 — 不在 40-pin header GPIO 范围
> - m1: GPIO2_D0-D3 — 不在 40-pin header GPIO 范围

| Mux Group | CLK | MOSI | MISO | CS0 | On 40-pin? |
|------|------|------|------|------|:---:|
| m0 | GPIO0_C5(pin21) | GPIO0_B6(pin14) | GPIO0_B5(pin13) | GPIO0_C6(pin22) | ❌ |
| m1 | GPIO2_D1(pin25) | GPIO2_D0(pin24) | GPIO2_D3(pin27) | GPIO2_D2(pin26) | ❌ |

## UART Pin Mapping

| UART | tty | TX Pin | RX Pin | Device | Phase |
|------|-----|:---:|:---:|------|:---:|
| UART2 | ttyS2 | 8 | 10 | CH343 USB-UART 串口终端 | 1 |
| UART4 | ttyS4 | 32 | 33 | K-Bus TH3122.4 | 2 |
| — | — | — | — | (其余 UART 空闲可用) | — |

## I2C Pin Mapping

| Bus | SDA Pin | SCL Pin | Device | Phase |
|-----|:---:|:---:|------|:---:|
| I2C0 | 27 | 28 | Touchscreen | 6 |

---

> 📝 **填写说明**：用万用表或逻辑分析仪逐个 pin 测量后填写上表。
> 参考 Waveshare CM4-IO-BASE-B 原理图: https://www.waveshare.net/wiki/CM4-IO-BASE-B
