# 参考 - 7inch DSI LCD 屏幕

> 来源：https://www.waveshare.net/wiki/7inch_DSI_LCD
> 型号：Waveshare 7inch DSI LCD (C)

## 规格

| 参数 | 值 |
|------|-----|
| 尺寸 | 7 寸 |
| 分辨率 | 800 × 480 |
| 触摸 | 电容式 5 点触控 |
| 面板 | 钢化玻璃，硬度 6H |
| 接口 | MIPI DSI（15-pin FPC） |
| 刷新率 | 60Hz |
| 功耗 | 5V / 500mA |
| 背光 | 软件可调 |

> ✅ **官方确认支持 Core3566**：Debian/Ubuntu，五点触控，免驱

## 接口定义（15-pin FPC）

| PIN | 信号 | PIN | 信号 |
|-----|------|-----|------|
| 1 | GND | 9 | DSI1_DP0 |
| 2 | DSI1_DN1 | 10 | GND |
| 3 | DSI1_DP1 | 11 | SCL0 |
| 4 | GND | 12 | SDA0 |
| 5 | DSI1_CN | 13 | GND |
| 6 | DSI1_CP | 14 | 3V3 |
| 7 | GND | 15 | 3V3 |
| 8 | DSI1_DN0 | | |

- 触摸 I2C：SCL0/SDA0（同 I2C-10）
- 额外 2-pin 电源线：从 40PIN GPIO 取电（5V + GND）

## 硬件连接（搭配 CM4 / Core3566）

1. 使用 DSI-Cable-12cm（22-pin → 15-pin FPC 软排线），连接底板 DSI1 到屏幕
2. 2-pin 电源线从屏幕接到底板 40PIN GPIO 的 5V + GND
3. 屏幕背面可螺丝固定 Core3566 + 底板

## 软件配置

### config.txt 设置
```
# V4 底板（DSI1）
dtoverlay=vc4-kms-dsi-7inch,dsi1

# V1~V3 底板（DSI0）
# dtoverlay=vc4-kms-dsi-7inch,dsi0
```

### 背光调节
```bash
# 0~255，0=最暗，255=最亮
echo 128 | sudo tee /sys/class/backlight/*/brightness
```

微雪还提供了图形化背光调节工具（Brightness.zip）。

### 显示旋转（Bookworm/Trixie）
- GUI：Preferences → Screen Configuration → DSI-2 → Orientation → 勾选方向 → Apply
- 命令行：在 `/boot/cmdline.txt` 开头添加 `video=DSI-1:800x480M@60,rotate=90`

### 触摸旋转
创建 `/etc/udev/rules.d/99-waveshare-touch.rules`：
```
# 90°
ENV{ID_INPUT_TOUCHSCREEN}=="1", ENV{LIBINPUT_CALIBRATION_MATRIX}="0 -1 1 1 0 0"
# 180°
# ENV{ID_INPUT_TOUCHSCREEN}=="1", ENV{LIBINPUT_CALIBRATION_MATRIX}="-1 0 1 0 -1 1"
# 270°
# ENV{ID_INPUT_TOUCHSCREEN}=="1", ENV{LIBINPUT_CALIBRATION_MATRIX}="0 1 0 -1 0 1"
```

### 禁用触摸
config.txt 添加：`disable_touchscreen=1`

### 屏幕休眠
`xset dpms force off`（仅 Bullseye/Buster）

## 资料文件

已下载至 `参考资料文档/`：
- 2D 图纸: `7inch-DSI-LCD.pdf`
- 触摸图纸: `7inch-DSI-TOUCH-DS.pdf`
