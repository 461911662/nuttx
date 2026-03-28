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

#include <sys/types.h>
#include <nuttx/irq.h>
#include <arch/irq.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/ioexpander/gpio.h>

#include <arch/board/board.h>

#include "esp32s3-boss1-cx.h"
#include "esp32s3_gpio.h"
#include "hardware/esp32s3_gpio_sigmap.h"

#if defined(CONFIG_DEV_GPIO)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Output pins */

#ifndef BOARD_NGPIOOUT
#  define BOARD_NGPIOOUT 0
#endif

/* Input pins */

#ifndef BOARD_NGPIOIN
#  define BOARD_NGPIOIN 0
#endif

/* Interrupt pins */

#ifndef BOARD_NGPIOINT
#  define BOARD_NGPIOINT 0
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct esp32s3gpio_dev_s
{
  struct gpio_dev_s gpio;
  uint8_t id;
};

struct esp32s3gpint_dev_s
{
  struct esp32s3gpio_dev_s esp32s3gpio;
  pin_interrupt_t callback;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#if BOARD_NGPIOOUT > 0
static int gpout_read(struct gpio_dev_s *dev, bool *value);
static int gpout_write(struct gpio_dev_s *dev, bool value);
#endif

#if BOARD_NGPIOIN > 0
static int gpin_read(struct gpio_dev_s *dev, bool *value);
#endif

#if defined(CONFIG_ESP32S3_GPIO_IRQ) && BOARD_NGPIOINT > 0
static int gpint_read(struct gpio_dev_s *dev, bool *value);
static int gpint_attach(struct gpio_dev_s *dev, pin_interrupt_t callback);
static int gpint_enable(struct gpio_dev_s *dev, bool enable);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Output GPIO pins */

#if BOARD_NGPIOOUT > 0
static const uint32_t g_gpiooutputs[BOARD_NGPIOOUT] =
{
  0  /* Placeholder - to be defined in board.h */
};

static struct esp32s3gpio_dev_s g_gpout[BOARD_NGPIOOUT];

static const struct gpio_operations_s g_gpout_ops =
{
  .go_read   = gpout_read,
  .go_write  = gpout_write,
  .go_attach = NULL,
  .go_enable = NULL,
};
#endif

/* Input GPIO pins */

#if BOARD_NGPIOIN > 0
static const uint32_t g_gpioinputs[BOARD_NGPIOIN] =
{
  0  /* Placeholder - to be defined in board.h */
};

static struct esp32s3gpio_dev_s g_gpin[BOARD_NGPIOIN];

static const struct gpio_operations_s g_gpin_ops =
{
  .go_read   = gpin_read,
  .go_write  = NULL,
  .go_attach = NULL,
  .go_enable = NULL,
};
#endif

/* Interrupt GPIO pins */

#if defined(CONFIG_ESP32S3_GPIO_IRQ) && BOARD_NGPIOINT > 0
static const uint32_t g_gpiointinputs[BOARD_NGPIOINT] =
{
  0  /* Placeholder - to be defined in board.h */
};

static struct esp32s3gpint_dev_s g_gpint[BOARD_NGPIOINT];

static const struct gpio_operations_s gpint_ops =
{
  .go_read   = gpint_read,
  .go_write  = NULL,
  .go_attach = gpint_attach,
  .go_enable = gpint_enable,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: gpout_read
 ****************************************************************************/

#if BOARD_NGPIOOUT > 0
static int gpout_read(struct gpio_dev_s *dev, bool *value)
{
  struct esp32s3gpio_dev_s *escdev = (struct esp32s3gpio_dev_s *)dev;

  DEBUGASSERT(escdev != NULL && value != NULL);
  DEBUGASSERT(escdev->id < BOARD_NGPIOOUT);

  *value = esp32s3_gpioread(g_gpiooutputs[escdev->id]);
  return OK;
}
#endif

/****************************************************************************
 * Name: gpout_write
 ****************************************************************************/

#if BOARD_NGPIOOUT > 0
static int gpout_write(struct gpio_dev_s *dev, bool value)
{
  struct esp32s3gpio_dev_s *escdev = (struct esp32s3gpio_dev_s *)dev;

  DEBUGASSERT(escdev != NULL);
  DEBUGASSERT(escdev->id < BOARD_NGPIOOUT);

  esp32s3_gpiowrite(g_gpiooutputs[escdev->id], value);
  return OK;
}
#endif

/****************************************************************************
 * Name: gpin_read
 ****************************************************************************/

#if BOARD_NGPIOIN > 0
static int gpin_read(struct gpio_dev_s *dev, bool *value)
{
  struct esp32s3gpio_dev_s *escdev = (struct esp32s3gpio_dev_s *)dev;

  DEBUGASSERT(escdev != NULL && value != NULL);
  DEBUGASSERT(escdev->id < BOARD_NGPIOIN);

  *value = esp32s3_gpioread(g_gpioinputs[escdev->id]);
  return OK;
}
#endif

/****************************************************************************
 * Name: esp32s3gpio_interrupt
 ****************************************************************************/

#if defined(CONFIG_ESP32S3_GPIO_IRQ) && BOARD_NGPIOINT > 0
static int esp32s3gpio_interrupt(int irq, void *context, void *arg)
{
  struct esp32s3gpint_dev_s *escdev = (struct esp32s3gpint_dev_s *)arg;

  DEBUGASSERT(escdev != NULL && escdev->callback != NULL);

  escdev->callback(&escdev->esp32s3gpio.gpio, escdev->esp32s3gpio.id);
  return OK;
}
#endif

/****************************************************************************
 * Name: gpint_read
 ****************************************************************************/

#if defined(CONFIG_ESP32S3_GPIO_IRQ) && BOARD_NGPIOINT > 0
static int gpint_read(struct gpio_dev_s *dev, bool *value)
{
  struct esp32s3gpint_dev_s *escdev = (struct esp32s3gpint_dev_s *)dev;

  DEBUGASSERT(escdev != NULL && value != NULL);
  DEBUGASSERT(escdev->esp32s3gpio.id < BOARD_NGPIOINT);

  *value = esp32s3_gpioread(g_gpiointinputs[escdev->esp32s3gpio.id]);
  return OK;
}
#endif

/****************************************************************************
 * Name: gpint_attach
 ****************************************************************************/

#if defined(CONFIG_ESP32S3_GPIO_IRQ) && BOARD_NGPIOINT > 0
static int gpint_attach(struct gpio_dev_s *dev, pin_interrupt_t callback)
{
  struct esp32s3gpint_dev_s *escdev =
    (struct esp32s3gpint_dev_s *)dev;
  int irq;
  int ret;

  DEBUGASSERT(escdev != NULL);
  DEBUGASSERT(escdev->esp32s3gpio.id < BOARD_NGPIOINT);

  irq = ESP32S3_PIN2IRQ(g_gpiointinputs[escdev->esp32s3gpio.id]);

  /* Make sure the interrupt is disabled */

  esp32s3_gpioirqdisable(irq);
  ret = irq_attach(irq, esp32s3gpio_interrupt,
                   &g_gpint[escdev->esp32s3gpio.id]);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: gpint_attach() failed: %d\n", ret);
      return ret;
    }

  escdev->callback = callback;
  return OK;
}
#endif

/****************************************************************************
 * Name: gpint_enable
 ****************************************************************************/

#if defined(CONFIG_ESP32S3_GPIO_IRQ) && BOARD_NGPIOINT > 0
static int gpint_enable(struct gpio_dev_s *dev, bool enable)
{
  struct esp32s3gpint_dev_s *escdev =
    (struct esp32s3gpint_dev_s *)dev;
  int irq;

  DEBUGASSERT(escdev != NULL);
  DEBUGASSERT(escdev->esp32s3gpio.id < BOARD_NGPIOINT);

  irq = ESP32S3_PIN2IRQ(g_gpiointinputs[escdev->esp32s3gpio.id]);

  if (enable)
    {
      if (escdev->callback != NULL)
        {
          /* Configure the interrupt for rising edge */

          esp32s3_gpioirqenable(irq, RISING);
        }
    }
  else
    {
      esp32s3_gpioirqdisable(irq);
    }

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_gpio_initialize
 *
 * Description:
 *   Initialize GPIO driver for ESP32-S3 built-in GPIO pins.
 *
 ****************************************************************************/

int esp32s3_gpio_initialize(void)
{
  int pincount = 0;
  int i;
  int ret;

#if BOARD_NGPIOIN > 0
  for (i = 0; i < BOARD_NGPIOIN; i++)
    {
      /* Configure the pins that will be used as INPUT */

      esp32s3_configgpio(g_gpioinputs[i], INPUT_FUNCTION_2 | PULLUP);
      syslog(LOG_INFO, "Configuring GPIO %d\n", g_gpioinputs[i]);

      g_gpin[i].gpio.gp_pintype = GPIO_INPUT_PIN;
      g_gpin[i].gpio.gp_ops     = &g_gpin_ops;
      g_gpin[i].id              = i;

      ret = gpio_pin_register(&g_gpin[i].gpio, pincount);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: gpio_pin_register failed: %d\n", ret);
          return ret;
        }

      pincount++;
    }
#endif

#if BOARD_NGPIOOUT > 0
  for (i = 0; i < BOARD_NGPIOOUT; i++)
    {
      /* Configure the pins that will be used as output */

      esp32s3_gpio_matrix_out(g_gpiooutputs[i], SIG_GPIO_OUT_IDX, 0, 0);
      esp32s3_configgpio(g_gpiooutputs[i], OUTPUT_FUNCTION_2 |
                         INPUT_FUNCTION_2);
      esp32s3_gpiowrite(g_gpiooutputs[i], 0);

      g_gpout[i].gpio.gp_pintype = GPIO_OUTPUT_PIN;
      g_gpout[i].gpio.gp_ops     = &g_gpout_ops;
      g_gpout[i].id              = i;

      ret = gpio_pin_register(&g_gpout[i].gpio, pincount);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: gpio_pin_register failed: %d\n", ret);
          return ret;
        }

      pincount++;
    }
#endif

#if defined(CONFIG_ESP32S3_GPIO_IRQ) && BOARD_NGPIOINT > 0
  for (i = 0; i < BOARD_NGPIOINT; i++)
    {
      /* Configure the pins that will be used as interrupt input */

      esp32s3_configgpio(g_gpiointinputs[i], INPUT_FUNCTION_2 | PULLUP);

      g_gpint[i].esp32s3gpio.gpio.gp_pintype = GPIO_INTERRUPT_PIN;
      g_gpint[i].esp32s3gpio.gpio.gp_ops   = &gpint_ops;
      g_gpint[i].esp32s3gpio.id             = i;

      ret = gpio_pin_register(&g_gpint[i].esp32s3gpio.gpio, pincount);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: gpio_pin_register failed: %d\n", ret);
          return ret;
        }

      pincount++;
    }
#endif

  UNUSED(i);
  UNUSED(ret);
  return OK;
}

#endif /* CONFIG_DEV_GPIO && !CONFIG_GPIO_LOWER_HALF */
