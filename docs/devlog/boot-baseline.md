# Boot Baseline — Core3566 Armbian

> 测量日期：2026-05-30 (第二轮完整测量，含 Qt6 eglfs 首帧)
> 硬件：Core3566 (RK3566, 2G/32G) + CM4-IO-BASE-B + eMMC 启动
> 系统：Debian 10 (buster), 内核 4.19.232 (Rockchip BSP)
> GPU：Mali-G52 (proprietary driver, OpenGL ES 3.2)

## Measured Boot Times

### A. 默认配置 (wifi-autoconnect 启用)

```
Startup finished in 3.239s (kernel) + 13.656s (userspace) = 16.896s
graphical.target reached after 13.590s in userspace
```

⚠️ **wifi-autoconnect.service 是最大瓶颈**：10.642s（脚本内 sleep 5 + dhclient 等待阻塞 multi-user.target）

#### systemd-analyze blame (Top 15)

```
 10.642s wifi-autoconnect.service  ← 阻塞启动！
  1.543s dev-mmcblk0p8.device
  1.240s rkwifibt.service
  1.216s usbdevice.service
  1.211s resize-all.service
  1.044s udisks2.service
   940ms hwclock-sync.service
   717ms systemd-resolved.service
   548ms systemd-udev-trigger.service
   538ms systemd-journald.service
   417ms cpufrequtils.service
   407ms loadcpufreq.service
   365ms keyboard-setup.service
   269ms user@1000.service
   251ms networking.service
```

#### systemd-analyze critical-chain

```
graphical.target @13.590s
└─multi-user.target @13.589s
  └─wifi-autoconnect.service @2.942s +10.642s
    └─wpa_supplicant.service @2.772s +153ms
      └─dbus.service @2.669s
        └─basic.target @2.659s
          └─sockets.target @2.659s
            └─ssh.socket @2.658s
              └─sysinit.target @2.628s
                └─systemd-timesyncd.service @2.438s +188ms
                  └─systemd-tmpfiles-setup.service @2.325s +98ms
                    └─local-fs.target @2.287s
                      └─userdata.mount @2.309s +29ms
                        └─systemd-fsck@dev-disk-by\x2dpartlabel-userdata.service @2.176s +109ms
                          └─dev-disk-by\x2dpartlabel-userdata.device @1.992s
```

### B. 优化配置 (wifi-autoconnect 禁用)

```
Startup finished in 3.218s (kernel) + 3.954s (userspace) = 7.173s
graphical.target reached after 3.890s in userspace
```

#### systemd-analyze blame (Top 15)

```
  1.270s usbdevice.service
  1.241s dev-mmcblk0p8.device
  1.229s rkwifibt.service
  1.109s resize-all.service
  1.088s udisks2.service
   877ms hwclock-sync.service
   786ms systemd-resolved.service
   499ms cpufrequtils.service
   463ms systemd-journald.service
   449ms systemd-udev-trigger.service
   439ms loadcpufreq.service
   330ms keyboard-setup.service
   317ms polkit.service
   290ms systemd-logind.service
   285ms async.service
```

#### systemd-analyze critical-chain

```
graphical.target @3.890s
└─udisks2.service @2.678s +1.088s
  └─basic.target @2.657s
    └─paths.target @2.654s
      └─acpid.path @2.654s
        └─sysinit.target @2.629s
          └─systemd-timesyncd.service @2.441s +186ms
            └─systemd-tmpfiles-setup.service @2.344s +79ms
              └─local-fs.target @2.340s
                └─userdata.mount @2.309s +29ms
                  └─systemd-fsck@dev-disk-by\x2dpartlabel-userdata.service @2.176s +109ms
                    └─dev-disk-by\x2dpartlabel-userdata.device @1.992s
```

### Manual Measurements

| Stage | Start Event | End Event | 默认 (s) | 优化 (s) |
|------|------------|-----------|:---:|:---:|
| U-Boot SPL | Power on | DDR init done | ~1.0s* | ~1.0s* |
| U-Boot proper | DDR init done | "Starting kernel" | ~0.8s* | ~0.8s* |
| Kernel boot | "Starting kernel" | systemd start | 3.239 | 3.218 |
| Userspace init | PID 1 start | `graphical.target` | 13.590 | 3.890 |
| Qt eglfs cold | App launch | First frame render | 1.158 | 1.158 |
| Qt eglfs warm | App launch | First frame render | 0.467 | 0.467 |
| **Total cold boot** | Power on | UI first frame | **~18.8s** | **~9.3s** | **~9.7s** |

