# WiFi 自动连接修复

## 问题

Core3566 板子重启后 WiFi 不会自动连接。需要手动通过串口执行 `wpa_supplicant` 和 `dhclient` 命令才能联网。

## 根因分析

1. **AP6256 驱动加载慢**：Broadcom WiFi 芯片驱动需要 ~10s 才能让 `wlan0` 接口出现
2. **networking.service 时序问题**：`ifup -a` 在 `wlan0` 还不存在时就执行，导致 "No such device" 错误
3. **无重试机制**：`networking.service` 失败后不会重试，WiFi 永远不会自动连上

日志证据：
```
May 30 01:50:06 linaro-alip wpa_supplicant[453]: Could not read interface wlan0 flags: No such device
May 30 01:50:06 linaro-alip ifup[302]: ifup: failed to bring up wlan0
```

## 修复方案

### 1. 修改 interfaces 配置

文件：`/etc/network/interfaces.d/wlan0`

```diff
- auto wlan0
+ allow-hotplug wlan0
  iface wlan0 inet dhcp
  wpa-conf /etc/wpa_supplicant/wpa_supplicant.conf
```

`auto` 表示启动时无条件 `ifup`，`allow-hotplug` 表示仅在内核通知设备存在时才 `ifup`。

### 2. 创建 WiFi 自动连接服务

#### 辅助脚本：`/usr/local/bin/wifi-connect.sh`

```sh
#!/bin/sh
# Wait for wlan0 to appear (up to 60 seconds)
for i in $(seq 1 60); do
    [ -d /sys/class/net/wlan0 ] && break
    sleep 1
done
if [ ! -d /sys/class/net/wlan0 ]; then
    echo "ERROR: wlan0 not found after 60s"
    exit 1
fi

# Kill any stale wpa_supplicant for wlan0
pkill -f 'wpa_supplicant.*wlan0' 2>/dev/null
sleep 1

# Start wpa_supplicant
wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf
sleep 5

# Get DHCP lease
dhclient wlan0
```

#### Systemd 服务：`/etc/systemd/system/wifi-autoconnect.service`

```ini
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

#### Systemd Timer：`/etc/systemd/system/wifi-autoconnect.timer`

```ini
[Unit]
Description=Start WiFi auto-connect 10s after boot (non-blocking)

[Timer]
OnBootSec=10s
AccuracySec=1s

[Install]
WantedBy=timers.target
```

⚠️ **v2 方案改进**：原 v1 使用 `WantedBy=multi-user.target` 会导致 WiFi 连接阻塞启动 10.6s。
改为 timer 在开机 10s 后触发，WiFi 后台连接不阻塞。

### 3. 启用服务

```bash
sudo chmod +x /usr/local/bin/wifi-connect.sh
sudo systemctl daemon-reload
sudo systemctl enable wifi-autoconnect.timer
sudo systemctl start wifi-autoconnect.timer
# 注意：不要 enable wifi-autoconnect.service，它由 timer 触发
```

## 验证结果

重启后自动连接成功（开机 10s 后 timer 触发，WiFi 后台连接）：

```
● wifi-autoconnect.timer - Start WiFi auto-connect 10s after boot (non-blocking)
   Active: active (waiting)

● wifi-autoconnect.service - Auto-connect WiFi (on-demand, non-blocking)
   Active: active (exited) since Sat 2026-05-30 10:40:02 CST
  Process: 635 ExecStart=/usr/local/bin/wifi-connect.sh (code=exited, status=0/SUCCESS)

   inet 192.168.1.161/24 brd 192.168.1.255 scope global dynamic wlan0
```

开机时间从 16.9s → 7.9s（WiFi 不再阻塞启动）。

## 踩坑记录：Type=notify 陷阱

如果设置 `Type=notify`，systemd 会等待服务调用 `systemd-notify --ready`。
`wifi-connect.sh` 不调用此命令，导致 systemd 等待超时后判定 `Failed with result 'protocol'`。
配合 `Restart=on-failure` 会导致无限重启循环。

**正确做法**：`Type=oneshot` + `RemainAfterExit=yes`（oneshot 不需要 notify）。

## SSH 免密登录（附带设置）

为方便开发，已设置 SSH 公钥认证：

```bash
# Windows 侧生成密钥
ssh-keygen -t ed25519 -N '""'

# 通过串口桥写入公钥（base64 编码避免特殊字符问题）
echo <base64_of_pubkey> | base64 -d >> ~/.ssh/authorized_keys
```

之后可通过 `ssh linaro@192.168.1.161` 免密登录，SCP 也无需密码。

## 文件位置

| 文件 | 本地 | 板上 |
|------|------|------|
| wifi-connect.sh | `config/scripts/wifi-connect.sh` | `/usr/local/bin/wifi-connect.sh` |
| wifi-autoconnect.service | `config/systemd/wifi-autoconnect.service` | `/etc/systemd/system/wifi-autoconnect.service` |
| interfaces.d/wlan0 | - | `/etc/network/interfaces.d/wlan0` |
| wpa_supplicant.conf | - | `/etc/wpa_supplicant/wpa_supplicant.conf` |
