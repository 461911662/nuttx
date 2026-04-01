/****************************************************************************
 * drivers/ioexpander/xl9555_int.c
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
 * References:
 *   XL9555 Datasheet - 16-bit I2C I/O expander with interrupt
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/kmalloc.h>
#include <nuttx/ioexpander/ioexpander.h>

#include "xl9555_int.h"

#if defined(CONFIG_IOEXPANDER_XL9555_INT)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_I2C
#  error I2C support is required (CONFIG_I2C)
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static inline int xl9555_int_write(FAR struct xl9555_int_dev_s *xl,
                                   FAR const uint8_t *wbuffer, int wbuflen);
static inline int xl9555_int_writeread(FAR struct xl9555_int_dev_s *xl,
                                       FAR const uint8_t *wbuffer, int wbuflen,
                                       FAR uint8_t *rbuffer, int rbuflen);
static int xl9555_int_direction(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                                int dir);
static int xl9555_int_option(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                             int opt, FAR void *value);
static int xl9555_int_writepin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                               bool value);
static int xl9555_int_readpin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                              FAR bool *value);
static int xl9555_int_readbuf(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                              FAR bool *value);
#ifdef CONFIG_IOEXPANDER_MULTIPIN
static int xl9555_int_multiwritepin(FAR struct ioexpander_dev_s *dev,
                                    FAR const uint8_t *pins,
                                    FAR const bool *values, int count);
static int xl9555_int_multireadpin(FAR struct ioexpander_dev_s *dev,
                                   FAR const uint8_t *pins, FAR bool *values,
                                   int count);
static int xl9555_int_multireadbuf(FAR struct ioexpander_dev_s *dev,
                                   FAR const uint8_t *pins, FAR bool *values,
                                   int count);
#endif
#ifdef CONFIG_IOEXPANDER_INT_ENABLE
static FAR void *xl9555_int_attach(FAR struct ioexpander_dev_s *dev,
                                   ioe_pinset_t pinset,
                                   ioe_callback_t callback, FAR void *arg);
static int xl9555_int_detach(FAR struct ioexpander_dev_s *dev,
                             FAR void *handle);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_XL9555_INT_MULTIPLE
static struct xl9555_int_dev_s g_xl9555_int;
#else
static struct xl9555_int_dev_s *g_xl9555_int_list;
#endif

static const struct ioexpander_ops_s g_xl9555_int_ops =
{
  xl9555_int_direction,
  xl9555_int_option,
  xl9555_int_writepin,
  xl9555_int_readpin,
  xl9555_int_readbuf
#ifdef CONFIG_IOEXPANDER_MULTIPIN
  , xl9555_int_multiwritepin
  , xl9555_int_multireadpin
  , xl9555_int_multireadbuf
#endif
#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  , xl9555_int_attach
  , xl9555_int_detach
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: xl9555_int_write
 *
 * Description:
 *   Write to the I2C device.
 *
 ****************************************************************************/

static inline int xl9555_int_write(FAR struct xl9555_int_dev_s *xl,
                                   FAR const uint8_t *wbuffer, int wbuflen)
{
  struct i2c_msg_s msg;
  int ret;

  msg.frequency = xl->config->frequency;
  msg.addr      = xl->config->address;
  msg.flags     = 0;
  msg.buffer    = (FAR uint8_t *)wbuffer;
  msg.length    = wbuflen;

  ret = I2C_TRANSFER(xl->i2c, &msg, 1);
  return (ret >= 0) ? OK : ret;
}

/****************************************************************************
 * Name: xl9555_int_writeread
 *
 * Description:
 *   Write to then read from the I2C device.
 *
 ****************************************************************************/

static inline int xl9555_int_writeread(FAR struct xl9555_int_dev_s *xl,
                                       FAR const uint8_t *wbuffer, int wbuflen,
                                       FAR uint8_t *rbuffer, int rbuflen)
{
  struct i2c_config_s config;

  config.frequency = xl->config->frequency;
  config.address   = xl->config->address;
  config.addrlen   = 7;

  return i2c_writeread(xl->i2c, &config, wbuffer, wbuflen,
                       rbuffer, rbuflen);
}

/****************************************************************************
 * Name: xl9555_int_setbit
 *
 * Description:
 *   Write a bit in a register pair.
 *
 ****************************************************************************/

