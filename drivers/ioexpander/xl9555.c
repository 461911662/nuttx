/****************************************************************************
 * drivers/ioexpander/xl9555.c
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
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <string.h>

#include <nuttx/irq.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/kmalloc.h>
#include <nuttx/ioexpander/ioexpander.h>

#include "xl9555.h"

#if defined(CONFIG_IOEXPANDER_XL9555)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_I2C
#  error I2C support is required (CONFIG_I2C)
#endif

#define XL9555_REG_INPUT0      0x00
#define XL9555_REG_INPUT1      0x01
#define XL9555_REG_OUTPUT0     0x02
#define XL9555_REG_OUTPUT1     0x03
#define XL9555_REG_POLINV0     0x04
#define XL9555_REG_POLINV1     0x05
#define XL9555_REG_CONFIG0     0x06
#define XL9555_REG_CONFIG1     0x07

#define XL9555_REG_INPUT       XL9555_REG_INPUT0
#define XL9555_REG_OUTPUT      XL9555_REG_OUTPUT0
#define XL9555_REG_POLINV      XL9555_REG_POLINV0
#define XL9555_REG_CONFIG      XL9555_REG_CONFIG0

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static inline int xl9555_write(FAR struct xl9555_dev_s *xl,
                               FAR const uint8_t *wbuffer, int wbuflen);
static inline int xl9555_writeread(FAR struct xl9555_dev_s *xl,
                                   FAR const uint8_t *wbuffer, int wbuflen,
                                   FAR uint8_t *rbuffer, int rbuflen);
static int xl9555_direction(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                            int dir);
static int xl9555_option(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                         int opt, FAR void *val);
static int xl9555_writepin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                           bool value);
static int xl9555_readpin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                          FAR bool *value);
static int xl9555_readbuf(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                          FAR bool *value);

#ifdef CONFIG_IOEXPANDER_MULTIPIN
static int xl9555_multiwritepin(FAR struct ioexpander_dev_s *dev,
                                FAR const uint8_t *pins,
                                FAR const bool *values, int count);
static int xl9555_multireadpin(FAR struct ioexpander_dev_s *dev,
                               FAR const uint8_t *pins, FAR bool *values,
                               int count);
static int xl9555_multireadbuf(FAR struct ioexpander_dev_s *dev,
                               FAR const uint8_t *pins, FAR bool *values,
                               int count);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_XL9555_MULTIPLE
static struct xl9555_dev_s g_xl9555;
#else
static struct xl9555_dev_s *g_xl9555list;
#endif

static const struct ioexpander_ops_s g_xl9555_ops =
{
  xl9555_direction,
  xl9555_option,
  xl9555_writepin,
  xl9555_readpin,
  xl9555_readbuf
#ifdef CONFIG_IOEXPANDER_MULTIPIN
  , xl9555_multiwritepin
  , xl9555_multireadpin
  , xl9555_multireadbuf
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline int xl9555_write(FAR struct xl9555_dev_s *xl,
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

static inline int xl9555_writeread(FAR struct xl9555_dev_s *xl,
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

static int xl9555_setbit(FAR struct xl9555_dev_s *xl, uint8_t addr,
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

#ifdef CONFIG_XL9555_SHADOW_MODE
  buf[1] = xl->sreg[addr];
#else
  ret = xl9555_writeread(xl, &buf[0], 1, &buf[1], 1);
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

#ifdef CONFIG_XL9555_SHADOW_MODE
  xl->sreg[addr] = buf[1];
#endif

  ret = xl9555_write(xl, buf, 2);

#ifdef CONFIG_XL9555_RETRY
  if (ret != OK)
    {
      ret = xl9555_write(xl, buf, 2);
    }
#endif

  return ret;
}

static int xl9555_getbit(FAR struct xl9555_dev_s *xl, uint8_t addr,
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
      addr += 1;
      pin -= 8;
    }

  ret = xl9555_writeread(xl, &addr, 1, &buf, 1);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_XL9555_SHADOW_MODE
  xl->sreg[addr] = buf;
#endif

  *val = (buf >> pin) & 1;
  return OK;
}

static int xl9555_direction(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                            int direction)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
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

  ret = xl9555_setbit(xl, XL9555_REG_CONFIG, pin,
                      (direction == IOEXPANDER_DIRECTION_IN));
  nxmutex_unlock(&xl->lock);
  return ret;
}

static int xl9555_option(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                         int opt, FAR void *value)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
  int ret = -EINVAL;

  if (opt == IOEXPANDER_OPTION_INVERT)
    {
      ret = nxmutex_lock(&xl->lock);
      if (ret < 0)
        {
          return ret;
        }

      ret = xl9555_setbit(xl, XL9555_REG_POLINV, pin,
                          ((uintptr_t)value == IOEXPANDER_VAL_INVERT));
      nxmutex_unlock(&xl->lock);
    }

  return ret;
}

static int xl9555_writepin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                           bool value)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_setbit(xl, XL9555_REG_OUTPUT, pin, value);
  nxmutex_unlock(&xl->lock);
  return ret;
}

static int xl9555_readpin(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                          FAR bool *value)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_getbit(xl, XL9555_REG_INPUT, pin, value);
  nxmutex_unlock(&xl->lock);
  return ret;
}

static int xl9555_readbuf(FAR struct ioexpander_dev_s *dev, uint8_t pin,
                          FAR bool *value)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_getbit(xl, XL9555_REG_OUTPUT, pin, value);
  nxmutex_unlock(&xl->lock);
  return ret;
}

#ifdef CONFIG_IOEXPANDER_MULTIPIN

static int xl9555_getmultibits(FAR struct xl9555_dev_s *xl, uint8_t addr,
                               FAR const uint8_t *pins, FAR bool *values,
                               int count)
{
  uint8_t buf[2];
  int ret = OK;
  int i;
  int index;
  int pin;

  ret = xl9555_writeread(xl, &addr, 1, buf, 2);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_XL9555_SHADOW_MODE
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

static int xl9555_multiwritepin(FAR struct ioexpander_dev_s *dev,
                                FAR const uint8_t *pins,
                                FAR const bool *values, int count)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
  uint8_t addr = XL9555_REG_OUTPUT;
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

#ifndef CONFIG_XL9555_SHADOW_MODE
  ret = xl9555_writeread(xl, &addr, 1, &buf[1], 2);
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

#ifdef CONFIG_XL9555_SHADOW_MODE
  xl->sreg[addr]     = buf[1];
  xl->sreg[addr + 1] = buf[2];
#endif

  ret = xl9555_write(xl, buf, 3);

  nxmutex_unlock(&xl->lock);
  return ret;
}

static int xl9555_multireadpin(FAR struct ioexpander_dev_s *dev,
                                FAR const uint8_t *pins, FAR bool *values,
                                int count)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_getmultibits(xl, XL9555_REG_INPUT, pins, values, count);
  nxmutex_unlock(&xl->lock);
  return ret;
}

static int xl9555_multireadbuf(FAR struct ioexpander_dev_s *dev,
                                FAR const uint8_t *pins, FAR bool *values,
                                int count)
{
  FAR struct xl9555_dev_s *xl = (FAR struct xl9555_dev_s *)dev;
  int ret;

  ret = nxmutex_lock(&xl->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = xl9555_getmultibits(xl, XL9555_REG_OUTPUT, pins, values, count);
  nxmutex_unlock(&xl->lock);
  return ret;
}

#endif

#ifdef CONFIG_XL9555_SHADOW_MODE
/****************************************************************************
 * Name: xl9555_update_shadow
 *
 * Description:
 *   Update shadow registers from hardware.
 *   Reads all registers (0x00-0x07) and stores in sreg[].
 *
 ****************************************************************************/

