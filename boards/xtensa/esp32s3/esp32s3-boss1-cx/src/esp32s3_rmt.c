/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/esp32s3_rmt.c
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

#include <errno.h>
#include <debug.h>
#include <stdio.h>

#include "xtensa.h"

#include <nuttx/kmalloc.h>
#include <nuttx/rmt/rmtchar.h>
#ifdef CONFIG_WS2812_NON_SPI_DRIVER
#include <nuttx/leds/ws2812.h>
#include "espressif/esp_ws2812.h"
#endif

#include "espressif/esp_rmt.h"

#ifdef CONFIG_ESP_RMT

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_rmt_tx_init
 *
 * Description:
 *   Initialize RMT peripheral for TX (IR or other protocols).
 *
 * Input Parameters:
 *   channel - RMT channel number (0-7)
 *   gpio    - GPIO pin number
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_rmt_tx_init(int channel, int gpio)
{
  struct rmt_dev_s *rmt;
  int ret;

  rmt = esp_rmt_tx_init(channel, gpio);
  if (rmt == NULL)
    {
      syslog(LOG_ERR, "ERROR: esp_rmt_tx_init failed: ch=%d gpio=%d\n",
             channel, gpio);
      return -ENODEV;
    }

  ret = rmtchar_register(rmt);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: rmtchar_register failed: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO, "RMT TX initialized: channel=%d, gpio=%d\n",
         channel, gpio);

  return OK;
}

/****************************************************************************
 * Name: board_ws2812_init
 *
 * Description:
 *   Initialize RMT peripheral for WS2812 LED strip.
 *
 * Input Parameters:
 *   channel   - RMT channel number (0-7)
 *   gpio      - GPIO pin number
 *   led_count - Number of WS2812 LEDs
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_WS2812_NON_SPI_DRIVER
int board_ws2812_init(int channel, int gpio, int led_count)
{
  struct rmt_dev_s *rmt;
  struct ws2812_dev_s *led;

  rmt = esp_rmt_tx_init(channel, gpio);
  if (rmt == NULL)
    {
      syslog(LOG_ERR, "ERROR: esp_rmt_tx_init failed for WS2812: ch=%d gpio=%d\n",
             channel, gpio);
      return -ENODEV;
    }

  led = esp_ws2812_setup("/dev/leds0", rmt, led_count, false);
  if (led == NULL)
    {
      syslog(LOG_ERR, "ERROR: esp_ws2812_setup failed\n");
      return -ENODEV;
    }

  syslog(LOG_INFO, "WS2812 initialized: channel=%d, gpio=%d, leds=%d\n",
         channel, gpio, led_count);

  return OK;
}
#endif

#endif