static int xl9555_int_setbit(FAR struct xl9555_int_dev_s *xl, uint8_t addr,
                             uint8_t pin, bool bitval)
{
  uint8_t buf[2];
  int ret;

  if (pin > 15)
    {
      return -ENXIO;
    }
  else if (pin > 7)
    {
      addr++;
      pin -= 8;
    }

  buf[0] = addr;

#ifdef CONFIG_XL9555_INT_SHADOW_MODE
  buf[1] = xl->sreg[addr];
#else
  ret = xl9555_int_writeread(xl, &buf[0], 1, &buf[1], 1);
  if (ret < 0)
    {
      return ret;
    }
#endif

  if (bitval)
    {
      buf[1] |= (1 << pin);
    }
  else
    {
      buf[1] &= ~(1 << pin);
    }

#ifdef CONFIG_XL9555_INT_SHADOW_MODE
  xl->sreg[addr] = buf[1];
#endif

  ret = xl9555_int_write(xl, buf, 2);

#ifdef CONFIG_XL9555_INT_RETRY
  if (ret != OK)
    {
      ret = xl9555_int_write(xl, buf, 2);
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: xl9555_int_getbit
 *
 * Description:
 *   Get a bit from a register pair.
 *
 ****************************************************************************/

static int xl9555_int_getbit(FAR struct xl9555_int_dev_s *xl, uint8_t addr,
                             uint8_t pin, FAR bool *val)
{
  uint8_t buf;
  int ret;

  if (pin > 15)
    {
      return -ENXIO;
    }
  else if (pin > 7)
    {
      addr++;
      pin -= 8;
    }

  ret = xl9555_int_writeread(xl, &addr, 1, &buf, 1);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_XL9555_INT_SHADOW_MODE
  xl->sreg[addr] = buf;
#endif

  *val = (buf >> pin) & 1;
  return OK;
}

/****************************************************************************
 * Name: xl9555_int_direction
 *
 * Description:
 *   Set the direction of an ioexpander pin.
 *
 ****************************************************************************/

static int xl9555_int_direction(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                                int direction)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  int ret;

  if (direction != IOEXPANDER_DIRECTION_IN &&
      direction != IOEXPANDER_DIRECTION_OUT)
    {
      return -EINVAL;
    }

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_int_setbit(xl, XL9555_INT_REG_CONFIG, pin,
                          (direction == IOEXPANDER_DIRECTION_IN));
  nxmutex_unlock(&xl->lock);
  return ret;
}

/****************************************************************************
 * Name: xl9555_int_option
 *
 * Description:
 *   Set pin options.
 *
 ****************************************************************************/

static int xl9555_int_option(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                             int opt, FAR void *value)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  int ret = -EINVAL;

  if (opt == IOEXPANDER_OPTION_INVERT)
    {
      ret = nxmutex_lock(&xl->lock);
      if (ret < 0)
        {
          return ret;
        }

      ret = xl9555_int_setbit(xl, XL9555_INT_REG_POLINV, pin,
                              ((uintptr_t)value == IOEXPANDER_VAL_INVERT));
      nxmutex_unlock(&xl->lock);
    }

  return ret;
}

/****************************************************************************
 * Name: xl9555_int_writepin
 *
 * Description:
 *   Set the pin level.
 *
 ****************************************************************************/

static int xl9555_int_writepin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                               bool value)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_int_setbit(xl, XL9555_INT_REG_OUTPUT, pin, value);
  nxmutex_unlock(&xl->lock);
  return ret;
}

/****************************************************************************
 * Name: xl9555_int_readpin
 *
 * Description:
 *   Read the actual PIN level.
 *
 ****************************************************************************/

static int xl9555_int_readpin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                              FAR bool *value)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_int_getbit(xl, XL9555_INT_REG_INPUT, pin, value);
  nxmutex_unlock(&xl->lock);
  return ret;
}

/****************************************************************************
 * Name: xl9555_int_readbuf
 *
 * Description:
 *   Read the buffered pin level.
 *
 ****************************************************************************/

static int xl9555_int_readbuf(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                              FAR bool *value)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_int_getbit(xl, XL9555_INT_REG_OUTPUT, pin, value);
  nxmutex_unlock(&xl->lock);
  return ret;
}

#ifdef CONFIG_IOEXPANDER_MULTIPIN

/****************************************************************************
 * Name: xl9555_int_getmultibits
 *
 * Description:
 *   Read multiple bits from XL9555 registers.
 *
 ****************************************************************************/

