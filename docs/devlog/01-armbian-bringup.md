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

1. **WiFi (AP6256) 5GHz 不可用** 🔴 → 🟡 已定位根因
   - **现象**: Station 模式下无法扫描 5GHz 频道（`iw dev wlan0 scan freq 5180-5825` 返回 0 结果）
   - **根因**: BSP 4.19 `dhd` 驱动（101.10.361.24）的 5GHz Station 模式有 bug。`iw phy` 列出 5GHz 频道但扫描不到 AP
   - **2.4GHz 正常**: 同一驱动 2.4GHz 工作正常。已用手机 2.4GHz 热点验证：连接成功、DHCP 正常、ping 8.8.8.8 0% 丢包
   - **解决**: 升级 Rockchip BSP 5.10 内核（保留 NPU + 修复 5GHz WiFi）
   - **调试过程**: 先排查 wpa_supplicant 配置（缺少 conf）、NetworkManager 冲突、SSID 误配为 WPA2-EAP（实为 WPA2-PSK）、再定位到 5GHz 硬件层扫描失败

2. **NetworkManager 干扰**: 手动启动 wpa_supplicant 后 NM 会杀掉进程。解决：`systemctl stop NetworkManager; systemctl disable NetworkManager`

3. **关键配置参数**: 老 dhd 驱动需要 `ieee80211w=0`（禁用 PMF），否则 WPA2/3 混合模式 AP 的 4-way 握手失败

4. **Debian Buster EOL**: 原镜像 USTC 源已失效。已切换到 `archive.debian.org` + DNS 修复（`systemd-resolved` 停止后手动设 `223.5.5.5`）。`apt update` 成功，136 个包可升级。

5. **方案 A 固件更新（死胡同）**: Luckfox Pico 仓库的 AP6256 固件与 Core3566 现有固件完全相同。5GHz bug 在 dhd 驱动代码中，非固件问题。

6. **长期 5GHz 路径**: 需 Armbian 主线 6.1+（`brcmfmac` 原生支持 5GHz）+ RKNPU 外挂模块（`github.com/rockchip-linux/rknpu` 支持独立编译）。Phase 8 前执行。

7. **启动时间 10min → 6.87s**: `systemd-time-wait-sync.service` 无 RTC 电池 + 无网络时等 NTP 超时 10 分钟。已 mask + 禁用 NetworkManager-wait-online、apt-daily 定时器。

8. **SPI 设备节点缺失**: 4 个 SPI 控制器（spi@fe610000 ~ fe640000）状态均为 `disabled`。CONFIG_SPI_SPIDEV=y 但无设备树设备声明。自定义 dtoverlay 被 BSP 拒绝（fragment 编号冲突机制不兼容）。工厂 `mcp2515_can0=on` overlay 已启用，但无物理芯片时不创建设备节点。→ Phase 2 接 MCP2515 CAN 模块后自然解决。

9. **Qt5 安装受阻**: 无天线 → 2.4GHz WiFi 下载大文件频繁断连 → apt 反复失败。已安装 dtc (device-tree-compiler)。Qt5 待天线到货后继续。

10. **dhd 驱动串口刷屏**: WiFi 连接后 dhd 驱动向 ttyFIQ0 持续输出调试信息（每秒数条），淹没了串口自动化脚本的输出。已用 `dmesg -n 1` 抑制内核消息。
