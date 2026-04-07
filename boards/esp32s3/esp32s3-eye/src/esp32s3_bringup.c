/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-eye/src/esp32s3_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <errno.h>
#include <nuttx/fs/fs.h>

#ifdef CONFIG_ESP32S3_TIMER
#include "esp32s3_board_tim.h"
#endif

#ifdef CONFIG_ESP32S3_WIFI
#include "esp32s3_board_wlan.h"
#endif

#ifdef CONFIG_ESP32S3_BLE
#include "esp32s3_ble.h"
#endif

#ifdef CONFIG_ESP32S3_WIFI_BT_COEXIST
#include "esp32s3_wifi_adapter.h"
#endif

#ifdef CONFIG_ESP32S3_RT_TIMER
#include "esp32s3_rt_timer.h"
#endif

#ifdef CONFIG_ESP32S3_I2C
#include "esp32s3_i2c.h"
#endif

#ifdef CONFIG_WATCHDOG
#include "esp32s3_board_wdt.h"
#endif

#ifdef CONFIG_INPUT_BUTTONS
#include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_ESP32S3_SPI
#include "esp32s3_spi.h"
#endif

#ifdef CONFIG_LCD_DEV
#include <nuttx/board.h>
#include <nuttx/lcd/lcd_dev.h>
#endif

#ifdef CONFIG_ESP32S3_SDMMC
#include "esp32s3_board_sdmmc.h"
#endif

#ifdef CONFIG_ESP32S3_CAM
#include "esp32s3_camera.h"
#endif

#include "esp32s3-eye.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=y :
 *     Called from board_late_initialize().
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=n && CONFIG_BOARDCTL=y :
 *     Called from the NSH library
 *
 ****************************************************************************/

int esp32s3_bringup(void)
{
    int ret;

#ifdef CONFIG_FS_PROCFS
    /* Mount the procfs file system */

    ret = nx_mount(NULL, "/proc", "procfs", 0, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "ERROR: Failed to mount procfs at /proc: %d\n", ret);
    }
#endif

#ifdef CONFIG_FS_TMPFS
    /* Mount the tmpfs file system */

    ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "ERROR: Failed to mount tmpfs at %s: %d\n",
            CONFIG_LIBC_TMPDIR, ret);
    }
#endif

#ifdef CONFIG_ESP32S3_TIMER
    /* Configure general purpose timers */

    ret = board_tim_init();
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to initialize timers: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP32S3_RT_TIMER
    ret = esp32s3_rt_timer_init();
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to initialize RT timer: %d\n", ret);
    }
#endif

#ifdef CONFIG_WATCHDOG
    /* Configure watchdog timer */

    ret = board_wdt_init();
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to initialize watchdog timer: %d\n", ret);
    }
#endif

#ifdef CONFIG_I2C_DRIVER
    /* Configure I2C peripheral interfaces */

    ret = board_i2c_init();
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to initialize I2C driver: %d\n", ret);
    }
#endif

#ifdef CONFIG_INPUT_BUTTONS
    /* Register the BUTTON driver */

    ret = btn_lower_initialize("/dev/buttons");
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to initialize button driver: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP32S3_SPIFLASH
    ret = board_spiflash_init();
    if (ret) {
        syslog(LOG_ERR, "ERROR: Failed to initialize SPI Flash\n");
    }
#endif

#ifdef CONFIG_ESP32S3_WIRELESS

#ifdef CONFIG_ESP32S3_WIFI_BT_COEXIST
    ret = esp32s3_wifi_bt_coexist_init();
    if (ret) {
        syslog(LOG_ERR, "ERROR: Failed to initialize Wi-Fi and BT coexist\n");
    }
#endif

#ifdef CONFIG_ESP32S3_BLE
    ret = esp32s3_ble_initialize();
    if (ret) {
        syslog(LOG_ERR, "ERROR: Failed to initialize BLE\n");
    }
#endif

#ifdef CONFIG_ESP32S3_WIFI
    ret = board_wlan_init();
    if (ret < 0) {
        syslog(LOG_ERR, "ERROR: Failed to initialize wireless subsystem=%d\n",
            ret);
    }
#endif

#endif

#ifdef CONFIG_DEV_GPIO
    ret = esp32s3_gpio_init();
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to initialize GPIO Driver: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP32S3_EYE_LCD

#ifdef CONFIG_VIDEO_FB
    ret = fb_register(0, 0);
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to initialize Frame Buffer Driver.\n");
        return ret;
    }
#elif defined(CONFIG_LCD_DEV)
    ret = board_lcd_initialize();
    if (ret < 0) {
        syslog(LOG_ERR, "ERROR: Failed to initialize LCD.\n");
        return ret;
    }

    ret = lcddev_register(0);
    if (ret < 0) {
        syslog(LOG_ERR, "ERROR: lcddev_register() failed: %d\n", ret);
    }
#endif

#endif

#ifdef CONFIG_ESP32S3_SDMMC
    ret = board_sdmmc_initialize();
    if (ret < 0) {
        syslog(LOG_ERR, "ERROR: Failed to initialize SDMMC: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP32S3_CAM
    {
        static const struct esp32s3_cam_config_s cam_config = {
            .xclk_pin = ESP32S3_EYE_CAM_XCLK,
            .pclk_pin = ESP32S3_EYE_CAM_PCLK,
            .vsync_pin = ESP32S3_EYE_CAM_VSYNC,
            .href_pin = ESP32S3_EYE_CAM_HREF,
            .data_pins = {
                ESP32S3_EYE_CAM_D0, ESP32S3_EYE_CAM_D1,
                ESP32S3_EYE_CAM_D2, ESP32S3_EYE_CAM_D3,
                ESP32S3_EYE_CAM_D4, ESP32S3_EYE_CAM_D5,
                ESP32S3_EYE_CAM_D6, ESP32S3_EYE_CAM_D7 },
            .xclk_freq = ESP32S3_EYE_CAM_XCLK_FREQ,
            .width = 320,
            .height = 240,
            .bpp = 2
        };

        ret = esp32s3_cam_initialize(&cam_config);
        if (ret < 0) {
            syslog(LOG_ERR, "ERROR: Failed to initialize camera: %d\n", ret);
        } else {

            ret = board_ov2640_initialize();
            if (ret < 0) {
                syslog(LOG_ERR, "ERROR: Failed to initialize OV2640: %d\n", ret);
            } else {
                syslog(LOG_INFO, "OV2640 sensor configured for QVGA RGB565\n");
            }
        }
    }
#endif

    /* If we got here then perhaps not all initialization was successful, but
     * at least enough succeeded to bring-up NSH with perhaps reduced
     * capabilities.
     */

    UNUSED(ret);
    return OK;
}
