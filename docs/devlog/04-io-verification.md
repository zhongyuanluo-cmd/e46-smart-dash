# Dev Log — IO Verification

> 日期：待填
> 硬件：Core3566 + CM4-IO-BASE-B + logic analyzer + breadboard

## 1. GPIO

| Pin | GPIO# | Output Test | Input Test | Notes |
|------|:---:|:---:|:---:|------|
| 3 | ❓ | ❌ | ❌ | |
| 5 | ❓ | ❌ | ❌ | |
| 7 | ❓ | ❌ | ❌ | |

## 2. UART

| Device | TX Pin | RX Pin | Loopback @115200 | Loopback @1500000 | Notes |
|------|:---:|:---:|:---:|:---:|------|
| ttyS2 | ❓ | ❓ | ❌ | ❌ | |

## 3. SPI

| Device | MOSI | MISO | SCLK | CS | Loopback Test | Notes |
|------|:---:|:---:|:---:|:---:|:---:|------|
| spidev0.0 | 19 | 21 | 23 | 24 | ❌ | |

## 4. I2C

| Bus | SDA | SCL | Pull-ups Present | Scan Result | Notes |
|:---:|:---:|:---:|:---:|------|------|
| i2c-0 | 27 | 28 | ❓ | ❓ | |

## 5. Issues & Workarounds

1. [No issues yet]
