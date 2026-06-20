# 参考 — CarPlay 实现方案调研

> 日期：2026-06-09
> 目标：调研 Core3566 (RK3566) 上实现无线 CarPlay 的可行方案，不依赖 Carlinkit 商业模块

---

## 1. 技术本质

CarPlay 协议本质上是三层结构：

```
┌────────────────────────────────────────┐
│  CarPlay 应用层                         │
│  H.264 视频流 + LPCM 音频 + 触控事件    │
├────────────────────────────────────────┤
│  iAP2 会话层 (Control Session Message)  │
│  TLV 编码: 认证/识别/WiFi/车辆状态       │
├────────────────────────────────────────┤
│  iAP2 链路层                            │
│  自定义成帧: FF 55/FF 5A 标记 + ACK + CRC│
├────────────────────────────────────────┤
│  传输层: Bluetooth RFCOMM / WiFi / USB  │
└────────────────────────────────────────┘
```

Phone 做所有重活（渲染 UI、运行 App），车机端只是一个**视频解码器 + 触摸回传器**。

---

## 2. 关键开源项目

### 2.1 `HaToan/carplay-wifi-extractor` ⭐ 最重要

- **GitHub**: https://github.com/HaToan/carplay-wifi-extractor
- **语言**: Python 3.11+ / asyncio
- **用途**: 完整 iAP2 协议栈实现，包含纯软件 MFi 认证框架
- **覆盖范围**:

| 组件            | 文件                             | 功能                         |
| ------------- | ------------------------------ | -------------------------- |
| iAP2 链路层      | `link_layer.py`                | SYN/NEGO/ACK 成帧、CRC、序列号    |
| 蓝牙传输          | `transport/bluetooth.py`       | BlueZ RFCOMM server，BLE 广告 |
| CSM 会话层       | `control_session_message/*.py` | 识别、认证、WiFi、CarPlay 消息编解码   |
| MFi 软件认证      | `mfi_auth_coprocessor.py`      | 可切换硬件 I2C / 纯软件签名模式        |
| 配件模拟 (Server) | `__main__.py`                  | 模拟 CarPlay 车机，接受 iPhone 连接 |
| 手机模拟 (Client) | `client/__main__.py`           | 模拟 iPhone，从真车提取 WiFi 凭证    |

**关键代码 — MFi 软件认证框架**：

```python
# mfi_auth_coprocessor.py
_USE_EMULATED = os.environ.get("IAP2_EMULATE_MFI", "1") != "0"

def read_certificate():
    if _USE_EMULATED:
        return _EMULATED_CERT   # 纯软件返回证书（可替换为真实 X.509）
    # 否则从 I2C 地址读取 MFi 芯片
    # ⚠️ CP 2.0C (MFI337S3959): 地址 0x20 (RST=GND)
    # ⚠️ CP 2.0 (旧版 wiomoc.de): 地址 0x10

def generate_challenge_response(challenge):
    if _USE_EMULATED:
        # 演示用哈希，替换为 ECDSA/RSA 私钥签名即可
        digest = hashlib.sha256(b"IAP2_EMULATED_MFI" + challenge).digest()
        return digest
    # 否则通过 I2C 让 MFi 芯片签名
```

### 2.2 `45clouds/WirelessCarPlay`

- **GitHub**: https://github.com/45clouds/WirelessCarPlay
- **语言**: C (Apple SDK 派生)
- **许可证**: MIT
- **用途**: 完整的 AirPlay/CarPlay 接收器，包含 H.264 视频流处理
- **目标硬件**: Raspberry Pi 3 (armv7l)
- **关键文件**:
  - `AirPlayReceiverSession.c` — CarPlay 会话管理
  - `AppleCarPlay_AppStub.c` — CarPlay 应用桩
  - `MFiServerPlatformLinux.c` — Linux I2C MFi 芯片接口
  - `MFiSAP.c` — MFi Security Association Protocol
- **限制**: 默认需要物理 MFi 芯片 (I2C 地址 0x11，可能是旧版 CP 2.0 配置；CP 2.0C 应使用 0x20/0x21)。需改造为软件模式或适配芯片地址

### 2.3 其他可用项目

