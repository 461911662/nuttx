/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/esp32s3_buttons.c
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

#include "esp32s3-boss1-cx.h"

#ifdef CONFIG_ARCH_BUTTONS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define KEY_IO_PIN(n) ((n) + 1)

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
  return btn_cb->handler(0, NULL, btn_cb->arg);
}

static btn_buttonset_t xl9555_btn_buttons(void)
{
  btn_buttonset_t ret = 0;
  bool value;
  int ret_val;
  int i;

  for (i = 0; i < BOARD_BUTTON_NUM; i++)
    {
      ret_val = IOEXP_READPIN(g_btn_ioe, KEY_IO_PIN(i), &value);
      if (ret_val == OK && value)
        {
          ret |= (1 << i);
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_button_initialize
 *
 * Description:
 *   Initialize button hardware. Called by btn_lower_initialize().
 *
 ****************************************************************************/

uint32_t board_button_initialize(void)
{
  int ret;
  int i;

  g_btn_ioe = esp32s3_gpioexp_getioe();
  if (g_btn_ioe == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to get XL9555 device\n");
      return 0;
    }

  for (i = 0; i < BOARD_BUTTON_NUM; i++)
    {
      ret = IOEXP_SETDIRECTION(g_btn_ioe, KEY_IO_PIN(i),
                               IOEXPANDER_DIRECTION_IN);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to set direction for pin %d: %d\n",
                 KEY_IO_PIN(i), ret);
          return ret;
        }

      /* invert pin state refs to boss1-cx board */
      ret = IOEXP_SETOPTION(g_btn_ioe, KEY_IO_PIN(i),
                           IOEXPANDER_OPTION_INVERT,
                           (FAR void *)IOEXPANDER_VAL_INVERT);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to set invert option for pin %d: %d\n",
                 KEY_IO_PIN(i), ret);
          return ret;
        }
    }

  return BOARD_BUTTON_NUM;
}

/****************************************************************************
 * Name: board_buttons
 *
 * Description:
 *   Read current button states.
 *
 ****************************************************************************/

uint32_t board_buttons(void)
{
  return xl9555_btn_buttons();
}

/****************************************************************************
 * Name: board_button_irq
 *
 * Description:
 *   Attach interrupt callback to XL9555 button.
 *
 ****************************************************************************/

#ifdef CONFIG_ARCH_IRQBUTTONS
static struct btn_ioe_callback_s g_btn_cb;

int board_button_irq(int id, xcpt_t irqhandler, FAR void *arg)
{
  if (id < 0 || id >= BOARD_BUTTON_NUM)
    {
      return -EINVAL;
    }

  IOEXP_SETOPTION(g_btn_ioe, KEY_IO_PIN(id),
                  IOEXPANDER_OPTION_INTCFG,
                  (FAR void *)IOEXPANDER_VAL_BOTH);

  g_btn_cb.handler = irqhandler;
  g_btn_cb.arg = arg;

  IOEP_ATTACH(g_btn_ioe,
              (1 << KEY_IO_PIN(id)),
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
