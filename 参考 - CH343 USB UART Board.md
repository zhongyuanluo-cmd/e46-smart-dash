# 参考 - CH343 USB UART Board

> 来源：https://www.waveshare.net/wiki/CH343_USB_UART_Board
> 芯片：WCH CH343

## 规格

| 参数 | 值 |
|------|-----|
| 类型 | USB 转 TTL 串口 |
| 供电 | 5V（USB） |
| 波特率 | 50bps ～ 6Mbps |
| TTL 电平 | 5V / 3.3V（跳线帽选择）/ 2.5V / 1.8V（外部输入） |
| 系统支持 | Windows / Linux / Mac / Android |
| 驱动 | CDC 免驱（Linux/Mac），Windows 可选 VCP |

## 支持的波特率

50, 75, 100, 110, 134.5, 150, 300, 600, 900, 1200, 1800, 2400, 3600, 4800, 9600, 14400, 19200, 28800, 33600, 38400, 56000, 57600, 76800, **115200**, 128000, 153600, 230400, 256000, 307200, 460800, **921600**, 1M, 1.5M, 2M, 3M, 4M, 6M

> ⚠️ 注意：**不支持 250000 bps**（非标准波特率）

## 接口定义

| 引脚 | 功能 | 方向 |
|------|------|------|
| VCC | 对外供电 5V/3.3V（可设） | OUT |
| 5V | 对外供电 5V | OUT |
| 3V3 | 对外供电 3.3V | OUT |
| GND | 地 | — |
| TXD | 接 MCU.RXD | OUT |
| RXD | 接 MCU.TXD | IN |
| RTS# | 接 MCU.CTS | OUT |
| CTS# | 接 MCU.RTS | IN |
| RI# | 振铃指示 | IN |
| DCD# | 载波检测 | IN |
| DTR# | 数据终端就绪 | OUT |
| DSR# | 数据装置就绪 | IN |

- 电平选择：跳线帽短接 = 板载 5V/3.3V；拔掉跳线帽 = 外部 VCC 输入（如 1.8V/2.5V）

## 使用方法

### Linux
免驱，插上即用。设备名：`/dev/ttyACM0`（或 `ttyACM1`...）
```bash
ls /dev/tty*
minicom -D /dev/ttyACM0
```

### Windows
- CDC 驱动（默认）：设备管理器显示为 `USB Serial Device`
- VCP 驱动（可选）：需安装 `CH343SER.7z`，显示为 `COMx`

### macOS
需安装驱动 `CH34XSER_MAC.7z`

## 在本项目中的用途

Core3566 调试串口（UART2），用于：
- 查看 U-Boot / Kernel 启动日志
- 访问串口终端
- 1500000 8N1

> Rockchip 调试串口默认使用 1.5M 波特率，不是常见的 115200。

接线：
```
CH343          Core3566（40PIN GPIO）
GND    ───     PIN 6  (GND)
TXD    ───     PIN 8  (UART2_TXD / GPIO14)
RXD    ───     PIN 10 (UART2_RXD / GPIO15)
```

> 只需要 GND + TXD + RXD 三根线

## 资料文件

已下载至 `参考资料文档/`：
- 原理图: `CH343_USB_UART_Board_sch.pdf`
- CH343 数据手册: `CH343DS1.PDF`