| 项目 | 语言 | 用途 |
|------|------|------|
| `electric-monk/pycarplay` | Python | Carlinkit dongle receiver |
| `harrylepotter/carplay-receiver` | Python | Linux CarPlay receiver |
| `rayphee/carplay-client` | Rust | 高性能 Linux CarPlay client |
| `rhysmorgan134/node-CarPlay` | TypeScript | Node.js CarPlay via Carlinkit USB |
| `niellun/FastCarPlay` | C | 轻量 Carlinkit receiver |
| `f-io/LIVI` | TypeScript/Electron | 全功能 CarPlay + AA 车机 |
| `ludwig-v/wireless-carplay-dongle-reverse-engineering` | Python/Shell | Carlinkit 固件逆向 |

---

## 3. iAP2 协议详解

### 3.1 MFi 认证流程

```
iPhone                          CarPlay 配件
  │                                  │
  │──── RequestAuthenticationCertificate (0xAA00) ────►│
  │                                  │
  │◄─── AuthenticationCertificate (0xAA01) ────────────│
  │     └── X.509 DER, ~920 bytes, Apple CA 签发        │
  │                                  │
  │──── RequestChallengeResponse (0xAA02) ────────────►│
  │     └── 20 bytes random challenge                   │
  │                                  │
  │◄─── AuthenticationResponse (0xAA03) ───────────────│
  │     └── ECDSA/RSA 签名 (私钥对 challenge 签名)      │
  │                                  │
  │  用 Apple Root CA 验证证书链 + 签名                  │
  │                                  │
  │──── AuthenticationSucceeded (0xAA05) ─────────────►│
```

**核心事实**: MFi 芯片的唯一作用就是存放证书+私钥并做签名。其他一切都可以纯软件实现。

### 3.2 CSM 消息 ID 参考

| 消息 | ID | 方向 |
|------|:---:|------|
| `RequestAuthenticationCertificate` | 0xAA00 | Client → Accessory |
| `AuthenticationCertificate` | 0xAA01 | Accessory → Client |
| `RequestAuthenticationChallengeResponse` | 0xAA02 | Client → Accessory |
| `AuthenticationResponse` | 0xAA03 | Accessory → Client |
| `AuthenticationFailed` | 0xAA04 | Client → Accessory |
| `AuthenticationSucceeded` | 0xAA05 | Client → Accessory |
| `StartIdentification` | 0x1D00 | Client → Accessory |
| `IdentificationInformation` | 0x1D01 | Accessory → Client |
| `IdentificationAccepted` | 0x1D02 | Client → Accessory |
| `IdentificationRejected` | 0x1D03 | Client → Accessory |
| `RequestAccessoryWiFiConfigurationInformation` | 0x5702 | Client → Accessory |
| `AccessoryWiFiConfigurationInformation` | 0x5703 | Accessory → Client |

### 3.3 无线 CarPlay 完整握手流程

```
Phase 0: 链路层
  iPhone ──► Adapter: ff 55 02 00 ee 10  (SYN)
  Adapter ──► iPhone: ff 55 02 00 ee 10  (SYN-ACK)
  参数协商 (max packet size, retransmit timeout)

Phase 1: 识别
  Adapter ──► iPhone: StartIdentification (0x1D00)
  iPhone ──► Adapter: IdentificationInformation (名称/型号/制造商/蓝牙MAC/支持CarPlay)
  Adapter ──► iPhone: IdentificationAccepted (0x1D02)

Phase 2: MFi 认证
  (见 3.1)

Phase 3: WiFi 凭证交换 (无线 CarPlay 关键步骤)
  Adapter ──► iPhone: RequestAccessoryWiFiConfigurationInformation (0x5702)
  iPhone ──► Adapter: AccessoryWiFiConfigurationInformation (0x5703)
    ├── SSID, Passphrase (明文!)
    └── Security: WPA2, Channel

Phase 4: CarPlay 会话
  iPhone 通过 WiFi 直连 Adapter
  AirPlay 会话建立
  H.264 视频 + LPCM 音频 + 触控回传
```

---

## 4. Core3566 自研方案

### 4.1 架构

