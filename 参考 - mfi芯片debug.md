# 参考 - MFi 芯片 Debug

平台: Core3566 + CM4 IO Base + Linux
芯片: MFi337S3959（iPod Authentication Coprocessor 2.0C, USON-8）
现象: I2C 扫不到设备

---

## 一、引脚与基本规格

| Pin | 名称 | 说明 |
|-----|------|------|
| 1 | GND | 电源地 |
| 2 | SDA | I2C 数据 |
| 3-5 | NC | **必须悬空，禁止连接** |
| 6 | SCL | I2C 时钟 |
| 7 | RST | 复位/地址选择（不能悬空） |
| 8 | VCC | 1.62 ~ 5.5V，3.3V 适用 |

底部 thermal pad 可悬空或接 GND。

### I2C 地址（关键）

Linux 用 **7-bit 地址**：

| RST 状态 | 7-bit 地址 | 写地址(8-bit) | 读地址(8-bit) |
|----------|-----------|--------------|--------------|
| 接 GND   | **0x10**  | 0x20 | 0x21 |
| 接 VCC   | **0x11**  | 0x22 | 0x23 |

⚠️ `i2cdetect` 中应看到 `0x10` 或 `0x11`，而不是 `0x20`。
软件中常见错误：直接写 0x20 → 永远扫不到。

### 时序与协议要点

- 上电后 SDA、SCL 必须保持高电平
- VCC 起来后 **≥ 10ms** 才能发第一笔 I2C
- VCC 必须在超过 0.4V 后 200µs 内达到目标电压的 90%
- 若 RST 由 GPIO 控制，第一笔传输后 ≥ 1ms 才能拉高
- **最大 SCL 速率 400kHz**（最低 10kHz）
- 2.0C **不做 clock stretching**，busy 时直接 NACK
- master 收到 NACK 必须 **等 500µs 后重试**
- SDA 需要 **2.2kΩ 上拉到 VCC**
- VCC-GND 间需就近放 **0.1µF 去耦电容**

---

## 二、二极管测试读数解读

万用表二极管档：Pin8(VCC)红 / Pin1(GND)黑 → 反向偏置 ESD 保护二极管。

| 读数 | 判断 |
|------|------|
| 开路 或缓慢充电 1-2V | **正常**（内部 LDO + 去耦电容充电） |
| < 0.5V 持续 | 可能短路，疑似击穿 |
| 反向（GND红/VCC黑）应读 ~0.4-0.7V | 二极管正向压降 |

仅 1-2V 读数本身**不能定罪芯片**，需结合两个方向综合判断。

---

## 三、排查清单（按优先级）

### ⭐ 1. SDA/SCL 上拉电阻
最高频问题。CM4 IO Base 的上拉**不一定连到你接的那一组 header**。

**验证方法**（断电）：万用表电阻档量 SDA 对 VCC、SCL 对 VCC，应 ~2.2kΩ（CM4 内部上拉约 1.8kΩ 也算）。
读到 ∞ 或几十 kΩ → 上拉缺失。

**修复**：SDA、SCL 各飞线 2.2kΩ 到 3.3V。

### ⭐ 2. 软件 I2C 地址用错
确认软件用的是 **0x10 / 0x11**（7-bit），不是 0x20 / 0x22。

```bash
i2cdetect -y 1     # 看是否出现 0x10 或 0x11
```

### 3. RST 引脚状态
**禁止悬空**。需接 GND（地址 0x10）或 VCC（地址 0x11）或 GPIO 控制。

### 4. CM4 I2C bus 配置
```bash
ls /dev/i2c-*
i2cdetect -l
cat /boot/config.txt | grep -i i2c
```
默认只开 i2c-1（GPIO2/3）。其他 pin 需 `dtoverlay=i2c1` 或对应 overlay。

### 5. SDA / SCL 接反
Pin2=SDA，Pin6=SCL。底座对称焊容易反，互换试。

### 6. NC 引脚（Pin3-5）被连
万用表测 Pin3、4、5 对 GND 和 VCC，应**全开路**。被短到 GND/VCC 会导致芯片异常。

### 7. NACK 单次扫描漏判
2.0C busy 时返回 NACK，`i2cdetect` 单次可能漏。
```bash
for i in $(seq 1 20); do i2cdetect -y 1 | grep -E '10|11'; sleep 0.1; done
```

### 8. SCL 速率超 400kHz
检查 `/boot/config.txt` 中 `i2c_arm_baudrate`，默认 100kHz 安全。

### 9. 上电时序
若 kernel probe 太早（VCC 上来后 < 10ms 就发 I2C），会失败。可加延迟或 deferred probe。

### 10. 去耦电容
VCC-GND 必须就近 0.1µF。

### 11. 焊接虚焊（USON-8 底部焊盘）
视觉看不出，需热风返修或换板。

### 12. 假货 / 翻新片
淘宝 MFi 芯片高发。换一颗 / 换一家供货验证。

---

## 四、终极手段：抓波形

逻辑分析仪 / 示波器接 SDA、SCL：
- 看到 START 后 SCL 是否在跑
- 第 9 个时钟周期 SDA 是被拉低（**ACK，芯片在线**）还是保持高（**NACK，无响应**）

定位结论：

| 现象 | 定位 |
|------|------|
| 波形正常但全 NACK | 地址错 / RST 状态错 / 芯片坏 |
| SDA 一直被拉低 | 总线被某设备锁死（SDA 卡死） |
| 完全无波形 | 软件没发包 / bus 没使能 |
| SCL 跑 SDA 不动 | 上拉缺失 |

---

## 五、当前进度记录

- ✅ Pin1-Pin8 实测 3.3V，供电 OK
- ✅ RST 接 GND 和 VCC 都试过，均不工作
- ✅ 测座接触良好
- 🔄 待验证：SDA/SCL 上拉电阻、软件地址（0x10/0x11）、波形
- ⚠️ 优先飞线 2.2kΩ 上拉 + 确认 7-bit 地址使用正确

---

## 六、参考文档

- `参考资料文档/3530714577MFI337S3959规格书.pdf`
- iPod Authentication Coprocessor 2.0C Specification, Apple Inc., 2011-06-22
- I2C Spec: http://www.semiconductors.philips.com/acrobat_download/literature/9398/39340011.pdf
