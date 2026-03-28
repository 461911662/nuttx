/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/esp32s3_gpio.c
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
#include <stdio.h>
#include <syslog.h>

#include <nuttx/irq.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/ioexpander/xl9555_int.h>
#include <nuttx/ioexpander/gpio.h>

#include <arch/board/board.h>
#include "esp32s3_gpio.h"
#include "esp32s3_i2c.h"
#include "esp32s3-boss1-cx.h"

#ifdef CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GPIO_EXP_SCL_PIN  CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP_SCLPIN
#define GPIO_EXP_SDA_PIN  CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP_SDAPIN
#define GPIO_EXP_IRQ_PIN  CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP_IRQ_PIN
#define GPIO_EXP_ADDR     CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP_ADDR
#define GPIO_EXP_FREQ     CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP_FREQ

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct esp32s3_gpioexp_dev_s
{
  FAR struct ioexpander_dev_s *ioe;
#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  int irq;
#endif
};

static struct esp32s3_gpioexp_dev_s g_gpioexp;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_IOEXPANDER_INT_ENABLE

/****************************************************************************
 * Name: esp32s3_gpioexp_interrupt
 *
 * Description:
 *   Handle GPIO interrupt events from XL9555.
 *
 ****************************************************************************/

static int esp32s3_gpioexp_interrupt(int irq, FAR void *context, FAR void *arg)
{
  FAR struct esp32s3_gpioexp_dev_s *dev = (FAR struct esp32s3_gpioexp_dev_s *)arg;

  /* Interrupt is handled by the xl9555_int driver via its internal worker.
   * This top-half just acknowledges the interrupt.
   */

  UNUSED(dev);
  return OK;
}

/****************************************************************************
 * Name: esp32s3_gpioexp_attach
 *
 * Description:
 *   Attach the XL9555 interrupt handler to the GPIO interrupt.
 *
 ****************************************************************************/

static int esp32s3_gpioexp_attach(FAR struct xl9555_int_config_s *config,
                                 xcpt_t isr, FAR void *arg)
{
  int ret;

  ret = irq_attach(g_gpioexp.irq, isr, arg);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: irq_attach failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: esp32s3_gpioexp_enable
 *
 * Description:
 *   Enable or disable the GPIO interrupt.
 *
 ****************************************************************************/

static void esp32s3_gpioexp_enable(FAR struct xl9555_int_config_s *config,
                                   bool enable)
{
  if (enable)
    {
      esp32s3_gpioirqenable(g_gpioexp.irq, RISING);
    }
  else
    {
      esp32s3_gpioirqdisable(g_gpioexp.irq);
    }
}

#endif /* CONFIG_IOEXPANDER_INT_ENABLE */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_gpioexp_initialize
 *
 * Description:
 *   Initialize and register the GPIO expander driver.
 *
 ****************************************************************************/

int esp32s3_gpioexp_initialize(void)
{
  FAR struct i2c_master_s *i2c;
  FAR struct ioexpander_dev_s *ioe;
  struct xl9555_int_config_s config;
  int ret;
  int i;

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  /* Configure the interrupt pin */

  g_gpioexp.irq = GPIO_EXP_IRQ_PIN + XTENSA_IRQ_FIRSTPERIPH;
  esp32s3_configgpio(GPIO_EXP_IRQ_PIN, INPUT_FUNCTION_2 | PULLUP);
#endif

  /* Initialize I2C bus */

  i2c = esp32s3_i2cbus_initialize(0);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize I2C bus\n");
      return -ENODEV;
    }

  /* Configure XL9555 */

  config.address   = GPIO_EXP_ADDR;
  config.frequency = GPIO_EXP_FREQ;

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  config.attach = esp32s3_gpioexp_attach;
  config.enable = esp32s3_gpioexp_enable;
#endif

  /* Initialize XL9555 driver */

  ioe = xl9555_int_initialize(i2c, &config);
  if (ioe == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize XL9555 driver\n");
      ret = -ENODEV;
      goto errout_i2c;
    }

  g_gpioexp.ioe = ioe;

  /* Register all 16 pins as GPIO devices */

  for (i = 0; i < 16; i++)
    {
      char name[16];

      /* Set pin direction to input by default */

      ret = IOEXP_SETDIRECTION(ioe, i, IOEXPANDER_DIRECTION_IN_PULLUP);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to set direction for pin %d: %d\n",
                 i, ret);
          continue;
        }

      /* Register pin as GPIO device with gpio_exp prefix */

      snprintf(name, sizeof(name), "gpio_exp%d", i);
      ret = gpio_lower_half_byname(ioe, i, GPIO_INPUT_PIN_PULLUP, name);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: gpio_lower_half_byname failed for pin %d: %d\n",
                 i, ret);
        }
    }

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  /* Enable interrupt */

  esp32s3_gpioirqenable(g_gpioexp.irq, RISING);
#endif

  syslog(LOG_INFO, "GPIO expander initialized\n");
  return OK;

errout_i2c:
  esp32s3_i2cbus_uninitialize(i2c);
  return ret;
}

#endif /* CONFIG_ESP32S3_BOSS1_CX_GPIO_EXP */