```
┌─────────────────────────────────────────────────┐
│                 Core3566 (RK3566)                 │
│                                                   │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ BlueZ    │  │ AP6256   │  │ Mali-G52      │  │
│  │ BLE+RFCOMM│  │ 5GHz AP  │  │ H.264 VDPU    │  │
│  └────┬─────┘  └────┬─────┘  └───────┬───────┘  │
│       │             │               │           │
│  ┌────▼─────────────▼───────────────▼───────┐   │
│  │          iAP2 + CarPlay 协议栈            │   │
│  │  (Python→C++ 移植 + Qt6/QML 集成)        │   │
│  └────────────────┬─────────────────────────┘   │
│                   │                              │
│  ┌────────────────▼─────────────────────────┐   │
│  │  Qt6/QML UI (520×280 CarPlay 区域)       │   │
│  │  + PulseAudio 音频路由                    │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
         ▲                              │
         │ WiFi 5GHz / Bluetooth        │ 触摸坐标
         │                              ▼
    ┌────────┐                   ┌──────────┐
    │ iPhone │                   │ 触摸屏    │
    └────────┘                   └──────────┘
```

### 4.2 移植评估

| 组件 | 来源 | 源语言 | 目标语言 | 难度 | 估计工时 |
|------|------|:---:|:---:|:---:|:---:|
| iAP2 链路层 | HaToan | Python | C++ | ⭐⭐ | 2 周 |
| iAP2 CSM 会话层 | HaToan | Python | C++ | ⭐⭐ | 2 周 |
| MFi 软件认证 | HaToan 框架 | Python | C++ | ⭐ | 1 周 |
| CarPlay 接收器 | 45clouds | C | C++ | ⭐⭐ | 3 周 |
| H.264 硬解码 | Mali-G52 VDPU | - | GStreamer | ⚠️ 待验证 | 1 周 |
| Qt6 UI 集成 | 自研 | - | C++/QML | ⭐⭐⭐ | 2 周 |
| PulseAudio 路由 | 自研 | - | C++ | ⭐ | 1 周 |
| **合计** | | | | | **12 周** |

### 4.3 硬件匹配度

| 需求 | Core3566 能力 | 状态 |
|------|-------------|:---:|
| Bluetooth BLE + RFCOMM | AP6256, BlueZ | ✅ |
| 5GHz WiFi AP 模式 | AP6256, hostapd | ✅ |
| H.264 硬解码 | Mali-G52, 需验证 VDPU 节点 | ⚠️ |
| OpenGL ES 渲染 | Mali-G52, ES 3.2 | ✅ |
| USB Host (有线 CarPlay) | xHCI | ✅ |
| 2GB RAM | CarPlay 峰值 ~500MB 估算 | ✅ |
| Debian 10, kernel 4.19 | USB/RFCOMM 驱动成熟 | ✅ |

### 4.4 关键风险

1. **MFi 凭证获取**：需要从 MFi 设备提取 Apple 签发的证书+私钥（唯一的外部依赖）
2. **H.264 硬解路径**：需验证 `/dev/video*` 是否存在 H.264 解码器节点 (`v4l2-ctl --list-formats`)
3. **iOS 版本兼容性**：Apple 可能在 iOS 更新中改变协议细节

---

## 5. Carlinkit Dongle 硬件参考

市面上 ¥30-80 的无线 CarPlay USB dongle 内部：

| 组件 | 型号 |
|------|------|
| SoC | Freescale i.MX6 UltraLite (Cortex-A7) |
| Flash | Macronix 16MB SPI NOR |
| WiFi/BT | RTL8822BS / RTL8822CS / RTL8733BS |

这些 dongle 的 BOM 成本不到 ¥25，运行着被广泛克隆的固件。MFi 凭证存储在 I2C 地址 **0x20**（CP 2.0C 规格书确认，RST=GND 时）或 0x10（旧版 CP 2.0）的认证芯片中。

**逆向工程参考**：
- `ludwig-v/wireless-carplay-dongle-reverse-engineering` (GPL-3.0)
- `lvalen91/CPC200-CCPA-Firmware-Dump`
- `Veyron2K/Carlinkit-CPC200-Autokit-Reverse-Engineering`

---

## 6. 推荐实施路径

### Phase 8a: 快速原型验证（2 周）

1. 购买 Carlinkit CPC200-CCPA dongle (~¥50)
2. 用 `pycarplay` 或 `node-CarPlay` 在 Core3566 上读取 Dongle 的 USB 输出
3. 验证 H.264 视频能通过 Mali-G52 硬解并在 Qt6/QML 区域显示
4. 验证触摸事件能通过 USB HID 正确回传

