# Dev Log — Armbian Bringup

> 日期：待填
> 硬件：Core3566 (RK3566 2G/32G) + CM4-IO-BASE-B
> 镜像：待填 (Luckfox Armbian image URL)

## 1. SD Card Flashing

- Image URL: 
- Flashing tool: balenaEtcher
- SD card: Raspberry Pi SD Card 32GB A2
- Flash duration: 

## 2. First Boot

- UART baud: 1500000-8-N-1
- U-Boot SPL output: [normal / issue]
- Kernel boot log: [normal / issue]
- Login prompt reached: [yes / no]
- First boot time: s

## 3. Initial Configuration

- armbian-config WiFi: [success / failed]
- Network test (`ping google.com`): [success / failed]
- Locale set: en_US.UTF-8 + zh_CN.UTF-8
- `apt update && apt upgrade`: [success / failed]

## 4. Dev Packages Installed

```
build-essential cmake git vim i2c-tools spi-tools can-utils pkg-config
```

## 5. Kernel Driver Verification

```bash
$ uname -r
[TBD]

$ zcat /proc/config.gz | grep -E "PANFROST|ROCKCHIP|MIPI_DSI|GPIO_SYSFS|SPI_SPIDEV|I2C_CHARDEV"
[TBD]
```

| Driver | Expected | Actual | Notes |
|------|:---:|:---:|------|
| CONFIG_DRM_PANFROST | y/m | ❓ | |
| CONFIG_DRM_ROCKCHIP | y | ❓ | |
| CONFIG_ROCKCHIP_DW_MIPI_DSI | y | ❓ | |
| CONFIG_GPIO_SYSFS | y | ❓ | |
| CONFIG_SPI_SPIDEV | y/m | ❓ | |
| CONFIG_I2C_CHARDEV | y/m | ❓ | |

## 6. Issues & Workarounds

1. [No issues yet]
