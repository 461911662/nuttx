/****************************************************************************
 * boards/xtensa/esp32s3/20260409-boss0/src/esp32s3_autoleds.c
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

#include <nuttx/board.h>
#include <arch/board/board.h>
#include <nuttx/ioexpander/ioexpander.h>

#include "esp32s3-20260409-boss0.h"

#ifdef CONFIG_ARCH_LEDS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* There is one LED on the 20260409-boss0 board:
 *
 *     LED              XL9555 Pin
 *     ---------------- ---------
 *     LED1              P14
 *
 * The LED is not used by the board port unless CONFIG_ARCH_LEDS is
 * defined. In that case, the usage by the board port is defined in
 * include/board.h and src/esp32s3_autoleds.c. The LED is used to encode
 * OS-related events as follows:
 *
 *   SYMBOL                MEANING               LED STATE
 *   -------------------  -----------------------  --------
 *   LED_STARTED          NuttX has been started     OFF
 *   LED_HEAPALLOCATE     Heap has been allocated    OFF
 *   LED_IRQSENABLED      Interrupts enabled         OFF
 *   LED_STACKCREATED     Idle stack created         ON
 *   LED_INIRQ            In an interrupt            N/C
 *   LED_SIGNAL           In a signal handler        N/C
 *   LED_ASSERTION        An assertion failed        N/C
 *   LED_PANIC            The system has crashed     N/C
 */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct ioexpander_dev_s *g_aled_ioe;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_led_initialize
 ****************************************************************************/

void esp32s3_led_initialize(void)
{
  g_aled_ioe = esp32s3_gpioexp_getioe();
}

/****************************************************************************
 * Name: board_autoled_on
 ****************************************************************************/

void board_autoled_on(int led)
{
  bool ledon = true;

  switch (led)
    {
      case 0:
        ledon = false;
        break;

      case 2:
        return;

      case 1:
      case 3:
        break;
    }

  if (g_aled_ioe != NULL)
    {
      IOEXP_WRITEPIN(g_aled_ioe, BOARD_XL9555_IO_P14, ledon ? 1 : 0);
    }
}

/****************************************************************************
 * Name: board_autoled_off
 ****************************************************************************/

void board_autoled_off(int led)
{
  switch (led)
    {
      case 0:
      case 1:
      case 3:
        break;

      case 2:
        return;
    }

  if (g_aled_ioe != NULL)
    {
      IOEXP_WRITEPIN(g_aled_ioe, BOARD_XL9555_IO_P14, 0);
    }
}

#endif /* CONFIG_ARCH_LEDS */