### Phase 8b: 纯软件方案（10 周）

1. 从 Dongle 提取 MFi 凭证（或从其他 MFi 设备）
2. 将 HaToan iAP2 协议栈从 Python 移植到 C++
3. 集成到 Qt6/QML UI 框架
4. 配置 AP6256 5GHz AP 模式
5. 实现完整的无线 CarPlay 连接 → 视频 → 触控链路

### Phase 8c: 移除 Dongle（如果 8b 成功）

纯软件方案验证通过后，不再需要物理 dongle。

---

## 7. 参考资料

- **iAP2 协议详解**: https://wiomoc.de/misc/posts/mfi_iap.html
- **Wireless CarPlay 协议**: https://deepwiki.com/signalius/WirelessCarPlay/3.2-iap2-protocol
- **CarPlay 攻击面分析**: https://www.oligo.security/blog/pwn-my-ride-exploring-the-carplay-attack-surface
- **Linux 内核 CarPlay gadget 驱动**: `drivers/usb/gadget/carplay.c` (Parrot)
- **iAP2 协议 PDF**: https://github.com/45clouds/WirelessCarPlay (iap2.pdf, carplay.pdf)

---

## 8. 调查结论 — 45clouds/WirelessCarPlay（2026-06-11）

### 8.1 仓库状态

| 维度 | 详情 |
|------|------|
| 作者 | MikeBravo (45clouds) |
| 语言 | C 96.1%（Apple MFi SDK 派生，版本 320.17） |
| 许可证 | MIT |
| 社区 | 1k+ Stars, 177 Forks，但**无人确认成功跑通过** |
| 活跃度 | ❌ 最后提交 2 年前，无 Release，无 Package |
| 目标硬件 | Raspberry Pi 3 (armv7l, Debian 10 Buster) |

### 8.2 实现方式

核心思路：Raspberry Pi 作为"无线桥接器"插入车机 USB 口，iPhone 通过 BLE + WiFi 连接 Pi，Pi 透传数据给车机。

```
iPhone ──BLE + 5GHz WiFi──► RPi ──USB──► 车机 (CarPlay USB 口)
```

源码关键组件：

| 组件 | 文件 | 功能 |
|------|------|------|
| AirPlay 服务端 | `AirPlayReceiverServer.c` | Bonjour 广播 `_carplay-ctrl._tcp` |
| 会话管理 | `AirPlayReceiverSession.c` | H.264 视频流 + LPCM 音频 |
| CarPlay 控制 | `CarPlayControlClient.c` | HTTP `/ctrl-int/1/connect` 发起连接 |
| MFi 认证 | `MFiSAP.c`, `MFiServerPlatformLinux.c` | I2C 0x11 读取物理 MFi 芯片 |
| HID 输入 | `AppleCarPlay_AppStub.c` | 触摸/旋钮事件回传 |

### 8.3 为什么不采用此方案

| 问题 | 严重度 | 说明 |
|------|:---:|------|
| **无人成功** | 🔴 | Issues #22/#23/#24/#27/#28 全部问"有人跑通了吗"，0 确认 |
| **WiFi 音频流断裂** | 🔴 | Issue #21: "old version sdk no appear main audio stream under wifi" |
| **编译地狱** | 🔴 | 依赖链极深（mDNSResponder + AccessorySDK + 加密库），7 年无编译指南 |
| **架构不匹配** | 🟡 | armv7l → Core3566 aarch64 需大量改造 |
| **MFi 芯片硬依赖** | 🟡 | 软件模拟不存在，但可通过外部芯片解决 |

### 8.4 仅有的参考价值

- Apple 官方 SDK 的 API 调用方式、Bonjour 服务类型（`_carplay-ctrl._tcp`）
- CarPlay 控制命令格式（HTTP GET `/ctrl-int/1/connect`）
- Apple MFi 认证文档（`Accessory Authentication.pdf` 等 PDF）

### 8.5 选型结论

**主方案**: `HaToan/carplay-wifi-extractor` (Python) + 真 MFi 芯片 (I2C) → 逐步移植 C++

