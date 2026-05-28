# HW Pin Mapping: CM4-IO-BASE-B 40-pin Header → RK3566 GPIO

> 来源：Waveshare CM4-IO-BASE-B 原理图 + RK3566 datasheet
> 最后更新：待硬件验证后填写

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

## Verified Pin Assignments — Phase 1

> ✅ 来源：Luckfox Core3566 官方文档 https://wiki.luckfox.com/Core3566/#52-串口登录

| Header Pin | Function | Connected To | Verified |
|:---:|------|------|:---:|
| PIN 4 | 5V | 7" DSI LCD 红色电源线 | ✅ |
| PIN 6 | GND | 7" DSI LCD 黑色电源线 | ✅ |
| PIN 8 | UART2_TX (GPIO14) | CH343 RXD（交叉） | ✅ |
| PIN 10 | UART2_RX (GPIO15) | CH343 TXD（交叉） | ✅ |
| PIN 14 | GND | CH343 GND | ✅ |

> 📝 PIN1 识别：底板 40PIN 排针旁有丝印标号，PIN1 附近有▼三角或白点标记。
> 方向：RJ45/USB 朝下，40PIN 在右侧，PIN1 在上方（靠近 Core3566 模块）。

## SPI0 Pin Mapping (for MCP2515 CAN controller - Phase 2)

| Signal | Header Pin | RK3566 GPIO | Connected |
|------|:---:|:---:|:---:|
| CS   | 24 | ❓ | ❌ |
| MOSI | 19 | ❓ | ❌ |
| MISO | 21 | ❓ | ❌ |
| SCLK | 23 | ❓ | ❌ |

## SPI1 Pin Mapping (for ICM-42688 IMU - Phase 3)

| Signal | Header Pin | RK3566 GPIO | Connected |
|------|:---:|:---:|:---:|
| CS   | 26 | ❓ | ❌ |
| MOSI | ❓ | ❓ | ❌ |
| MISO | ❓ | ❓ | ❌ |
| SCLK | ❓ | ❓ | ❌ |

## UART Pin Mapping

| UART | TX Pin | RX Pin | Device | Phase |
|------|:---:|:---:|------|:---:|
| UART1 | 8 | 10 | GPS ATGM336H | 3 |
| UART2 | ❓ | ❓ | K-Bus TH3122.4 | 2 |
| UART3 | ❓ | ❓ | I-Bus TH3122.4 | 2 |
| UART4 | ❓ | ❓ | AP6256 BT HCI | 4 |

## I2C Pin Mapping

| Bus | SDA Pin | SCL Pin | Device | Phase |
|-----|:---:|:---:|------|:---:|
| I2C0 | 27 | 28 | Touchscreen | 6 |

---

> 📝 **填写说明**：用万用表或逻辑分析仪逐个 pin 测量后填写上表。
> 参考 Waveshare CM4-IO-BASE-B 原理图: https://www.waveshare.net/wiki/CM4-IO-BASE-B
