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

struct xl9555_btn_lowerhalf_s
{
  FAR struct ioexpander_dev_s *ioe;
  btn_handler_t handler;
  FAR void *arg;
  FAR void *attach_handle[BOARD_BUTTON_NUM];
};

typedef struct xl9555_btn_lowerhalf_s *xl9555_btn_lowerhalf_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct xl9555_btn_lowerhalf_s g_xl9555_btn =
{
  .ioe   = NULL,
  .handler = NULL,
  .arg   = NULL,
  .attach_handle = { NULL },
};

static const struct btn_lowerhalf_s g_xl9555_btn_lowerhalf =
{
  .bl_supported = xl9555_btn_supported,
  .bl_buttons   = xl9555_btn_buttons,
  .bl_enable    = xl9555_btn_enable,
  .bl_write     = NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int xl9555_btn_callback(FAR struct ioexpander_dev_s *dev,
                               ioe_pinset_t pinset, FAR void *arg)
{
  FAR struct xl9555_btn_lowerhalf_s *lower = &g_xl9555_btn;
  btn_buttonset_t buttons = 0;

  if (lower->handler == NULL)
    {
      return OK;
    }

  if (pinset & (1 << KEY_IO_PIN(BUTTON_KEY0)))
    {
      buttons |= BUTTON_KEY0_BIT;
    }

  if (pinset & (1 << KEY_IO_PIN(BUTTON_KEY1)))
    {
      buttons |= BUTTON_KEY1_BIT;
    }

  if (pinset & (1 << KEY_IO_PIN(BUTTON_KEY2)))
    {
      buttons |= BUTTON_KEY2_BIT;
    }

  if (pinset & (1 << KEY_IO_PIN(BUTTON_KEY3)))
    {
      buttons |= BUTTON_KEY3_BIT;
    }

  lower->handler(&g_xl9555_btn_lowerhalf, lower->arg);
  return OK;
}

static btn_buttonset_t xl9555_btn_supported(
  FAR const struct btn_lowerhalf_s *lower)
{
  return (BUTTON_KEY0_BIT | BUTTON_KEY1_BIT | BUTTON_KEY2_BIT | BUTTON_KEY3_BIT);
}

static btn_buttonset_t xl9555_btn_buttons(
  FAR const struct btn_lowerhalf_s *lower)
{
  FAR struct xl9555_btn_lowerhalf_s *priv = (FAR struct xl9555_btn_lowerhalf_s *)lower;
  btn_buttonset_t ret = 0;
  bool value;
  int ret_val;
  int i;

  DEBUGASSERT(priv->ioe != NULL);

  for (i = 0; i < BOARD_BUTTON_NUM; i++)
    {
      ret_val = IOEXP_READPIN(priv->ioe, KEY_IO_PIN(i), &value);
      if (ret_val == OK && value)
        {
          ret |= (1 << i);
        }
    }

  return ret;
}

static void xl9555_btn_enable(FAR const struct btn_lowerhalf_s *lower,
                              btn_buttonset_t press, btn_buttonset_t release,
                              btn_handler_t handler, FAR void *arg)
{
  FAR struct xl9555_btn_lowerhalf_s *priv = (FAR struct xl9555_btn_lowerhalf_s *)lower;
  btn_buttonset_t either = press | release;
  ioe_pinset_t pinset = 0;
  int i;

  DEBUGASSERT(priv->ioe != NULL);

  priv->handler = handler;
  priv->arg = arg;

  if (handler == NULL || either == 0)
    {
      for (i = 0; i < BOARD_BUTTON_NUM; i++)
        {
          if (priv->attach_handle[i] != NULL)
            {
              IOEXP_DETACH(priv->ioe, priv->attach_handle[i]);
              priv->attach_handle[i] = NULL;
            }
        }

      return;
    }

  for (i = 0; i < BOARD_BUTTON_NUM; i++)
    {
      if ((either & (1 << i)) != 0)
        {
          IOEXP_SETOPTION(priv->ioe, KEY_IO_PIN(i),
                         IOEXPANDER_OPTION_INTCFG,
                         (FAR void *)IOEXPANDER_VAL_BOTH);

          priv->attach_handle[i] = IOEXP_ATTACH(priv->ioe,
                                                (1 << KEY_IO_PIN(i)),
                                                xl9555_btn_callback,
                                                NULL);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int xl9555_btn_initialize(void)
{
  FAR struct ioexpander_dev_s *ioe;
  int ret;
  int i;

  ioe = esp32s3_gpioexp_getioe();
  if (ioe == NULL)
    {
      syslog(LOG_ERR, "ERROR: esp32s3_gpioexp_getioe() returned NULL\n");
      return -ENODEV;
    }

  g_xl9555_btn.ioe = ioe;

  for (i = 0; i < BOARD_BUTTON_NUM; i++)
    {
      ret = IOEXP_SETDIRECTION(ioe, KEY_IO_PIN(i), IOEXPANDER_DIRECTION_IN);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to set direction for pin %d: %d\n",
                 KEY_IO_PIN(i), ret);
          return ret;
        }

      ret = IOEXP_SETOPTION(ioe, KEY_IO_PIN(i),
                           IOEXPANDER_OPTION_INVERT,
                           (FAR void *)IOEXPANDER_VAL_NORMAL);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to set invert option for pin %d: %d\n",
                 KEY_IO_PIN(i), ret);
          return ret;
        }
    }

  return OK;
}

int board_button_initialize(void)
{
  int ret;

  ret = xl9555_btn_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: xl9555_btn_initialize() failed: %d\n", ret);
      return ret;
    }

  ret = btn_register("/dev/btn0", &g_xl9555_btn_lowerhalf);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: btn_register failed: %d\n", ret);
      return ret;
    }

  return OK;
}

#ifdef CONFIG_ARCH_IRQBUTTONS
int board_button_irq(int id, xcpt_t irqhandler, FAR void *arg)
{
  if (id < 0 || id >= BOARD_BUTTON_NUM)
    {
      return -EINVAL;
    }

  xl9555_btn_enable(&g_xl9555_btn_lowerhalf,
                    (1 << id), (1 << id),
                    irqhandler, arg);
  return OK;
}
#endif

#endif /* CONFIG_ARCH_BUTTONS */