**不采用**: `45clouds/WirelessCarPlay` 直接运行（编译不可行、功能不完整）

---

## 9. MFi 芯片 (MFI337S3959) 接线参考

> **来源**: Apple 官方规格书 `3530714577MFI337S3959规格书.pdf` (2026-06-15 获取)
> **芯片型号**: iPod Authentication Coprocessor **2.0C** (DeviceVersion = 0x05)

### 9.1 与早期逆向分析的差异 ⚠️

wiomoc.de 逆向的是旧版 **CP 2.0**（2018 年左右的芯片），MFI337S3959 是 **CP 2.0C**，存在以下关键差异：

| 维度 | CP 2.0 (wiomoc.de 逆向) | CP 2.0C (MFI337S3959 规格书) |
|------|:---:|:---:|
| **I2C 地址** | 0x10 | **0x20/0x21** (RST=GND) 或 0x22/0x23 (RST=VCC) |
| **RST 引脚** | 未提及 | **必须连接**，决定 I2C 地址 |
| **证书存储** | 0x31 连续读取 | 0x31–0x3A 分 10 页，每页 128 bytes |
| **认证控制位** | 简单 bit 0 | PROC_CONTROL 3-bit [2:0], 多操作模式 |
| **自检功能** | 无 | 0x40 自检控制 |
| **SEC 计数器** | 无 | 0x4D 系统事件计数器（断电前需等归零） |

### 9.2 芯片规格

| 属性 | 值 |
|------|------|
| 封装 | **QFN8**（也常见于 DFN8 标注） |
| 典型尺寸 | **2mm × 2mm** 或 2mm × 3mm |
| 引脚间距 (pitch) | **0.5mm**（最常见） |
| **I2C 写地址** | **0x20** (RST 接 GND) 或 **0x22** (RST 接 VCC) |
| **I2C 读地址** | **0x21** (RST 接 GND) 或 **0x23** (RST 接 VCC) |
| 工作电压 | **3.3V**（Core3566 I2C 默认 3.3V 直接匹配） |
| 签名算法 | RSA-1024 + SHA-1 |
| 证书格式 | PKCS#7 包裹的 X.509 DER，≤ 1280 bytes |
| DeviceVersion | 0x05 |
| 功耗 | 自动进入 Sleep 模式，I2C 通信时自动唤醒 |

### 9.3 官方引脚定义（规格书 Table 2-1, Figure 2-1）

```
       QFN8 顶视图 (Pin 1 在左下角，逆时针编号)
       
        Pin 8     Pin 7     Pin 6     Pin 5
        ┌─────┬─────┬─────┬─────┐
        │ VCC │ RST │ SCL │ NC  │
        │  ⑧  │  ⑦  │  ⑥  │  ⑤  │
        │     │     │     │     │
        │ GND │ SDA │ NC  │ NC  │
        │  ①  │  ②  │  ③  │  ④  │
        └─────┴─────┴─────┴─────┘
        Pin 1     Pin 2     Pin 3     Pin 4
```

| Pin | 信号名 | I/O | 描述 |
|:---:|--------|:---:|------|
| 1 | **GND** | — | 电源负极端子 |
| 2 | **SDA** | I/O | I2C 串行数据 |
| 3 | **NC** | — | 不可连接 (Not Connected) |
| 4 | **NC** | — | 不可连接 |
| 5 | **NC** | — | 不可连接 |
| 6 | **SCL** | I | I2C 串行时钟 |
| 7 | **RST** | I | 复位时选 I2C 地址；运行时做 warm reset |
| 8 | **VCC** | — | 电源正极端子 (3.3V) |

### 9.4 参考电路（规格书 Figure 2-2）

```
                VCC (3.3V)
                  │
    ┌─────────────┼─────────────┐
    │             │             │
    │   ┌──── 2.2kΩ ──── SDA (Pin 2)
    │   │                       │
    │   ├──── 2.2kΩ ──── SCL (Pin 6)
    │   │                       │
    │   ├──── 0.1µF ──── GND    (VCC 去耦电容)
    │   │
    │   │            RST (Pin 7) ──── GND  → 地址 0x20/0x21
    │   │                       或  ──── VCC → 地址 0x22/0x23
    │   │
    │   │            NC  (Pin 3/4/5) ──── 悬空，不可连接
    │   │
    │   └──────────── GND (Pin 1) ──── 电源地
    │
    └──────────── VCC (Pin 8) ──── 3.3V
```

