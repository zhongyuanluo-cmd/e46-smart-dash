# SSH 上板检查清单 — 独立审查者交接文档

> 创建日期: 2026-05-31 | 创建人: 前任审查 Agent | 用途: 供独立审查者上板验证

---

## 背景

本项目已完成六轮 PC 端代码/文档审查（见 `docs/review/01~06-*.md`），Worker 已修复所有问题。
本轮为 **SSH 上板实际检查**，验证 Core3566 板端环境与项目文档/代码的一致性。

**⚠ 组织纪律：审查者与被审查者不能是同一人。本清单由前任审查 Agent 编写，但上板检查必须由独立审查者执行。**

---

## 板端连接信息

| 项目 | 值 |
|------|-----|
| SSH | `linaro@192.168.1.161` (sudo 无密码) |
| 系统 | Armbian Debian 10, kernel 4.19.232, aarch64 |
| 串口 | COM3, 1500000 baud (备用) |
| 电源 | 需要用户确认是否已上电 |

---

## 检查项目

### 1. 系统基础 ✅/❌

```bash
# 内核版本 (期望: 4.19.232)
uname -r

# 系统架构 (期望: aarch64)
uname -m

# 磁盘空间 (关注 / 是否充足)
df -h

# 内存 (期望: ~2GB 可用)
free -h

# CPU 信息
cat /proc/cpuinfo | head -20

# 运行时间
uptime
```

**验证标准**: 内核 4.19.232, aarch64, 磁盘有充足空间, 内存 ~2GB

---

### 2. Qt6 部署 ✅/❌

```bash
# Qt6 .so 文件 (期望: 50 个版本化 .so)
ls -la /usr/local/lib/libQt6*.so.6.8.2 | wc -l

# 关键模块存在性
ls /usr/local/lib/libQt6Core.so.6.8.2
ls /usr/local/lib/libQt6Gui.so.6.8.2
ls /usr/local/lib/libQt6Qml.so.6.8.2
ls /usr/local/lib/libQt6Quick.so.6.8.2
ls /usr/local/lib/libQt6Widgets.so.6.8.2
ls /usr/local/lib/libQt6Network.so.6.8.2

# ldconfig 链接 (期望: .so.6 指向 .so.6.8.2)
ls -la /usr/local/lib/libQt6Core.so.6
ldconfig -p | grep libQt6Core

# Qt6 版本确认 (期望: 6.8.2)
strings /usr/local/lib/libQt6Core.so.6.8.2 | grep "6\.8\.2" | head -3
```

**验证标准**: 50 个 .so.6.8.2, ldconfig 链接正确, 版本号 6.8.2

---

### 3. GPU 驱动 (Mali-G52) ✅/❌

```bash
# Mali 设备节点 (期望: /dev/mali0 存在)
ls -la /dev/mali0

# libmali 库 (期望: 已安装)
dpkg -l | grep libmali
ls -la /usr/lib/aarch64-linux-gnu/libmali*

# OpenGL ES 版本 (期望: ES 3.2)
# 注意: 需要 eglfs 环境才能测试, 远程 SSH 无法直接运行
# 但可以检查库链接
strings /usr/lib/aarch64-linux-gnu/libmali*.so | grep -i "opengl es" | head -3
```

**验证标准**: /dev/mali0 存在, libmali 已安装, 支持 ES 3.2

---

### 4. Device Tree Overlays ✅/❌

```bash
# 已安装的 overlays (关注 MCP2515 相关)
ls -la /boot/overlays/

# MCP2515 单 CAN overlay (Phase 2 需要, 当前可能是双 CAN 版本)
ls -la /boot/overlays/mcp2515*.dtbo

# 主配置
cat /boot/armbianEnv.txt 2>/dev/null || cat /boot/config.txt 2>/dev/null

# 已加载的 overlays
cat /sys/kernel/config/device-tree/overlays/ 2>/dev/null
ls /sys/kernel/config/device-tree/overlays/ 2>/dev/null

# dmesg 中 MCP2515/CAN 相关信息 (期望: probe 失败因 vdd-supply, 因为硬件未接)
dmesg | grep -i "mcp2515\|can0\|can1" | tail -10
```

**验证标准**: overlay 文件存在, config 正确引用, MCP2515 dmesg 符合预期(硬件未接)

---

### 5. WiFi ✅/❌

```bash
# WiFi 接口 (期望: wlan0 存在)
ip addr show wlan0

# 连接状态 (期望: connected, IP 192.168.1.161)
iwconfig wlan0 2>/dev/null || iw dev wlan0 info 2>/dev/null

# wpa_supplicant 服务 (期望: active)
systemctl status wpa_supplicant 2>/dev/null

# WiFi 自动连接服务 (项目自定义)
systemctl status wifi-autoconnect.service 2>/dev/null
systemctl status wifi-autoconnect.timer 2>/dev/null

# AP6256 固件加载
dmesg | grep -i "brcm\|ap6256\|firmware" | tail -10
```

**验证标准**: wlan0 有 IP, wpa_supplicant 运行, 自定义 systemd 服务正常

---

### 6. I2C ✅/❌

```bash
# I2C 设备节点 (期望: /dev/i2c-0, /dev/i2c-5 等)
ls -la /dev/i2c-*

# I2C 总线探测 (注意: 可能需要 root)
sudo i2cdetect -l

# I2C5 (40-pin Pin 3/5, SDA5/SCL5) 上挂载的设备
sudo i2cdetect -y 5 2>/dev/null || echo "I2C5 not available"
```

**验证标准**: /dev/i2c-* 存在, i2c-5 (40-pin I2C5) 可探测

---

### 7. SPI ✅/❌