static int xl9555_int_getmultibits(FAR struct xl9555_int_dev_s *xl, uint8_t addr,
                                   FAR const uint8_t *pins, FAR bool *values,
                                   int count)
{
  uint8_t buf[2];
  int ret = OK;
  int i;
  int index;
  int pin;

  ret = xl9555_int_writeread(xl, &addr, 1, buf, 2);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_XL9555_INT_SHADOW_MODE
  xl->sreg[addr]     = buf[0];
  xl->sreg[addr + 1] = buf[1];
#endif

  for (i = 0; i < count; i++)
    {
      index = 0;
      pin   = pins[i];
      if (pin > 15)
        {
          return -ENXIO;
        }
      else if (pin > 7)
        {
          index = 1;
          pin  -= 8;
        }

      values[i] = (buf[index] >> pin) & 1;
    }

  return OK;
}

/****************************************************************************
 * Name: xl9555_int_multiwritepin
 *
 * Description:
 *   Set the pin level for multiple pins.
 *
 ****************************************************************************/

static int xl9555_int_multiwritepin(FAR struct ioexpander_dev_s *dev,
                                    FAR const uint8_t *pins,
                                    FAR const bool *values, int count)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  uint8_t addr = XL9555_INT_REG_OUTPUT;
  uint8_t buf[3];
  int ret;
  int i;
  int index;
  int pin;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

#ifndef CONFIG_XL9555_INT_SHADOW_MODE
  ret = xl9555_int_writeread(xl, &addr, 1, &buf[1], 2);
  if (ret < 0)
    {
      nxmutex_unlock(&xl->lock);
      return ret;
    }
#else
  buf[1] = xl->sreg[addr];
  buf[2] = xl->sreg[addr + 1];
#endif

  for (i = 0; i < count; i++)
    {
      index = 1;
      pin = pins[i];
      if (pin > 15)
        {
          nxmutex_unlock(&xl->lock);
          return -ENXIO;
        }
      else if (pin > 7)
        {
          index = 2;
          pin -= 8;
        }

      if (values[i])
        {
          buf[index] |= (1 << pin);
        }
      else
        {
          buf[index] &= ~(1 << pin);
        }
    }

  buf[0] = addr;

#ifdef CONFIG_XL9555_INT_SHADOW_MODE
  xl->sreg[addr]     = buf[1];
  xl->sreg[addr + 1] = buf[2];
#endif

  ret = xl9555_int_write(xl, buf, 3);

  nxmutex_unlock(&xl->lock);
  return ret;
}

/****************************************************************************
 * Name: xl9555_int_multireadpin
 *
 * Description:
 *   Read the actual level for multiple pins.
 *
 ****************************************************************************/

static int xl9555_int_multireadpin(FAR struct ioexpander_dev_s *dev,
                                   FAR const uint8_t *pins, FAR bool *values,
                                   int count)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_int_getmultibits(xl, XL9555_INT_REG_INPUT,
                                pins, values, count);
  nxmutex_unlock(&xl->lock);
  return ret;
}

/****************************************************************************
 * Name: xl9555_int_multireadbuf
 *
 * Description:
 *   Read the buffered level of multiple pins.
 *
 ****************************************************************************/

static int xl9555_int_multireadbuf(FAR struct ioexpander_dev_s *dev,
                                   FAR const uint8_t *pins, FAR bool *values,
                                   int count)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_int_getmultibits(xl, XL9555_INT_REG_OUTPUT,
                                pins, values, count);
  nxmutex_unlock(&xl->lock);
  return ret;
}

#endif

#ifdef CONFIG_XL9555_INT_SHADOW_MODE
/****************************************************************************
 * Name: xl9555_int_update_shadow
 *
 * Description:
 *   Update shadow registers from hardware.
 *
 ****************************************************************************/

static void xl9555_int_update_shadow(FAR struct xl9555_int_dev_s *xl)
{
  uint8_t addr;
  uint8_t buf[2];
  int ret;

  for (addr = 0x00; addr <= 0x07; addr += 2)
    {
      ret = xl9555_int_writeread(xl, &addr, 1, buf, 2);
      if (ret >= 0)
        {
          xl->sreg[addr]     = buf[0];
          xl->sreg[addr + 1] = buf[1];
        }
    }
}
#endif

