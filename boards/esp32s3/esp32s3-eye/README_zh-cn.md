# ESP32-S3-EYE 开发板对 openvela 的支持

[ [English](README.md) | 简体中文 ]

## 简介

本目录为 **乐鑫 ESP32-S3-EYE** 开发板提供 openvela 支持，基于
`dev-ai-contest-2026` 分支。

开发板硬件细节、原理图和官方上手指南请参考乐鑫官方文档：


- [ESP32-S3-EYE 上手指南（esp-who）](https://github.com/espressif/esp-who/blob/master/docs/en/get-started/ESP32-S3-EYE_Getting_Started_Guide.md)

> ⚠️ **分支依赖**
>
> 本板适配仅在 `open-vela/nuttx` 与 `open-vela/vendor_espressif` 的
> `dev-ai-contest-2026` 分支上可编译。`trunk` 或 `dev` 分支由于尚未合入
> 芯片层依赖，无法编译。

## 目录结构

```
vendor/espressif/boards/esp32s3/esp32s3-eye/
├── Kconfig                 # 板级 Kconfig 选项
├── include/board.h         # 时钟 / LED / GPIO 板级定义
├── src/                    # 板级 bring-up 源码
│   ├── esp32s3_appinit.c
│   ├── esp32s3_boot.c
│   ├── esp32s3_bringup.c
│   ├── esp32s3_board_camera.c
│   ├── esp32s3_board_lcd.c
│   ├── esp32s3_board_spi.c
│   └── ...
├── configs/openvela/       # 全功能 defconfig
└── scripts/                # 链接脚本 + esp-hal-3rdparty 补丁
```

## 支持的外设

| 外设 | 驱动 | 设备节点 |
|------|------|----------|
| OV2640 摄像头（DVP，QVGA RGB565） | `esp32s3_cam` + V4L2 | `/dev/video0` |
| ST7789 LCD（1.3" 240×240 SPI）    | `esp32s3_board_lcd`  | `/dev/lcd0`   |
| MSM261S4030H0 PDM 麦克风          | I2S0 RX              | `/dev/audio/pcm_in0` |
| microSD（1-bit SDIO）             | `esp32s3_sdmmc`      | `/dev/mmcsd1` |
| QMA7981 三轴加速度计              | `qma7981`（I2C0 @0x12）| `/dev/accel0` |
| Wi-Fi 4 (b/g/n) STA               | `esp_wifi`           | `wlan0`       |
| BLE 5 LE                          | `esp32s3_ble_adapter` + NimBLE 主机 | `bnep0` |
| BOOT 按键                         | `userbuttons`        | `/dev/buttons` |
| Power LED                         | `userleds`           | `/dev/userleds` |

## GPIO 引脚映射（openvela defconfig）

| 功能 | GPIO |
|------|------|
| BOOT 按键 | GPIO0 |
| Power LED | GPIO3 |
| I2C0 SDA / SCL | GPIO4 / GPIO5 |
| I2S0 DIN / BCLK / WS | GPIO2 / GPIO41 / GPIO42 |
| 摄像头 XCLK | GPIO15（LEDC，20 MHz）|
| LCD DC / BL | GPIO43 / GPIO48 |
| SDMMC CMD   | GPIO38 |

OV2640 DVP 数据脚、LCD SPI 信号、SD-MMC 数据脚遵循乐鑫官方
ESP32-S3-EYE 原理图 v2.2，由芯片层驱动和 `src/` 下板级 glue 配置。

## 编译

openvela 仓库提供的 `build.sh` 脚本是唯一入口：

```bash
cd openvela
./build.sh \
    $(pwd)/vendor/espressif/boards/esp32s3/esp32s3-eye/configs/openvela \
    -j$(nproc)
```

成功时输出末尾会出现：

```
LD: nuttx
MKIMAGE: ESP32-S3 binary
Successfully created ESP32S3 image.
Generated: nuttx.bin
```

`nuttx/` 目录下生成的产物：

| 文件 | 大小 | 用途 |
|------|----:|------|
| `nuttx`     | 约 21 MB | ELF，供 GDB 使用 |
| `nuttx.bin` |  约 1.6 MB | 平面镜像，烧录到偏移 `0x0` |
| `nuttx.hex` |  约 4 MB | Intel-HEX（可选）|

> 如果执行过 `make distclean`，下次运行 `build.sh` 会自动重新应用
> `scripts/patches/0001-esp-hal-3rdparty-fix-spinlock-init.patch`。
> `distclean` 后绕过 `build.sh` 直接编译会因 `spinlock_init` 重复定义而
> 链接失败。

## 烧录

本板使用 **simple-boot** 方案：单个平面镜像烧录到 flash 偏移 `0x0`，
不需要单独的 bootloader 或分区表。

```bash
PORT=$(ls /dev/serial/by-id/ | grep Espressif_USB_JTAG_serial_debug_unit | head -1)
PORT="/dev/serial/by-id/$PORT"

# 可选：仅在切换不同固件家族时使用。
esptool --chip esp32s3 --port "$PORT" erase-flash

esptool --chip esp32s3 --port "$PORT" --baud 460800 \
        --before default-reset --after hard-reset \
        write-flash 0x0 nuttx/nuttx.bin
```

如果 `esptool` 连不上，按住 **BOOT** 后按一下 **RESET** 进入 ROM 下载
模式，然后释放 BOOT。

## 首次启动 & 快速验证

打开 USB-CDC 串口（115200 8N1）：

```bash
picocom -b 115200 /dev/ttyACM*
# 或: minicom -D /dev/ttyACM2 -b 115200 -8 -o
```

按板上 **RESET** 键，应当看到 NuttX 启动，`ls /dev` 列出所有外设：

```
nsh> uname -a
NuttX  0.0.0 <commit> <date> xtensa esp32s3-eye
nsh> ls /dev
/dev:
 accel0   audio/   buttons   console   i2c0   mmcsd1
 null     random   timer0    ttyACM0   userleds   video0   zero
```

各外设手动验证命令（全部在 `nsh>` 下执行）：

```
i2c dev 0x03 0x77        # OV2640 应当在 0x30 处 ACK
camera -h                # QVGA RGB565 视频流到 LCD
lvgldemo                 # ST7789 LVGL 演示
mount -t vfat /dev/mmcsd1 /mnt
ifconfig wlan0           # Wi-Fi MAC + IP
bt bnep0 info            # BDAddr + ACL 信息
buttons 1                # BOOT 按键事件
leds 1                   # 闪烁 power LED
ostest                   # 内核回归测试套件
```

Wi-Fi STA 模式连接示例：

```
wapi mode wlan0 2
wapi psk wlan0 <你的密码> 3 wpa
wapi essid wlan0 <你的SSID> 1
renew wlan0
ping -c 4 8.8.8.8
```

## 自定义配置

```bash
cd openvela/nuttx
make menuconfig          # 交互式调整
make savedefconfig
cp defconfig \
   ../vendor/espressif/boards/esp32s3/esp32s3-eye/configs/openvela/defconfig
```

如需新建配置变体（例如 `nsh-only`），把 `configs/openvela/` 目录复制为
`configs/<新名字>/`，然后把新路径传给 `build.sh`。

## 调试

- **串口日志** —— `syslog`/`printf` 输出到 USB-CDC，用 `picocom`/`tio`
  抓取。
- **USB-JTAG GDB** —— 同一个 USB 接口暴露 JTAG TAP，用
  `openocd -f board/esp32s3-builtin.cfg` 然后
  `xtensa-esp32s3-elf-gdb nuttx/nuttx -ex 'target remote :3333'`。
- **崩溃分析** —— 保存完整的寄存器 / 堆栈 dump，喂给
  `tools/scripts/decode_backtrace.py` 和 `nuttx/nuttx`。

## 已知限制

1. `fb` builtin 报 `Failed to open /dev/fb0: 2`；ST7789 注册为
   `/dev/lcd0` 而不是 framebuffer，请改用 `lvgldemo`。
2. `bt bnep0 scan get` 在某些 peer 组合下可能 hard-fault —— 这是上游
   NuttX BLE-GATT ENOTCONN 问题，根因是 LE Privacy 处理。HCI 连接 /
   配对不受影响，修复将作为独立的上游 PR 提交。
3. `/dev/uorb` 暂为空 —— QMA7981 当前直接注册为 `/dev/accel0`，
   uORB 集成排在 v2。
4. microSD 锁定 1-bit SDIO 模式；4-bit 支持在后续补丁中。

## 许可协议

本目录下所有文件均使用 Apache-2.0 协议（SPDX 标识符
`Apache-2.0`）；详见各文件头部声明。
