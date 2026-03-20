/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/esp32s3_userleds.c
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

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <syslog.h>

#include <nuttx/board.h>
#include <arch/board/board.h>

#include "esp32s3_gpio.h"
#include "esp32s3-boss1-cx.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint32_t g_ledcfg[BOARD_NLEDS] =
{
  GPIO_LED1,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_userled_initialize
 ****************************************************************************/

uint32_t board_userled_initialize(void)
{
  uint8_t i;

  syslog(LOG_INFO, "board_userled_initialize: Initializing %d LEDs\n", BOARD_NLEDS);

  for (i = 0; i < BOARD_NLEDS; i++)
    {
      esp32s3_configgpio(g_ledcfg[i], OUTPUT);
      syslog(LOG_INFO, "board_userled_initialize: Configured GPIO%d as OUTPUT\n", g_ledcfg[i]);
    }

  return BOARD_NLEDS;
}

/****************************************************************************
 * Name: board_userled
 ****************************************************************************/

void board_userled(int led, bool ledon)
{
  syslog(LOG_INFO, "board_userled: led=%d, ledon=%d\n", led, ledon);

  if ((unsigned)led < BOARD_NLEDS)
    {
      esp32s3_gpiowrite(g_ledcfg[led], ledon);
    }
}

/****************************************************************************
 * Name: board_userled_all
 ****************************************************************************/

void board_userled_all(uint32_t ledset)
{
  bool ledon;
  uint8_t i;

  syslog(LOG_INFO, "board_userled_all: ledset=0x%x\n", ledset);

  for (i = 0; i < BOARD_NLEDS; i++)
    {
      ledon = ((ledset & (1 << i)) != 0);
      syslog(LOG_INFO, "board_userled_all: LED%d = %d\n", i, ledon);
      esp32s3_gpiowrite(g_ledcfg[i], ledon);
    }
}
