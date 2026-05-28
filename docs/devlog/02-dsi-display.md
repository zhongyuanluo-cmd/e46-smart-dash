# Dev Log — MIPI DSI Display Bringup

> 日期：待填
> 硬件：Waveshare 7" DSI LCD + CM4-IO-BASE-B DSI1 connector

## 1. Physical Connection

- FPC cable connected: [yes / no]
- FPC orientation confirmed: [yes / no]
- DSI1 connector used: [yes / no]

## 2. Device Tree Overlay

- Overlay used: [built-in / custom / none]
- Overlay file: 
- Compilation command: `dtc -@ -I dts -O dtb -o /boot/overlays/xxx.dtbo xxx.dts`
- armbianEnv.txt changes:

```
[TBD]
```

## 3. DRM Verification

```bash
$ ls /sys/class/drm/
[TBD]

$ modetest -M rockchip
[TBD]
```

| Check | Expected | Actual | Notes |
|------|:---:|:---:|------|
| DRM connector name | `DSI-1` or similar | ❓ | |
| Connector status | connected | ❓ | |
| Mode | 800x480@60 | ❓ | |

## 4. Framebuffer Console

- Boot log visible on DSI screen: [yes / no]
- Login prompt visible: [yes / no]
- Console font size: [readable / too small / too large]

## 5. Panel Timing Parameters (from datasheet / overlay)

| Parameter | Value | Source |
|------|------|------|
| HFP | ❓ | |
| HBP | ❓ | |
| VFP | ❓ | |
| VBP | ❓ | |
| HSW | ❓ | |
| VSW | ❓ | |
| Pixel Clock | ❓ Hz | |

## 6. Issues & Workarounds

1. [No issues yet]