static void xl9555_update_shadow(FAR struct xl9555_dev_s *xl)
{
  uint8_t addr;
  uint8_t buf[2];
  int ret;

  for (addr = 0x00; addr <= 0x07; addr += 2)
    {
      ret = xl9555_writeread(xl, &addr, 1, buf, 2);
      if (ret >= 0)
        {
          xl->sreg[addr]     = buf[0];
          xl->sreg[addr + 1] = buf[1];
        }
    }
}
#endif

/****************************************************************************
 * Name: xl9555_self_test
 *
 * Description:
 *   Self-test function to verify XL9555 registers default values.
 *
 * Input Parameters:
 *   xl  - Pointer to the xl9555 device structure
 *
 ****************************************************************************/

#ifdef CONFIG_DEBUG_INFO
static void xl9555_self_test(FAR struct xl9555_dev_s *xl)
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

  syslog(LOG_INFO, "========== XL9555 Self Test ==========\n");

  for (i = 0; i < 8; i++)
    {
      addr = i;
      ret = xl9555_writeread(xl, &addr, 1, buf, 2);
      if (ret < 0)
        {
          syslog(LOG_ERR, "XL9555 Self Test: Failed to read %s\n", reg_name[i]);
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
static void xl9555_self_test(FAR struct xl9555_dev_s *xl)
{
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct xl9555_dev_s *xl9555_initialize(FAR struct i2c_master_s *i2cdev,
                                       FAR struct xl9555_config_s *config)
{
  FAR struct xl9555_dev_s *xl;
  FAR struct xl9555_config_s *cfg;

  DEBUGASSERT(i2cdev != NULL && config != NULL);

#ifdef CONFIG_XL9555_MULTIPLE
  xl = (FAR struct xl9555_dev_s *)
    kmm_zalloc(sizeof(struct xl9555_dev_s));
  if (!xl)
    {
      return NULL;
    }

  xl->flink = g_xl9555list;
  g_xl9555list = xl;
#else
  xl = &g_xl9555;
#endif

  cfg = (FAR struct xl9555_config_s *)kmm_zalloc(sizeof(struct xl9555_config_s));
  if (!cfg)
    {
#ifdef CONFIG_XL9555_MULTIPLE
      kmm_free(xl);
#endif
      return NULL;
    }

  memcpy(cfg, config, sizeof(struct xl9555_config_s));
  xl->config = cfg;
  xl->i2c = i2cdev;
  nxmutex_init(&xl->lock);

  xl->dev.ops = &g_xl9555_ops;

#ifdef CONFIG_XL9555_SHADOW_MODE
  xl9555_update_shadow(xl);
#endif

  xl9555_self_test(xl);

  return xl;
}

#endif /* CONFIG_IOEXPANDER_XL9555 */