> \* U-Boot 时间为估算值：dmesg 无直接 firmware 时间戳，需串口秒表实测。
> 估算依据：RK3566 SPL + U-Boot 典型值 ~1.5-2s (DDR init + device probe + kernel load)
>
> Qt eglfs 首帧测量方法：QSG_INFO=1 输出 "texture atlas" 行时间戳，从进程启动开始计时。
> 冷启动 (cold) = reboot 后首次运行；热启动 (warm) = 刚运行过第二次。

## Qt eglfs First Frame Detail

```
# 冷启动 (reboot后首次)
$ sudo QT_QPA_PLATFORM=eglfs QT_QPA_EGLFS_INTEGRATION=eglfs_kms QSG_INFO=1 /home/linaro/qt6-test/qt6-test
  → "rhi texture atlas dimensions" 出现在 1158ms
  → GPU: Mali-G52, OpenGL ES 3.2 (ARM vendor)
  → threaded render loop, vsync 16.67ms

# 热启动 (shader/pipeline cache已缓存)
$ sudo QT_QPA_PLATFORM=eglfs QT_QPA_EGLFS_INTEGRATION=eglfs_kms QSG_INFO=1 /home/linaro/qt6-test/qt6-test
  → "rhi texture atlas dimensions" 出现在 467ms
  → 冷/热差异 691ms = shader compilation (SPIR-V → Mali ISA)
```

### C. On-demand WiFi 配置 (wifi-autoconnect.timer，当前方案)

```
Startup finished in 3.382s (kernel) + 4.548s (userspace) = 7.931s
graphical.target reached after 4.467s in userspace
```

方案：`wifi-autoconnect.service` 改为 `Type=oneshot` + `RemainAfterExit=yes`，不随 boot 自动启动。
改为 `wifi-autoconnect.timer` 在 `OnBootSec=10s` 后触发，WiFi 在后台连接，不阻塞启动。

```ini
# wifi-autoconnect.timer
[Unit]
Description=Start WiFi auto-connect 10s after boot (non-blocking)
[Timer]
OnBootSec=10s
AccuracySec=1s
[Install]
WantedBy=timers.target

# wifi-autoconnect.service
[Unit]
Description=Auto-connect WiFi (on-demand, non-blocking)
After=wpa_supplicant.service
Wants=wpa_supplicant.service
[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/local/bin/wifi-connect.sh
[Install]
WantedBy=
```

⚠️ **踩坑记录**：`Type=notify` 会导致服务失败！因为 `wifi-connect.sh` 没有调用 `systemd-notify --ready`，
systemd 等待通知超时后判定 `Failed with result 'protocol'`，配合 `Restart=on-failure` 会导致重启循环。
正确做法：`Type=oneshot` + `RemainAfterExit=yes`。

## Optimization Targets (Buildroot - Phase 9)

| Stage | Current 默认 | Current 优化 | Target (Buildroot) | Saving Method |
|------|:---:|:---:|:---:|------|
| U-Boot | ~1.8s | ~1.8s | ≤ 1.5s | Fast boot mode, skip USB, hardcode config |
| Kernel | 3.24s | 3.22s | ≤ 2.5s | Trim unused drivers, initramfs |
| Userspace | 13.59s | 3.89s | ≤ 3.0s | Remove unnecessary services, on-demand WiFi |
| Qt eglfs cold | 1.16s | 1.16s | ≤ 0.5s | Pre-compile shaders, Qt Quick Compiler |
| Qt eglfs warm | 0.47s | 0.47s | ≤ 0.3s | Already fast, cache persistence |
| **Total cold** | **~18.8s** | **~9.3s** | **≤ 8.0s** | |

## Services to Optimize / Remove

### 必须处理 (阻塞关键路径)

| Service | Time (s) | Action | Reason |
|------|:---:|------|------|
| wifi-autoconnect | 10.642 | **改为 on-demand** | 脚本 sleep 5 + dhclient 等待阻塞 multi-user.target |
| udisks2 | 1.088 | **移除** | 车机无热插拔存储需求，优化后关键路径上 |
| hwclock-sync | 0.940 | **延迟** | RTC 有备份电池，可异步执行 |

### 应移除 (非必要服务)

| Service | Time (s) | Reason |
|------|:---:|------|
| usbdevice | 1.270 | 车机无 USB gadget 需求 |
| resize-all | 1.211 | 一次性 resize，首 boot 后禁用 |
| rkwifibt | 1.229 | WiFi/BT 固件加载，改为按需 |
| cpufrequtils + loadcpufreq | 0.938 | 可合并到 kernel governor |
| polkit | 0.317 | 嵌入式无 polkit 需求 |
| lightdm | 0.355 | 使用 eglfs，无需 X11 display manager |
| systemd-resolved | 0.786 | 使用静态 /etc/resolv.conf |
| networking | 0.251 | 静态 IP 或 connman |

