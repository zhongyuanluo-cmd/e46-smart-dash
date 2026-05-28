# Dev Log — Armbian Bringup

> 日期: 2026-05-29
> 硬件：Core3566 (RK3566 2G/32G) + CM4-IO-BASE-B V4
> 镜像：2023-09-13-debian-arm64-DSI1-lite.img (Luckfox 百度网盘)

## 1. SD Card Flashing

- **改为 eMMC 直烧**（跳过 SD 卡）
- 工具: DriverAssitant_v5.0 + RKDevTool v2.84
- 镜像: 2023-09-13-debian-arm64-DSI1-lite.img
- MD5: c5dce6822be273ecce7c1b9f67fa7df0

## 2. First Boot

- UART baud: 1500000-8-N-1
- U-Boot SPL output: normal
- Kernel boot log: normal
- Login prompt reached: yes
- Default login: linaro / linaro

## 3. Kernel & Drivers

```bash
$ uname -r
4.19.232
```

| Driver | Expected | Actual | Notes |
|------|:---:|:---:|------|
| CONFIG_DRM_PANFROST | y/m | N/A | Uses Mali Bifrost driver instead (Rockchip BSP) |
| CONFIG_MALI_BIFROST | y | ✅ y | Mali-G52 GPU supported |
| CONFIG_ROCKCHIP_RKNPU_DRM_GEM | y | ✅ y | NPU driver present |
| CONFIG_GPIO_SYSFS | y | ✅ y | |
| CONFIG_SPI_SPIDEV | y/m | ✅ y | |
| CONFIG_I2C_CHARDEV | y/m | ✅ y | |

## 4. DSI Display

| Check | Expected | Actual |
|------|:---:|:---:|
| DRM connector | card0-DSI-1 | ✅ card0-DSI-1 |
| Status | connected | ✅ connected |
| Mode | 800x480 | ✅ 800x480 |
| Framebuffer console | visible | ✅ 有字 |

## 5. Disk

```
/dev/root  26G  2.2G  23G  9% /
```

## 6. Issues

1. **WiFi (AP6256) 不可用**: Broadcom dhd 驱动在 Station 模式下扫描失败，无法连接 AP。内核 4.19 已知问题，需升级内核或换用 USB WiFi 适配器。
2. **无网络**: `apt update` 无法执行，阻断了后续包安装。需通过网线或修复 WiFi 解决。
