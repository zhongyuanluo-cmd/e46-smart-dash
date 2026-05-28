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

## Verified Pin Assignments

> ⚠️ Fill in after physical verification with multimeter/logic analyzer.

| Header Pin | Function | RK3566 GPIO | Device | Verified |
|:---:|------|:---:|------|:---:|
| 3 | GPIO | ❓ | - | ❌ |
| 5 | GPIO | ❓ | - | ❌ |
| 7 | GPIO | ❓ | - | ❌ |
| 8 | UART_TX | ❓ | ❓ | ❌ |
| 10 | UART_RX | ❓ | ❓ | ❌ |
| 11 | GPIO | ❓ | - | ❌ |
| 12 | GPIO | ❓ | - | ❌ |
| 13 | GPIO | ❓ | - | ❌ |
| 15 | GPIO | ❓ | - | ❌ |
| 16 | GPIO | ❓ | - | ❌ |
| 18 | GPIO | ❓ | - | ❌ |
| 19 | SPI_MOSI | ❓ | SPI0 | ❌ |
| 21 | SPI_MISO | ❓ | SPI0 | ❌ |
| 22 | GPIO | ❓ | - | ❌ |
| 23 | SPI_CLK | ❓ | SPI0 | ❌ |
| 24 | SPI_CE0 | ❓ | SPI0 | ❌ |
| 26 | SPI_CE1 | ❓ | SPI0 | ❌ |
| 27 | I2C_SDA | ❓ | I2C0 | ❌ |
| 28 | I2C_SCL | ❓ | I2C0 | ❌ |
| 29 | GPIO | ❓ | - | ❌ |
| 31 | GPIO | ❓ | - | ❌ |
| 32 | GPIO | ❓ | - | ❌ |
| 33 | GPIO | ❓ | - | ❌ |
| 35 | GPIO | ❓ | - | ❌ |
| 36 | GPIO | ❓ | - | ❌ |
| 37 | GPIO | ❓ | - | ❌ |
| 38 | GPIO | ❓ | - | ❌ |
| 40 | GPIO | ❓ | - | ❌ |

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