### 优化策略

1. ~~**wifi-autoconnect 改 on-demand**~~：✅ 已完成！使用 timer 触发，节省 10.6s。当前开机 7.9s
2. **移除 udisks2**：关键路径收益 1.1s
3. **Buildroot 裁剪**：移除所有桌面服务，仅保留 Qt eglfs + 核心驱动
4. **Shader 预编译**：Qt 冷启动 1.2s→0.5s (使用 qtquickcompiler 或预缓存 pipeline cache)
5. **U-Boot fast boot**：跳过 USB probe，hardcode bootargs，预计节省 0.5s

## Kernel Modules to Trim (Buildroot)

| Module / Subsystem | Reason | Saving |
|------|------|:---:|
| USB host (xhci, ehci, ohci) | No USB devices in final product | ~500KB |
| PCIe (pcie-rockchip-host) | Not used, SPI/I2C/UART are on-chip | ~300KB |
| Ethernet (stmmac, dwmac) | Production no wired network | ~200KB |
| HDMI/DP output | Only DSI display used | ~200KB |
| Camera ISP (rkisp) | No camera | ~400KB |
| NPU (RKNN) | Keep! Voice engine needs it | 0 |
| WiFi (AP6256 brcmfmac) | Keep for CarPlay 5GHz | 0 |
| Bluetooth (hci_uart, btusb) | Keep for A2DP/HFP | 0 |
| Filesystems (ext2/3, btrfs, xfs, nfs) | Keep only ext4 + vfat | ~300KB |
| Sound (USB audio, HDMI audio) | Keep only I2S + PDM | ~150KB |
| Crypto (af_alg, dm-crypt) | Not needed | ~200KB |
| Netfilter / iptables | Not needed for offline | ~300KB |
| **Estimated total trimming** | | **~2.5MB** |

## systemd Services to Remove (Armbian Baseline)

```bash
# 立即可禁用 (不影响基本功能):
sudo systemctl disable wifi-autoconnect.service  # 收益 10.6s！
sudo systemctl disable usbdevice.service          # 收益 1.3s
sudo systemctl disable resize-all.service         # 首次启动后
sudo systemctl disable udisks2.service            # 收益 1.1s
sudo systemctl disable polkit.service             # 嵌入式无需
sudo systemctl disable lightdm.service            # 使用 eglfs

# 需谨慎 (影响网络/调试):
sudo systemctl disable rkwifibt.service           # WiFi/BT 固件，按需加载
sudo systemctl disable hwclock-sync.service       # 可异步
sudo systemctl disable systemd-resolved.service   # 改用静态 DNS
```

## Buildroot Package Selection (Draft)

**KEEP:**
- `qt6base`, `qt6quick`, `qt6quickcontrols2` (UI)
- `bluez5` + `bluez-alsa` (Bluetooth)
- `pulseaudio` (Audio mixing)
- `can-utils` (SocketCAN debug)
- `i2c-tools`, `spi-tools` (HW debug)
- `dropbear` (SSH for dev)
- `e2fsprogs` (ext4 fsck)

**DROP:**
- `python3`, `perl`, `nodejs` (scripting langs)
- `apt`, `dpkg` (package manager)
- `systemd-udev` (static /dev preferred)
- `glibc-locales` (keep en_US only)
- `terminfo` (minimal)
- All X11/Wayland libs

## Saleae Logic 8 MCP (IO 测量工具)

### 设备状态
- **型号**: Saleae Logic 8 (8-channel logic analyzer)
- **Device ID**: `DEA3665FCEFE5CAB` (MCP `get_devices` 确认)
- **MCP Server**: 端口 10530，Logic 2 → Settings → Automation → MCP Server ✅
- **VS Code 集成**: 用户级 MCP 配置 `http://127.0.0.1:10530`

### 可用于
- Group 4 IO 信号捕获和验证（GPIO/UART/SPI 波形分析）
- 开机时序测量（U-Boot → Kernel → Qt 首帧信号边沿）
- CAN 总线调试（Phase 2+）

### MCP 工具清单
`start_capture`, `stop_capture`, `wait_capture`, `add_analyzer`, `remove_analyzer`,
`add_high_level_analyzer`, `export_data_table_csv`, `export_raw_data_csv`,
`export_raw_data_binary`, `legacy_export_analyzer`, `get_devices`,
`load_capture`, `save_capture`, `close_capture`
