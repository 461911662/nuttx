/****************************************************************************
 * boards/xtensa/esp32s3/20260409-boss0/src/esp32s3_buttons.c
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

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <stdbool.h>
#include <syslog.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <arch/board/board.h>
#include <nuttx/input/buttons.h>
#include <nuttx/ioexpander/ioexpander.h>

#include "esp32s3-20260409-boss0.h"

#ifdef CONFIG_ARCH_BUTTONS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUTTON_PIN 0

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct btn_ioe_callback_s
{
  xcpt_t handler;
  FAR void *arg;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct ioexpander_dev_s *g_btn_ioe;
static struct btn_ioe_callback_s g_btn_cb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int btn_ioe_irq_wrapper(FAR struct ioexpander_dev_s *dev,
                               ioe_pinset_t pinset, FAR void *arg)
{
  FAR struct btn_ioe_callback_s *btn_cb =
    (FAR struct btn_ioe_callback_s *)arg;

  UNUSED(dev);
  UNUSED(pinset);

  if (btn_cb == NULL || btn_cb->handler == NULL)
    {
      return -EINVAL;
    }

  return btn_cb->handler(0, NULL, btn_cb->arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_button_initialize
 ****************************************************************************/

uint32_t board_button_initialize(void)
{
  int ret;

  g_btn_ioe = esp32s3_gpioexp_getioe();
  if (g_btn_ioe == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to get XL9555 device\n");
      return 0;
    }

  ret = IOEXP_SETDIRECTION(g_btn_ioe, BUTTON_PIN, IOEXPANDER_DIRECTION_IN);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to set direction for pin %d: %d\n",
             BUTTON_PIN, ret);
      return ret;
    }

  return BOARD_BUTTON_NUM;
}

/****************************************************************************
 * Name: board_buttons
 ****************************************************************************/

uint32_t board_buttons(void)
{
  bool value;
  int ret;

  if (g_btn_ioe == NULL)
    {
      return 0;
    }

  ret = IOEXP_READPIN(g_btn_ioe, BUTTON_PIN, &value);
  if (ret == OK && value)
    {
      return (1 << BUTTON_PIN);
    }

  return 0;
}

/****************************************************************************
 * Name: board_button_irq
 ****************************************************************************/

#ifdef CONFIG_ARCH_IRQBUTTONS
int board_button_irq(int id, xcpt_t irqhandler, FAR void *arg)
{
  if (id < 0 || id >= BOARD_BUTTON_NUM)
    {
      return -EINVAL;
    }

  IOEXP_SETOPTION(g_btn_ioe, BUTTON_PIN,
                  IOEXPANDER_OPTION_INTCFG,
                  (FAR void *)IOEXPANDER_VAL_BOTH);

  g_btn_cb.handler = irqhandler;
  g_btn_cb.arg = arg;

  IOEP_ATTACH(g_btn_ioe,
              (1 << BUTTON_PIN),
              btn_ioe_irq_wrapper,
              &g_btn_cb);

  return OK;
}
#else
int board_button_irq(int id, xcpt_t irqhandler, FAR void *arg)
{
  UNUSED(id);
  UNUSED(irqhandler);
  UNUSED(arg);
  return -ENOSYS;
}
#endif

#endif /* CONFIG_ARCH_BUTTONS */