> ⚠️ 规格书推荐上拉电阻 **2.2kΩ**（非 4.7kΩ），去耦电容 **0.1µF**。

### 9.5 测试座接线（推荐 RST 接 GND）

```
测试座排针    →   Core3566
──────────────────────────────
Pin 1  GND   →   GND
Pin 2  SDA   →   I2C3_SDA  + 2.2kΩ 上拉到 3.3V
Pin 3  NC    →   悬空（不可连接任何东西）
Pin 4  NC    →   悬空
Pin 5  NC    →   悬空
Pin 6  SCL   →   I2C3_SCL  + 2.2kΩ 上拉到 3.3V
Pin 7  RST   →   GND（固定地址 0x20/0x21）
Pin 8  VCC   →   3.3V  + 0.1µF 去耦电容到 GND
```

### 9.6 上电时序（规格书要求）

```
1. VCC 上电，上升沿 < 200µs (从 0.4V → 90% VCC)
2. RST 保持目标电平 (GND = 0x20, VCC = 0x22)
3. SDA/SCL 保持高电平
4. ⏱ 等待 t_STARTUP ≥ 10ms
5. 开始第一次 I2C 传输
6. ⏱ 如需后续 warm reset: 第一次传输后 ≥ 1ms 再改变 RST
   如不需 warm reset: RST 直接固定接 GND 或 VCC 即可
```

> 如果 RST 接 GND 固定，跳过步骤 6，电路最简单。

### 9.7 Warm Reset 时序（可选）

如果需要在不断电情况下重置芯片：

```
1. SDA/SCL 拉高，停止 I2C 通信
2. RST 拉低，保持 ≥ 10µs
3. ⏱ 从 RST 下降沿起，等待 t_STARTUP ≥ 10ms
4. 开始新的 I2C 通信
```

> 常规使用不需要 warm reset，RST 直接固定电平即可。

### 9.8 I2C 寄存器完整映射（规格书版）

```
I2C 地址: 0x20 (写) / 0x21 (读)，RST 接 GND

┌────────┬──────┬──────┬──────────────────────────────────────────┐
│ 寄存器  │ 块   │ 访问  │ 描述                                      │
├────────┼──────┼──────┼──────────────────────────────────────────┤
│ 0x00   │ 0    │ R    │ DeviceVersion = 0x05 (2.0C)               │
│ 0x01   │ 0    │ R    │ FirmwareVersion                           │
│ 0x02   │ 0    │ R    │ Auth Protocol Major Version                │
│ 0x03   │ 0    │ R    │ Auth Protocol Minor Version                │
│ 0x04   │ 0    │ R    │ DeviceID (4 bytes, 大端)                   │
│ 0x05   │ 0    │ R    │ ErrorCode (1 byte, 读后清零)                │
├────────┼──────┼──────┼──────────────────────────────────────────┤
│ 0x10   │ 1    │ R/W  │ 认证控制/状态 (1 byte)                      │
│        │      │  W   │   PROC_CONTROL [2:0]:                      │
│        │      │      │     0 = NOP                                │
│        │      │      │     1 = 生成签名 (Generate Signature)       │
│        │      │      │     2 = 生成 Challenge (Generate Challenge) │
│        │      │      │     3 = 验证签名 (Verify Signature)         │
│        │      │      │     4 = 验证证书 (Validate Certificate)     │
│        │      │  R   │   bit 7: ERR_SET (1=有错误)                │
│        │      │      │   bit [3:0]: PROC_RESULTS (1=签名完成等)    │
│ 0x11   │ 1    │ R/W  │ 签名数据长度 (2 bytes, max 0x80)            │
│ 0x12   │ 1    │ R/W  │ 签名数据 (128 bytes)                        │
├────────┼──────┼──────┼──────────────────────────────────────────┤
│ 0x20   │ 2    │ R/W  │ Challenge 长度 (2 bytes, 1–128)            │
│ 0x21   │ 2    │ R/W  │ Challenge 数据                             │
├────────┼──────┼──────┼──────────────────────────────────────────┤
│ 0x30   │ 3    │ R    │ Accessory 证书长度 (2 bytes, ≤1280)        │
│ 0x31   │ 3    │ R    │ 证书 Page 1 (128 bytes)                    │
│ 0x32   │ 3    │ R    │ 证书 Page 2                                │
│  ...   │ 3    │ R    │ ...                                        │
│ 0x3A   │ 3    │ R    │ 证书 Page 10                               │
│        │      │      │ 可连续读取，从 0x31 开始顺序读              │
├────────┼──────┼──────┼──────────────────────────────────────────┤
│ 0x40   │ 4    │ W    │ 自检控制 (写 1 = 运行证书+私钥测试)         │
│        │      │ R    │   bit 7: 证书存在, bit 6: 私钥存在          │
│        │      │      │   读后自动清零                              │
│ 0x4D   │ 4    │ R    │ 系统事件计数器 SEC (每秒递减，到 0 停)       │
│        │      │      │  断电前必须等待 SEC = 0                     │
├────────┼──────┼──────┼──────────────────────────────────────────┤
│ 0x50   │ 5    │ R/W  │ Apple Device 证书长度 (≤1024 bytes)         │
│ 0x51–  │ 5    │ R/W  │ Apple Device 证书 Pages (128 bytes/page)   │
│ 0x58   │      │      │ (用于 accessory 认证 iPhone 场景)           │
└────────┴──────┴──────┴──────────────────────────────────────────┘
```

