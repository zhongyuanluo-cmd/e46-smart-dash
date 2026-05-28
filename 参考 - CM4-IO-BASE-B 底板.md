# 参考 - CM4-IO-BASE-B 底板

> 来源：https://www.waveshare.net/wiki/CM4-IO-BASE-B
> 搭配 Core3566（CM4 兼容）使用

## 版本

当前购买应为 **V4**（最新），关键变化：

| 版本 | 变更 |
|------|------|
| V1 | 初始版本 |
| V2 | DC-DC: TLV62130RGT → MP1658；BOOT 跳线帽改为拨码开关 |
| V3 | 修复部分 CM4 无法重启；新增 RTC 控制电路（默认不焊） |
| V4 | **修复 BT-DIS/WIFI-DIS 丝印**；**DSI 接口从 DSI0 改为 DSI1** |

## 板载资源

| 标号 | 功能 | 备注 |
|------|------|------|
| CM4 连接器 | Compute Module 4 | 兼容 Core3566 |
| USB Type C | 5V/2.5A 供电 + eMMC 烧录 | 烧录时不要接其他设备 |
| DISP0/DSI1 | MIPI DSI 接口 | V4 为 DSI1（22-pin FPC） |
| FAN | 风扇接口 | **仅支持 5V PWM 风扇**，EMC2301 控制器 |
| CAM ×2 | MIPI CSI 摄像头 | 双路 |
| HDMI0 | 标准 HDMI | 4K 30fps |
| HDMI1 | 排线 HDMI | 4K 30fps |
| USB 2.0 ×2 | 标准 USB A | 需配置 `dtoverlay=dwc2,dr_mode=host` |
| USB 2.0 排线 | 额外 USB | 通过 FE1.1S HUB（1 扩 4） |
| RJ45 | 千兆网口 | 10/100/1000M |
| M.2 | M KEY | **仅 PCIe**（NVMe 等），不支持 SATA；CM4 不支持 NVMe 启动 |
| Micro SD | TF 卡槽 | 仅 CM4 Lite 版本使用 |
| 40PIN GPIO | 树莓派标准 | 接 HAT 模块 |
| RTC | PCF85063 | I2C-10，地址 0x51 |
| BOOT | 拨码开关 | ON=USB-C 烧录，OFF=eMMC/SD 启动 |

## ⚠️ 关键注意事项

### EMC2301 风扇控制器
> **必须先连接风扇，再给主板通电！** 风扇控制芯片在无风扇负载的情况下上电会烧毁。
> 绝对不要在带电状态下插拔风扇。

- 仅支持 **5V PWM** 风扇，不支持 12V
- 风扇和 RTC 共享 I2C-10（FAN: 0x2f, RTC: 0x51）
- 内核需 ≥6.1.31，config.txt 配置：
  ```
  dtoverlay=i2c-fan,emc2301,i2c_csi_dsi,midtemp=45000,maxtemp=65000
  ```
- 验证：`i2cdetect -y 10`，生效时应显示 `UU`

### RTC 配置
```
dtparam=i2c_vc=on
dtoverlay=i2c-rtc,pcf85063a,i2c_csi_dsi
# 需要注释掉 dtparam=audio=on
```

### DSI 屏幕配置（V4 版本）
```
# /boot/firmware/config.txt
dtoverlay=vc4-kms-dsi-7inch,dsi1
```
> 使用 DSI 时，会占用 I2C-10，导致 RTC/FAN 与 DSI 共享 I2C 总线

### USB 2.0
默认禁用，需在 config.txt 添加：
```
dtoverlay=dwc2,dr_mode=host
```
新系统（2021.10.30 后）还需移除 `otg_mode=1`

### M.2 限制
- M KEY，仅 PCIe 通道
- 不支持 SATA 硬盘
- CM4 默认不支持 NVMe 启动

## 尺寸与电源
- 5V/2.5A USB-C 供电
- CM4 需稳定 5V/2A
- Type C 烧录镜像时不要连接其他设备

## 资料文件

已下载至 `参考资料文档/`：
- 原理图 V4: `CM4-IO-BASE-B_V4_SchDoc.pdf`
- CM4 核心板数据手册: `cm4-datasheet.pdf`
- 3D 文件: `CM4-IO-BASE-3D.zip`
- 40pin定义: https://pinout.xyz/