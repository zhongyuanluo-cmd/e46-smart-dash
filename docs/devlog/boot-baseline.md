# Boot Baseline — Core3566 Armbian

> 测量日期：待测
> 硬件：Core3566 (RK3566, 2G/32G) + CM4-IO-BASE-B + SD 卡启动

## Measured Boot Times

### systemd-analyze

```
$ systemd-analyze
Startup finished in X.XXXs (firmware) + X.XXXs (loader) + X.XXXs (kernel) + X.XXXs (userspace) = X.XXXs
```

### systemd-analyze blame (Top 10)

```
$ systemd-analyze blame
[TBD]
```

### systemd-analyze critical-chain

```
$ systemd-analyze critical-chain
[TBD]
```

### Manual Measurements

| Stage | Start Event | End Event | Time (s) |
|------|------------|-----------|:---:|
| U-Boot SPL | Power on | DDR init done | ❓ |
| U-Boot proper | DDR init done | "Starting kernel" | ❓ |
| Kernel boot | "Starting kernel" | First userspace log | ❓ |
| Userspace init | PID 1 start | `graphical.target` reached | ❓ |
| UI first frame | App launch | Screen shows content | ❓ |
| **Total cold boot** | Power on | UI first frame | ❓ |

## Optimization Targets (Buildroot - Phase 9)

| Stage | Current (Armbian) | Target (Buildroot) | Saving Method |
|------|:---:|:---:|------|
| U-Boot | ❓s | ≤ 1.5s | Fast boot mode, skip USB, hardcode config |
| Kernel | ❓s | ≤ 2.5s | Trim unused drivers, initramfs |
| Userspace + UI | ❓s | ≤ 4.0s | Remove unnecessary services, Qt quick launch |
| **Total** | **❓s** | **≤ 8.0s** | |

## Services to Remove (Buildroot Migration)

| Service | Current Time (s) | Reason |
|------|:---:|------|
| NetworkManager | ❓ | Static IP or none |
| systemd-resolved | ❓ | Not needed offline |
| bluetooth pairing agent | ❓ | Start on demand only |
| cron / anacron | ❓ | Not needed |
| rsyslog | ❓ | Replace with busybox syslogd |
| sshd | ❓ | Keep for debugging, disable for production |
| avahi-daemon | ❓ | mDNS not needed |
| polkit | ❓ | Single-user system |

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

## systemd Services to Remove

```bash
# Disable in Armbian for baseline comparison:
systemctl disable NetworkManager.service
systemctl disable NetworkManager-wait-online.service
systemctl disable systemd-resolved.service
systemctl disable avahi-daemon.service
systemctl disable avahi-daemon.socket
systemctl disable bluetooth.service      # Keep, but start on-demand
systemctl disable cron.service
systemctl disable syslog.service         # Keep, but replace with busybox
systemctl disable polkit.service
systemctl disable ModemManager.service
systemctl disable wpa_supplicant.service # Keep for CarPlay phase
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
