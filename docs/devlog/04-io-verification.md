# Dev Log — IO Verification

> 日期：待填
> 硬件：Core3566 + CM4-IO-BASE-B + logic analyzer + breadboard

## 1. GPIO

- GPIO sysfs enabled (CONFIG_GPIO_SYSFS=y)
- Pin numbers need verification with logic analyzer (Phase 2+)

## 2. UART

| Device | TX Pin | RX Pin | Loopback Test | Notes |
|------|:---:|:---:|:---:|------|
| ttyS1 | ❓ | ❓ | ❌ | Only UART exposed, console on ttyFIQ0 |

## 3. SPI

| Device | Status | Notes |
|------|:---:|------|
| spidev0.0 | ❌ 未启用 | 需要设备树 overlay；内核模块缺失（lite镜像） |

## 4. I2C ✅

| Bus | Scan Result | Notes |
|:---:|------|------|
| i2c-0 | 空 (all --) | 正常，无外设 |
| i2c-1 | 0x0c, **0x2f**(EMC2301风扇), 0x45, **0x51**(RTC) | ✅ |
| i2c-3 | 空 (all --) | 触摸屏待 Phase 6 |

## 5. Issues

1. **SPI 无法验证**: 内核模块未包含在 lite 镜像中，需要 `apt install` 或手动编译设备树
2. **UART 有限**: 只有 `/dev/ttyS1` 可用，回环测试需 GPIO 引脚映射确认后的 Phase 2
