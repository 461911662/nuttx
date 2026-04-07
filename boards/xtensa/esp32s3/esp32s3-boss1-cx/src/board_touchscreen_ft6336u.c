/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/board_touchscreen_ft6336u.c
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
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/irq.h>
#include <nuttx/input/ft5x06.h>
#include <nuttx/ioexpander/ioexpander.h>

#include "esp32s3_i2c.h"
#include "esp32s3-boss1-cx.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FT6336U_I2C_PORT         1
#define FT6336U_I2C_ADDR         0x38
#define FT6336U_I2C_FREQUENCY    (400 * 1000)

#define FT6336U_RST_XL9555_PIN   CONFIG_ESP32S3_BOSS1_CX_TOUCH_XL9555_RST_PIN
#define FT6336U_INT_XL9555_PIN   CONFIG_ESP32S3_BOSS1_CX_TOUCH_XL9555_INT_PIN

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ft5x06_priv_s
{
  FAR struct ioexpander_dev_s *ioe;
  FAR void *irqhandle;
  xcpt_t isr;
  FAR void *cbarg;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ft5x06_priv_s g_ft5x06_priv;

#ifndef CONFIG_FT5X06_POLLMODE
static int  esp32s3_ft5x06_attach(const struct ft5x06_config_s *config,
                                   xcpt_t isr, void *arg);
static void esp32s3_ft5x06_enable(const struct ft5x06_config_s *config,
                                   bool enable);
static void esp32s3_ft5x06_clear(const struct ft5x06_config_s *config);
#endif
static void esp32s3_ft5x06_wakeup(const struct ft5x06_config_s *config);
static void esp32s3_ft5x06_nreset(const struct ft5x06_config_s *config,
                                  bool state);

static const struct ft5x06_config_s g_ft5x06_config =
{
  .address   = FT6336U_I2C_ADDR,
  .frequency = FT6336U_I2C_FREQUENCY,
#ifndef CONFIG_FT5X06_POLLMODE
  .attach    = esp32s3_ft5x06_attach,
  .enable    = esp32s3_ft5x06_enable,
  .clear     = esp32s3_ft5x06_clear,
#endif
  .wakeup    = esp32s3_ft5x06_wakeup,
  .nreset    = esp32s3_ft5x06_nreset
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft5x06_irq_callback
 *
 * Description:
 *   Callback function for XL9555 IO9 interrupt. This is called when the
 *   FT6336U INT pin triggers (active low).
 *
 ****************************************************************************/

#ifndef CONFIG_FT5X06_POLLMODE
static int ft5x06_irq_callback(FAR struct ioexpander_dev_s *dev,
                               ioe_pinset_t pinset, FAR void *arg)
{
  bool value;

  IOEXP_READPIN(g_ft5x06_priv.ioe, FT6336U_INT_XL9555_PIN, &value);

  if (!value && g_ft5x06_priv.isr != NULL)
    {
      g_ft5x06_priv.isr(0, NULL, g_ft5x06_priv.cbarg);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: esp32s3_ft5x06_attach
 *
 * Description:
 *   Attach an FT5x06 interrupt handler to a GPIO interrupt.
 *
 ****************************************************************************/

#ifndef CONFIG_FT5X06_POLLMODE
static int esp32s3_ft5x06_attach(const struct ft5x06_config_s *config,
                                 xcpt_t isr, void *arg)
{
  g_ft5x06_priv.isr = isr;
  g_ft5x06_priv.cbarg = arg;

  g_ft5x06_priv.irqhandle = IOEP_ATTACH(g_ft5x06_priv.ioe,
                                        (1 << FT6336U_INT_XL9555_PIN),
                                        ft5x06_irq_callback,
                                        &g_ft5x06_priv);
  if (g_ft5x06_priv.irqhandle == NULL)
    {
      ierr("ERROR: Failed to attach interrupt callback\n");
      return -EINVAL;
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: esp32s3_ft5x06_enable
 *
 * Description:
 *   Enable or disable a GPIO interrupt.
 *
 ****************************************************************************/

#ifndef CONFIG_FT5X06_POLLMODE
static void esp32s3_ft5x06_enable(const struct ft5x06_config_s *config,
                                  bool enable)
{
  if (g_ft5x06_priv.irqhandle != NULL)
    {
      IOEP_DETACH(g_ft5x06_priv.ioe, g_ft5x06_priv.irqhandle);
      g_ft5x06_priv.irqhandle = NULL;
    }

  if (enable)
    {
      g_ft5x06_priv.irqhandle = IOEP_ATTACH(g_ft5x06_priv.ioe,
                                            (1 << FT6336U_INT_XL9555_PIN),
                                            ft5x06_irq_callback,
                                            &g_ft5x06_priv);
    }
}
#endif

/****************************************************************************
 * Name: esp32s3_ft5x06_clear
 *
 * Description:
 *   Acknowledge/clear any pending GPIO interrupt.
 *
 ****************************************************************************/

#ifndef CONFIG_FT5X06_POLLMODE
static void esp32s3_ft5x06_clear(const struct ft5x06_config_s *config)
{
  bool value;
  int ret;

  ret = IOEXP_READPIN(g_ft5x06_priv.ioe, FT6336U_INT_XL9555_PIN, &value);
  if (ret < 0)
    {
      ierr("ERROR: Failed to read INT pin: %d\n", ret);
    }
}
#endif

/****************************************************************************
 * Name: esp32s3_ft5x06_wakeup
 *
 * Description:
 *   Issue WAKE interrupt to FT5x06 to change the FT5x06 from Hibernate to
 *   Active mode.
 *
 ****************************************************************************/

static void esp32s3_ft5x06_wakeup(const struct ft5x06_config_s *config)
{
}

/****************************************************************************
 * Name: esp32s3_ft5x06_nreset
 *
 * Description:
 *   Control the chip reset pin (active low).
 *
 ****************************************************************************/

static void esp32s3_ft5x06_nreset(const struct ft5x06_config_s *config,
                                  bool state)
{
  int ret;

  ret = IOEXP_SETDIRECTION(g_ft5x06_priv.ioe, FT6336U_RST_XL9555_PIN,
                           IOEXPANDER_DIRECTION_OUT);
  if (ret < 0)
    {
      ierr("ERROR: Failed to set RST pin direction: %d\n", ret);
      return;
    }

  ret = IOEXP_WRITEPIN(g_ft5x06_priv.ioe, FT6336U_RST_XL9555_PIN, state);
  if (ret < 0)
    {
      ierr("ERROR: Failed to write RST pin: %d\n", ret);
    }
}

/****************************************************************************
 * Name: esp32s3_ft5x06_reset
 *
 * Description:
 *   Perform hardware reset of FT6336U.
 *
 ****************************************************************************/

#ifndef CONFIG_FT5X06_POLLMODE
static void esp32s3_ft5x06_reset(void)
{
  int ret;

  ret = IOEXP_SETDIRECTION(g_ft5x06_priv.ioe, FT6336U_RST_XL9555_PIN,
                           IOEXPANDER_DIRECTION_OUT);
  if (ret < 0)
    {
      ierr("ERROR: Failed to set RST pin direction: %d\n", ret);
      return;
    }

  ret = IOEXP_WRITEPIN(g_ft5x06_priv.ioe, FT6336U_RST_XL9555_PIN, false);
  if (ret < 0)
    {
      ierr("ERROR: Failed to assert RST: %d\n", ret);
      return;
    }

  nxsched_usleep(5000);

  ret = IOEXP_WRITEPIN(g_ft5x06_priv.ioe, FT6336U_RST_XL9555_PIN, true);
  if (ret < 0)
    {
      ierr("ERROR: Failed to deassert RST: %d\n", ret);
    }

  nxsched_usleep(100000);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_touchscreen_initialize
 *
 * Description:
 *   Initialize touchscreen.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

#ifndef CONFIG_FT5X06_POLLMODE
int board_touchscreen_initialize(void)
{
  FAR struct i2c_master_s *i2c;
  int ret;

  g_ft5x06_priv.ioe = esp32s3_gpioexp_getioe();
  if (g_ft5x06_priv.ioe == NULL)
    {
      ierr("ERROR: Failed to get IO expander device\n");
      return -ENODEV;
    }

  ret = IOEXP_SETDIRECTION(g_ft5x06_priv.ioe, FT6336U_RST_XL9555_PIN,
                           IOEXPANDER_DIRECTION_OUT);
  if (ret < 0)
    {
      ierr("ERROR: Failed to set RST pin direction: %d\n", ret);
      return ret;
    }

  ret = IOEXP_SETDIRECTION(g_ft5x06_priv.ioe, FT6336U_INT_XL9555_PIN,
                           IOEXPANDER_DIRECTION_IN);
  if (ret < 0)
    {
      ierr("ERROR: Failed to set INT pin direction: %d\n", ret);
      return ret;
    }

  i2c = esp32s3_i2cbus_initialize(FT6336U_I2C_PORT);
  if (i2c == NULL)
    {
      ierr("ERROR: Failed to initialize I2C bus %d\n", FT6336U_I2C_PORT);
      return -EINVAL;
    }

  esp32s3_ft5x06_reset();

  ret = ft5x06_register(i2c, &g_ft5x06_config, 0);
  if (ret < 0)
    {
      ierr("ERROR: ft5x06_register failed: %d\n", ret);
      esp32s3_i2cbus_uninitialize(i2c);
      return ret;
    }

  return OK;
}
#endif
