/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/esp32s3_bringup.c
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <debug.h>

#include <errno.h>
#include <nuttx/fs/fs.h>
#include <nuttx/himem/himem.h>
#include <arch/board/board.h>

#ifdef CONFIG_USERLED
#  include <nuttx/leds/userled.h>
#endif

#ifdef CONFIG_INPUT_BUTTONS
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_ESP_RMT
#  include "esp32s3_board_rmt.h"
#endif

#ifdef CONFIG_ESP32S3_I2C
#  include "esp32s3_i2c.h"
#endif

#include "esp32s3-boss1-cx.h"

#ifdef CONFIG_ESP32S3_SDMMC
#  include "esp32s3_board_sdmmc.h"
#endif

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
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount procfs at /proc: %d\n", ret);
    }
#endif

#ifdef CONFIG_FS_TMPFS
  /* Mount the tmpfs file system */

  ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount tmpfs at %s: %d\n",
             CONFIG_LIBC_TMPDIR, ret);
    }
#endif

#ifdef CONFIG_ESP32S3_SPIFLASH
  ret = board_spiflash_init();
  if (ret)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize SPI Flash\n");
    }
#endif

#ifdef CONFIG_USERLED
  /* Register the LED driver */

  ret = userled_lower_initialize("/dev/userleds");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: userled_lower_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP_RMT
  /* Initialize RMT TX for IR transmitter */

  ret = board_rmt_tx_init(BOARD_IR_TX_CHANNEL, BOARD_IR_TX_GPIO);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: board_rmt_tx_init() failed: %d\n", ret);
    }

#ifdef CONFIG_WS2812_NON_SPI_DRIVER
  /* Initialize RMT TX for WS2812 LED strip */

  ret = board_ws2812_init(BOARD_WS2812_RMT_CHANNEL,
                          BOARD_WS2812_GPIO,
                          CONFIG_WS2812_LED_COUNT);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: board_ws2812_init() failed: %d\n", ret);
    }
#endif
#endif

#ifdef CONFIG_I2C_DRIVER
  /* Configure I2C peripheral interfaces */

  ret = board_i2c_init();
  if (ret < 0)
    {
      syslog(LOG_ERR, "Failed to initialize I2C driver: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP
  /* Initialize GPIO expander (XL9555) */

  ret = esp32s3_gpioexp_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize GPIO expander: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESPRESSIF_I2S0
  /* Initialize I2S0 for LMD4030 microphone (master mode) */

  bool i2s0_enable_tx = false;
  bool i2s0_enable_rx = false;

#ifdef CONFIG_ESP32S3_BOSS1_CX_I2S0_TX
  i2s0_enable_tx = true;
#endif

#ifdef CONFIG_ESP32S3_BOSS1_CX_I2S0_RX
  i2s0_enable_rx = true;
#endif

  ret = board_i2sdev_initialize(0, i2s0_enable_tx, i2s0_enable_rx);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize I2S0 driver: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP32S3_BOSS1_CX_SPEAKER
  /* Initialize speaker (NS4168) with power control via XL9555 */

  ret = esp32s3_spk_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize speaker: %d\n", ret);
    }
#endif

#ifdef CONFIG_ESP32S3_SDMMC
  /* Initialize SDMMC for SD card */

  ret = board_sdmmc_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize SDMMC: %d\n", ret);
    }
#endif

#ifdef CONFIG_INPUT_BUTTONS
  /* Initialize button driver using NuttX generic lower half */

  ret = btn_lower_initialize("/dev/buttons");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: btn_lower_initialize failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DEV_GPIO
  /* Initialize native ESP32-S3 GPIO */

  ret = esp32s3_gpio_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize GPIO: %d\n", ret);
    }
#endif

  /* If we got here then perhaps not all initialization was successful, but
   * at least enough succeeded to bring-up NSH with perhaps reduced
   * capabilities.
   */

  UNUSED(ret);
  return OK;
}
