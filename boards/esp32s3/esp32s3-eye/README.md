# ESP32-S3-EYE Board Support for openvela

[ English | [简体中文](README_zh-cn.md) ]

## Introduction

This directory provides openvela support for the **Espressif ESP32-S3-EYE**
development board on the `dev-ai-contest-2026` branch.

For board hardware details, schematics and the official getting-started
guide, see the upstream Espressif documentation:

- [ESP32-S3-EYE product page](https://www.espressif.com/en/news/ESP32-S3-EYE)
- [ESP32-S3-EYE Getting Started Guide (esp-who)](https://github.com/espressif/esp-who/blob/master/docs/en/get-started/ESP32-S3-EYE_Getting_Started_Guide.md)

> ⚠️ **Branch dependency**
>
> This board overlay only builds on the `dev-ai-contest-2026` branch of
> `open-vela/nuttx` and `open-vela/vendor_espressif`. Building it from
> `trunk` or `dev` will fail because the chip-side dependencies are not
> yet upstream.

## Directory Structure

```
vendor/espressif/boards/esp32s3/esp32s3-eye/
├── Kconfig                 # board-level Kconfig options
├── include/board.h         # clock/LED/GPIO board definitions
├── src/                    # board bring-up sources
│   ├── esp32s3_appinit.c
│   ├── esp32s3_boot.c
│   ├── esp32s3_bringup.c
│   ├── esp32s3_board_camera.c
│   ├── esp32s3_board_lcd.c
│   ├── esp32s3_board_spi.c
│   └── ...
├── configs/openvela/       # full-feature defconfig
└── scripts/                # link script + esp-hal-3rdparty patch
```

## Supported Peripherals

| Peripheral | Driver | Device node |
|-----------|--------|-------------|
| OV2640 camera (DVP, QVGA RGB565) | `esp32s3_cam` + V4L2 | `/dev/video0` |
| ST7789 LCD (1.3" 240×240 SPI)    | `esp32s3_board_lcd`  | `/dev/lcd0`   |
| MSM261S4030H0 PDM microphone     | I2S0 RX              | `/dev/audio/pcm_in0` |
| microSD (1-bit SDIO)             | `esp32s3_sdmmc`      | `/dev/mmcsd1` |
| QMA7981 3-axis accelerometer     | `qma7981` (I2C0 @0x12) | `/dev/accel0` |
| Wi-Fi 4 (b/g/n) STA              | `esp_wifi`           | `wlan0`       |
| BLE 5 LE                         | `esp32s3_ble_adapter` + NimBLE host | `bnep0` |
| BOOT button                      | `userbuttons`        | `/dev/buttons` |
| Power LED                        | `userleds`           | `/dev/userleds` |

## GPIO Pin Map (openvela defconfig)

| Function | GPIO |
|----------|------|
| BOOT button | GPIO0 |
| Power LED   | GPIO3 |
| I2C0 SDA / SCL | GPIO4 / GPIO5 |
| I2S0 DIN / BCLK / WS | GPIO2 / GPIO41 / GPIO42 |
| Camera XCLK | GPIO15 (LEDC, 20 MHz) |
| LCD DC / BL | GPIO43 / GPIO48 |
| SDMMC CMD   | GPIO38 |

OV2640 DVP data pins, LCD SPI lines and SD-MMC data lines follow the
official Espressif ESP32-S3-EYE schematic v2.2; their assignments are baked
into the chip-side drivers and the board glue under `src/`.

## Build

The openvela `build.sh` wrapper is the only entry point you need:

```bash
cd openvela
./build.sh \
    $(pwd)/vendor/espressif/boards/esp32s3/esp32s3-eye/configs/openvela \
    -j$(nproc)
```

Successful output ends with:

```
LD: nuttx
MKIMAGE: ESP32-S3 binary
Successfully created ESP32S3 image.
Generated: nuttx.bin
```

Artefacts produced under `nuttx/`:

| File | Size | Purpose |
|------|-----:|---------|
| `nuttx`     | ~21 MB | ELF, used by GDB |
| `nuttx.bin` |  ~1.6 MB | flat image, flashed at offset `0x0` |
| `nuttx.hex` |  ~4 MB | Intel-HEX (optional) |

> If you ran `make distclean`, the next `build.sh` re-applies
> `scripts/patches/0001-esp-hal-3rdparty-fix-spinlock-init.patch`
> automatically. Building `nuttx` directly without the wrapper after
> `distclean` will fail with a duplicate `spinlock_init` link error.

## Flash

The board uses **simple-boot**: a single flat binary at flash offset `0x0`,
no separate bootloader or partition table.

```bash
PORT=$(ls /dev/serial/by-id/ | grep Espressif_USB_JTAG_serial_debug_unit | head -1)
PORT="/dev/serial/by-id/$PORT"

# Optional: erase only when switching firmware families.
esptool --chip esp32s3 --port "$PORT" erase-flash

esptool --chip esp32s3 --port "$PORT" --baud 460800 \
        --before default-reset --after hard-reset \
        write-flash 0x0 nuttx/nuttx.bin
```

If `esptool` cannot connect, hold **BOOT** while pressing **RESET** to
force ROM-download mode, then release BOOT.

## First Boot & Quick Tests

Open the USB-CDC console at 115200 8N1:

```bash
picocom -b 115200 /dev/ttyACM2
# or: minicom -D /dev/ttyACM2 -b 115200 -8 -o
```

Press the on-board **RESET**. You should see NuttX boot and `ls /dev`
list every peripheral:

```
nsh> uname -a
NuttX  0.0.0 <commit> <date> xtensa esp32s3-eye
nsh> ls /dev
/dev:
 accel0   audio/   buttons   console   i2c0   mmcsd1
 null     random   timer0    ttyACM0   userleds   video0   zero
```

Per-peripheral hand-test (all from `nsh>`):

```
i2c dev 0x03 0x77        # OV2640 ACKs at 0x30
camera -h                # QVGA RGB565 stream to LCD
lvgldemo                 # ST7789 LVGL demo
mount -t vfat /dev/mmcsd1 /mnt
ifconfig wlan0           # Wi-Fi MAC + IP
bt bnep0 info            # BDAddr + ACL info
buttons 1                # BOOT button events
leds 1                   # blink power LED
ostest                   # kernel regression suite
```

For Wi-Fi STA mode:

```
wapi mode wlan0 2
wapi psk wlan0 <YOUR_PSK> 3 wpa
wapi essid wlan0 <YOUR_SSID> 1
renew wlan0
ping -c 4 8.8.8.8
```

## Customising the Configuration

```bash
cd openvela/nuttx
make menuconfig          # tweak interactively
make savedefconfig
cp defconfig \
   ../vendor/espressif/boards/esp32s3/esp32s3-eye/configs/openvela/defconfig
```

To create an additional config variant (e.g. `nsh-only`), copy the
`configs/openvela/` directory to `configs/<your-name>/` and pass the new
path to `build.sh`.

## Debugging

- **Serial logs** — `syslog`/`printf` go to USB-CDC; capture with
  `picocom`/`tio`.
- **GDB over USB-JTAG** — the same USB connector exposes a JTAG TAP via
  `openocd -f board/esp32s3-builtin.cfg`, then
  `xtensa-esp32s3-elf-gdb nuttx/nuttx -ex 'target remote :3333'`.
- **Crash analysis** — save the full register/stack dump and feed it to
  `tools/scripts/decode_backtrace.py` together with `nuttx/nuttx`.

## Known Limitations

1. `fb` builtin app prints `Failed to open /dev/fb0: 2`; the ST7789 panel
   is registered as `/dev/lcd0`, not as a framebuffer. Use `lvgldemo`
   instead.
2. `bt bnep0 scan get` may hard-fault under some peer mixes — upstream
   NuttX BLE-GATT ENOTCONN issue traced to LE Privacy. HCI connect /
   pairing are unaffected; a fix is being prepared as a separate upstream
   PR.
3. `/dev/uorb` is empty — the QMA7981 driver registers as `/dev/accel0`
   for now; uORB integration is queued for v2.
4. microSD is locked to 1-bit SDIO; 4-bit support is on the follow-up
   list.

## License

All files in this directory are licensed under Apache-2.0 (SPDX
identifier `Apache-2.0`); see individual files for full headers.