```bash
# SPI 设备节点 (期望: /dev/spidev1.0)
ls -la /dev/spidev*

# SPI 驱动加载
dmesg | grep -i "spi\|spidev" | tail -10

# SPI1 状态 (Pin 19/21/23/24, MCP2515 用)
cat /sys/bus/spi/devices/spi1.0/modalias 2>/dev/null || echo "spi1.0 not found"
```

**验证标准**: /dev/spidev1.0 存在 (SPI1), SPI 驱动正常加载

---

### 8. UART ✅/❌

```bash
# 串口设备 (期望: ttyS4 用于 K-Bus)
ls -la /dev/ttyS*

# CH343 USB-UART (调试串口)
ls -la /dev/ttyUSB* 2>/dev/null || echo "No USB-UART"

# UART4 (ttyS4, K-Bus, Pin 32/33) 状态
stty -F /dev/ttyS4 2>/dev/null && echo "ttyS4 accessible" || echo "ttyS4 not accessible"
```

**验证标准**: ttyS4 (UART4/K-Bus) 存在且可访问

---

### 9. SARADC ✅/❌

```bash
# ADC 设备节点 (期望: /sys/bus/iio/devices/iio:device0)
ls -la /sys/bus/iio/devices/

# 电池电压读取 (如可用)
cat /sys/bus/iio/devices/iio:device0/in_voltage0_raw 2>/dev/null || echo "ADC not available"
```

**验证标准**: IIO ADC 设备存在, 能读取原始电压值

---

### 10. 项目构建产物 ✅/❌

```bash
# can-gateway 二进制 (期望: 不在板端, 需要交叉编译后 scp 上去)
ls -la /usr/local/bin/car-* 2>/dev/null || echo "No project binaries on board"

# 检查是否有临时部署
find /tmp /home/linaro -name "can-gateway*" -o -name "car-*" 2>/dev/null | head -10
```

**验证标准**: 明确项目二进制是否已部署到板端

---

### 11. systemd 服务 ✅/❌

```bash
# 项目自定义服务
systemctl list-unit-files | grep -E "can-gateway|wifi-autoconnect|ntp-rtc"

# WiFi 自动连接服务详情
systemctl cat wifi-autoconnect.service 2>/dev/null
systemctl cat wifi-autoconnect.timer 2>/dev/null

# NTP/RTC 同步脚本
ls -la /usr/local/bin/setup-ntp-rtc.sh 2>/dev/null
cat /usr/local/bin/setup-ntp-rtc.sh 2>/dev/null | head -20
```

**验证标准**: 项目 systemd 服务已正确安装和配置

---

### 12. 网络与时间 ✅/❌

```bash
# 网络连通性
ping -c 2 192.168.1.1 2>/dev/null || echo "Gateway unreachable"

# DNS 解析
nslookup baidu.com 2>/dev/null || echo "DNS not working"

# NTP 同步
timedatectl status 2>/dev/null

# RTC 时间
hwclock --show 2>/dev/null || echo "RTC not available"
```

**验证标准**: 网络通, DNS 可用, 时间同步正常

---

## 审查报告模板

独立审查者请按以下格式输出：

```markdown
# SSH 上板检查报告

> 检查日期: YYYY-MM-DD | 检查人: [独立审查者]
> SSH 连接: ✅/❌

## 检查结果汇总

| # | 检查项 | 期望 | 实际 | 判定 |
|---|--------|------|------|------|
| 1 | 系统基础 | 4.19.232/aarch64 | | ✅/❌ |
| 2 | Qt6 部署 | 50 .so, v6.8.2 | | ✅/❌ |
| 3 | GPU 驱动 | Mali-G52, ES 3.2 | | ✅/❌ |
| 4 | DT Overlays | MCP2515 dtbo 存在 | | ✅/❌ |
| 5 | WiFi | IP 192.168.1.161 | | ✅/❌ |
| 6 | I2C | /dev/i2c-* | | ✅/❌ |
| 7 | SPI | /dev/spidev1.0 | | ✅/❌ |
| 8 | UART | ttyS4 可访问 | | ✅/❌ |
| 9 | SARADC | IIO device | | ✅/❌ |
| 10 | 项目构建产物 | — | | ✅/❌ |
| 11 | systemd 服务 | wifi-autoconnect | | ✅/❌ |
| 12 | 网络与时间 | 网络/RTC | | ✅/❌ |

## 问题清单

### 🔴 严重 (必须修复)

### 🟡 建议 (应修复)

### 🔵 信息 (无需修复)

## 审查者声明

本人独立完成本检查，与前任审查 Agent 无利益关联。
```

---

## 前任审查者补充说明

1. **MCP2515 硬件未安装**: 只采购了 1 个模块（不是 2 个），未物理连接。dmesg 中 probe 失败是正常的。
2. **SPI1 已验证**: Phase 1 已完成 SPI1 loopback 测试（MOSI↔MISO 跳线, 100/100 字节匹配）。
3. **WiFi 是唯一网络**: 板端没有以太网连接，全靠 WiFi AP6256。
4. **DT overlay 问题**: `mcp2515_two_can.dtbo` 是双 CAN 版本，Phase 2 需要改为单 CAN。还有 `pinctrl-1` (high_speed state) 修复待做。
5. **K-Bus 收发器未安装**: TH3122.4 还未焊接/安装，ttyS4 存在但无实际通信。
6. **Qt6 eglfs 无法远程测试**: Mali GPU 渲染需要接屏幕 + 本地运行，SSH 无法验证。

---

## 审查者权限

- ✅ 读取所有板端文件
- ✅ 运行诊断命令
- ✅ 查看 dmesg / journalctl
- ❌ 不修改任何板端配置
- ❌ 不安装/卸载软件
- ❌ 不重启系统

---

*本清单由前任审查 Agent 于 2026-05-31 创建，供独立审查者使用。*
