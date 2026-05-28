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