#ifdef CONFIG_DEBUG_INFO
/****************************************************************************
 * Name: xl9555_int_self_test
 *
 * Description:
 *   Self-test function to verify XL9555 registers default values.
 *
 ****************************************************************************/

static void xl9555_int_self_test(FAR struct xl9555_int_dev_s *xl)
{
  uint8_t addr;
  uint8_t buf[2];
  int ret;
  int i;

  const uint8_t expected[8] =
    {
      0x00,  /* INPUT0  - 0x0x (ignore bit 0) */
      0x00,  /* INPUT1  - 0x0x (ignore bit 0) */
      0xff,  /* OUTPUT0 - 0xff */
      0xff,  /* OUTPUT1 - 0xff */
      0x00,  /* POLINV0 - 0x00 */
      0x00,  /* POLINV1 - 0x00 */
      0xff,  /* CONFIG0 - 0xff */
      0xff   /* CONFIG1 - 0xff */
    };

  const char *reg_name[8] =
    {
      "INPUT0 ", "INPUT1 ", "OUTPUT0", "OUTPUT1",
      "POLINV0", "POLINV1", "CONFIG0", "CONFIG1"
    };

  syslog(LOG_INFO, "========== XL9555_INT Self Test ==========\n");

  for (i = 0; i < 8; i++)
    {
      addr = i;
      ret = xl9555_int_writeread(xl, &addr, 1, buf, 2);
      if (ret < 0)
        {
          syslog(LOG_ERR, "XL9555_INT Self Test: Failed to read %s\n", reg_name[i]);
          continue;
        }

      if (i < 2)
        {
          syslog(LOG_INFO, "%s: 0x%02x (expected 0x%02x, ignore)\n",
                 reg_name[i], buf[0], expected[i]);
        }
      else
        {
          if (buf[0] == expected[i])
            {
              syslog(LOG_INFO, "%s: 0x%02x (expected 0x%02x) PASS\n",
                     reg_name[i], buf[0], expected[i]);
            }
          else
            {
              syslog(LOG_ERR, "%s: 0x%02x (expected 0x%02x) FAIL\n",
                     reg_name[i], buf[0], expected[i]);
            }
        }
    }

  syslog(LOG_INFO, "====================================\n");
}
#else
static void xl9555_int_self_test(FAR struct xl9555_int_dev_s *xl)
{
}
#endif

#ifdef CONFIG_IOEXPANDER_INT_ENABLE

/****************************************************************************
 * Name: xl9555_int_attach
 *
 * Description:
 *   Attach and enable a pin interrupt callback function.
 *
 ****************************************************************************/

static FAR void *xl9555_int_attach(FAR struct ioexpander_dev_s *dev,
                                   ioe_pinset_t pinset,
                                   ioe_callback_t callback,
                                   FAR void *arg)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  FAR void *handle = NULL;
  int i;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return NULL;
    }

  for (i = 0; i < CONFIG_XL9555_INT_NCALLBACKS; i++)
    {
      if (xl->cb[i].cbfunc == NULL)
        {
          xl->cb[i].pinset = pinset;
          xl->cb[i].cbfunc = callback;
          xl->cb[i].cbarg  = arg;
          handle            = &xl->cb[i];
          break;
        }
    }

  nxmutex_unlock(&xl->lock);
  return handle;
}

/****************************************************************************
 * Name: xl9555_int_detach
 *
 * Description:
 *   Detach and disable a pin interrupt callback function.
 *
 ****************************************************************************/

static int xl9555_int_detach(FAR struct ioexpander_dev_s *dev,
                             FAR void *handle)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)dev;
  FAR struct xl9555_int_callback_s *cb =
    (FAR struct xl9555_int_callback_s *)handle;

  DEBUGASSERT(xl != NULL && cb != NULL);
  DEBUGASSERT((uintptr_t)cb >= (uintptr_t)&xl->cb[0] &&
              (uintptr_t)cb <=
              (uintptr_t)&xl->cb[CONFIG_XL9555_INT_NCALLBACKS - 1]);
  UNUSED(xl);

  cb->pinset = 0;
  cb->cbfunc = NULL;
  cb->cbarg  = NULL;
  return OK;
}

/****************************************************************************
 * Name: xl9555_int_irqworker
 *
 * Description:
 *   Handle GPIO interrupt events (this function actually executes in the
 *   context of the worker thread).
 *
 ****************************************************************************/