### 9.9 签名流程（规格书版）

这是 iPhone 认证配件时的核心流程：

```
步骤 1: 读取设备信息
  Read 0x00 → DeviceVersion (应为 0x05)
  Read 0x04 → DeviceID (4 bytes)

步骤 2: 读取 Accessory 证书
  Read 0x30 → 证书长度 (2 bytes, Big-Endian)
  Read 0x31 → 证书 Page 1 (128 bytes)
  Read 0x32 → 证书 Page 2
  ... 继续读取直到覆盖证书长度

步骤 3: 写入 Challenge（来自 iPhone 的 20 bytes 随机数）
  Write 0x20 → [0x00, 0x14] (长度 = 20, Big-Endian)
  Write 0x21 → challenge 数据 (20 bytes)

步骤 4: 启动签名
  Write 0x10 → 0x01 (PROC_CONTROL = 1)

步骤 5: 等待完成 (轮询)
  Read 0x10 → 检查 PROC_RESULTS [3:0] == 1 且 ERR_SET == 0

步骤 6: 读取签名结果
  Read 0x11 → 签名实际长度 (2 bytes)
  Read 0x12 → 签名数据 (根据长度读取)
```

### 9.10 断电注意事项

```
系统事件计数器 (SEC) 在芯片通电后每秒递减 1。
断电前必须：
  1. Read 0x4D → 读取 SEC 值
  2. 如果 SEC > 0，等待其递减到 0
  3. SEC == 0 时方可移除 VCC 电源
```

### 9.11 需要修改的代码

`carplay-wifi-extractor/mfi_auth_coprocessor.py` 关键改动：

```python
# 地址修改 (CP 2.0C)
DEV_ADDR = 0x20  # 不是 0x10！RST 接 GND 时

# 证书读取：分页读取 0x31-0x3A，不是一次性从 0x31 读全部
def read_certificate():
    total_len = Word.unpack(_read_i2c(0x30, 2))[0]
    cert_data = b""
    for page in range(10):
        addr = 0x31 + page
        cert_data += _read_i2c(addr, 128)
    return cert_data[:total_len]

# 认证控制：PROC_CONTROL 3-bit 域
def generate_challenge_response(challenge):
    _write_i2c(0x20, Word.pack(len(challenge)))
    _write_i2c(0x21, challenge)
    _bus.write_byte_data(DEV_ADDR, 0x10, 0x01)  # PROC_CONTROL=1
    # 轮询 PROC_RESULTS==1, ERR_SET==0
    ...

# 上电时需读取 SEC 并在断电前等待
def check_sec():
    return _bus.read_byte_data(DEV_ADDR, 0x4D)
```

### 9.12 验证步骤

