/****************************************************************************
 * boards/xtensa/esp32s3/20260409-boss0/src/esp32s3_userleds.c
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
#include <nuttx/ioexpander/ioexpander.h>

#include "esp32s3-20260409-boss0.h"

#ifdef CONFIG_USERLED

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct ioexpander_dev_s *g_led_ioe;

static const uint32_t g_ledcfg[BOARD_NLEDS] =
{
  BOARD_XL9555_IO_P14,
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

  syslog(LOG_INFO, "board_userled_initialize: Initializing %" PRId16 " LEDs\n", BOARD_NLEDS);

  g_led_ioe = esp32s3_gpioexp_getioe();
  if (g_led_ioe == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to get XL9555 device\n");
      return 0;
    }

  for (i = 0; i < BOARD_NLEDS; i++)
    {
      IOEXP_SETDIRECTION(g_led_ioe, g_ledcfg[i], IOEXPANDER_DIRECTION_OUT);
    }

  return BOARD_NLEDS;
}

/****************************************************************************
 * Name: board_userled
 ****************************************************************************/

void board_userled(int led, bool ledon)
{
  syslog(LOG_INFO, "board_userled: led=%" PRId16 ", ledon=%" PRIu8 "\n", led, ledon);

  if ((unsigned)led < BOARD_NLEDS && g_led_ioe != NULL)
    {
      IOEXP_WRITEPIN(g_led_ioe, g_ledcfg[led], ledon ? 1 : 0);
    }
}

/****************************************************************************
 * Name: board_userled_all
 ****************************************************************************/

void board_userled_all(uint32_t ledset)
{
  bool ledon;
  uint8_t i;

  syslog(LOG_INFO, "board_userled_all: ledset=0x%" PRIu32 "\n", ledset);

  if (g_led_ioe == NULL)
    {
      return;
    }

  for (i = 0; i < BOARD_NLEDS; i++)
    {
      ledon = ((ledset & (1 << i)) != 0);
      syslog(LOG_INFO, "board_userled_all: LED%" PRIu8 " = %" PRIu8 "\n", i, ledon);
      IOEXP_WRITEPIN(g_led_ioe, g_ledcfg[i], ledon ? 1 : 0);
    }
}

#endif /* CONFIG_USERLED */