static void xl9555_int_irqworker(void *arg)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)arg;
  uint8_t addr = XL9555_INT_REG_INPUT;
  uint8_t buf[2];
  ioe_pinset_t pinset;
  ioe_pinset_t change;
  ioe_pinset_t press;
  ioe_pinset_t release;
  int ret;
  int i;

  ret = xl9555_int_writeread(xl, &addr, 1, buf, 2);
  if (ret == OK)
    {
#ifdef CONFIG_XL9555_INT_SHADOW_MODE
      xl->sreg[addr]     = buf[0];
      xl->sreg[addr + 1] = buf[1];
#endif
      pinset = ((unsigned int)buf[1] << 8) | buf[0];

      /* Calculate edge changes */
      change  = pinset ^ xl->last_pinset;
      press   = change & pinset;
      release = change & ~pinset;

      printf("xl9555_int_irqworker: pinset=0x%04x change=0x%04x press=0x%04x release=0x%04x\n",
             pinset, change, press, release);

      for (i = 0; i < CONFIG_XL9555_INT_NCALLBACKS; i++)
        {
          if (xl->cb[i].cbfunc != NULL)
            {
              ioe_pinset_t match_press = press & xl->cb[i].pinset;
              if (match_press != 0)
                {
                  printf("  -> press callback, match=0x%04x\n", match_press);
                  xl->cb[i].cbfunc(&xl->dev, match_press, xl->cb[i].cbarg);
                }

              ioe_pinset_t match_release = release & xl->cb[i].pinset;
              if (match_release != 0)
                {
                  printf("  -> release callback, match=0x%04x\n", match_release);
                  xl->cb[i].cbfunc(&xl->dev, match_release, xl->cb[i].cbarg);
                }
            }
        }

      xl->last_pinset = pinset;
    }

  xl->config->enable(xl->config, TRUE);
}

/****************************************************************************
 * Name: xl9555_int_interrupt
 *
 * Description:
 *   Handle GPIO interrupt events (this function executes in the
 *   context of the interrupt).
 *
 ****************************************************************************/

static int xl9555_int_interrupt(int irq, FAR void *context, FAR void *arg)
{
  FAR struct xl9555_int_dev_s *xl = (FAR struct xl9555_int_dev_s *)arg;

  if (work_available(&xl->work))
    {
      xl->config->enable(xl->config, FALSE);
      work_queue(HPWORK, &xl->work, xl9555_int_irqworker,
                 (FAR void *)xl, 0);
    }

  return OK;
}

#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: xl9555_int_initialize
 *
 * Description:
 *   Initialize a XL9555 I2C device.
 *
 ****************************************************************************/

FAR struct ioexpander_dev_s *xl9555_int_initialize(
                              FAR struct i2c_master_s *i2cdev,
                              FAR struct xl9555_int_config_s *config)
{
  FAR struct xl9555_int_dev_s *xl;
  FAR struct xl9555_int_config_s *cfg;

  DEBUGASSERT(i2cdev != NULL && config != NULL);

#ifdef CONFIG_XL9555_INT_MULTIPLE
  xl = (FAR struct xl9555_int_dev_s *)
    kmm_zalloc(sizeof(struct xl9555_int_dev_s));
  if (!xl)
    {
      return NULL;
    }

  xl->flink = g_xl9555_int_list;
  g_xl9555_int_list = xl;
#else
  xl = &g_xl9555_int;
#endif

  cfg = (FAR struct xl9555_int_config_s *)
    kmm_zalloc(sizeof(struct xl9555_int_config_s));
  if (!cfg)
    {
#ifdef CONFIG_XL9555_INT_MULTIPLE
      kmm_free(xl);
#endif
      return NULL;
    }

  memcpy(cfg, config, sizeof(struct xl9555_int_config_s));
  xl->config = cfg;
  xl->i2c = i2cdev;
  nxmutex_init(&xl->lock);

  xl->dev.ops = &g_xl9555_int_ops;

#ifdef CONFIG_XL9555_INT_SHADOW_MODE
  xl9555_int_update_shadow(xl);
#endif

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  xl->config->attach(xl->config, xl9555_int_interrupt, xl);
  xl->config->enable(xl->config, TRUE);
#endif

  xl9555_int_self_test(xl);

  return &xl->dev;
}

#endif /* CONFIG_IOEXPANDER_XL9555_INT */