```bash
# 1. 扫描 I2C 总线，确认芯片在线
sudo i2cdetect -y 3          # 应看到地址 0x20 响应 (不是 0x10!)

# 2. 读取 DeviceVersion (应返回 0x05)
i2cget -y 3 0x20 0x00

# 3. 读取证书长度
i2cget -y 3 0x20 0x30 w      # 2 bytes, Big-Endian

# 4. 自检
i2cset -y 3 0x20 0x40 0x01   # 启动自检
sleep 0.1
i2cget -y 3 0x20 0x40        # 读结果: bit7=证书, bit6=私钥
```

### 9.13 软件模拟 vs 真芯片

| 维度 | 软件模拟 (`IAP2_EMULATE_MFI=1`) | 真芯片 CP 2.0C (`IAP2_EMULATE_MFI=0`) |
|------|:---:|:---:|
| iPhone 认证 | ❌ 证书链验证失败 | ✅ Apple 签发 X.509 证书可过 |
| 签名算法 | SHA-256 哈希（假的） | RSA-1024 + SHA-1（芯片内部私钥） |
| 适用场景 | 协议调试、单元测试 | **生产环境、真 iPhone 连接** |
| I2C 地址 | 无 | **0x20/0x21** (RST=GND) |
| 依赖 | 无 | I2C 硬件 + MFI337S3959 + RST 引脚
---

## 10. 下一步计划

> 状态：芯片 + 测试座已到货，规格书已分析完毕（2026-06-15）

### Step 1: 硬件验证（预计 1-2 小时）

```
□ 1.1 将 MFI337S3959 放入 QFN8 测试座，确认方向（Pin 1 标记对齐）
□ 1.2 按 9.5 节接线表连接 Core3566（5 根有效线 + 2 个上拉电阻 + 1 个去耦电容）
□ 1.3 上电前用万用表二极管档验证 VCC-GND 之间无短路
□ 1.4 上电，运行 i2cdetect -y <总线号>，确认 0x20 地址响应
□ 1.5 读取 0x00 (DeviceVersion)，应返回 0x05
```

### Step 2: 芯片功能验证（预计 1 小时）

```
□ 2.1 运行自检: i2cset 0x40=0x01 → i2cget 0x40，确认 bit7=1 (证书存在)
□ 2.2 读取证书长度: i2cget 0x30 (2 bytes)
□ 2.3 逐页读取证书 (0x31-0x3A)，保存为 DER 文件
□ 2.4 用 openssl x509 -in cert.der -inform DER -text 查看证书信息
□ 2.5 写入 Challenge + 启动签名 (PROC_CONTROL=1)，验证签名结果非空
```

### Step 3: 修改 carplay-wifi-extractor（预计 2 小时）

```
□ 3.1 修改 mfi_auth_coprocessor.py:
       - DEV_ADDR = 0x20
       - I2C 总线号改为 Core3566 实际总线
       - 证书读取改为分页模式 (0x31-0x3A)
       - 签名控制适配 PROC_CONTROL 3-bit 位域
□ 3.2 设置 IAP2_EMULATE_MFI=0 运行配件模拟服务器
□ 3.3 用 Client 模式测试：验证 iPhone 客户端能否完成认证握手
```

### Step 4: CarPlay 无线连接联调（预计 1-2 天）

```
□ 4.1 配置 Core3566 AP6256 5GHz AP 模式 (hostapd)
□ 4.2 配置 BlueZ BLE 广播 (iAP2 UUID: 00000000-deca-fade-deca-deafdecacaff)
□ 4.3 启动 carplay-wifi-extractor server 模式
□ 4.4 iPhone 搜索蓝牙 → 连接 → WiFi 凭证交换 → AirPlay 会话建立
□ 4.5 验证 H.264 视频流 + 触控回传
```

### 风险点

| 风险 | 缓解 |
|------|------|
| 测试座接触不良 | 用万用表从测试座排针端测量芯片引脚间的二极管特性确认接触 |
| I2C 时序不匹配 | 先用 GPIO 模拟 I2C (`i2c-gpio,delay_us=50`)，调通后再尝试硬件 I2C |
| 芯片证书过期/被吊销 | 证书由 Apple CA 签发，通常有效期很长；自检通过即可确认 |
| carplay-wifi-extractor 的蓝牙部分依赖 BlueZ 版本 | Core3566 用 Debian 10，BlueZ 5.50+，应兼容 |