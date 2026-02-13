/****************************************************************************
 * boards/xtensa/boss1-esp32s3/boss1/src/esp32s3_xl9555.c
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

#include <nuttx/config.h>

#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>

#include <nuttx/irq.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/ioexpander/xl9555.h>
#include <nuttx/kmalloc.h>
#include <nuttx/wqueue.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mutex.h>
#include <poll.h>

#include <arch/board/board.h>
#include "esp32s3_gpio.h"
#include "esp32s3_i2c.h"
#include "esp32s3-boss1.h"

#ifdef CONFIG_BOSS1_ESP32S3_XL9555

#if !defined(CONFIG_BOSS1_ESP32S3_I2C0) && !defined(CONFIG_BOSS1_ESP32S3_I2C1)
#  error "I2C must be enabled for XL9555 support"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV
struct xl9555_poll_s
{
  FAR struct pollfd *fds;
  FAR struct xl9555_poll_s *next;
};
#endif

struct esp32s3_xl9555_s
{
  FAR struct ioexpander_dev_s *ioe;
  struct work_s            work;
  int                      irq;
  gpio_intrtype_t          irq_type;
  bool                     intr_enabled;

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV
  mutex_t lock;
  struct xl9555_poll_s *poll_list;
#endif
};

struct esp32s3_xl9555_s *g_xl9555;

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV

/****************************************************************************
 * Character Device File Operations
 ****************************************************************************/

static int xl9555_dev_open(FAR struct file *filep);
static int xl9555_dev_close(FAR struct file *filep);
static ssize_t xl9555_dev_read(FAR struct file *filep, FAR char *buffer,
                               size_t buflen);
static ssize_t xl9555_dev_write(FAR struct file *filep, FAR const char *buffer,
                                size_t buflen);
static int xl9555_dev_ioctl(FAR struct file *filep, int cmd,
                            unsigned long arg);
static int xl9555_dev_poll(FAR struct file *filep,
                           FAR struct pollfd *fds, bool setup);

static const struct file_operations g_xl9555_devops =
{
  xl9555_dev_open,    /* open */
  xl9555_dev_close,   /* close */
  xl9555_dev_read,    /* read */
  xl9555_dev_write,   /* write */
  NULL,               /* seek */
  xl9555_dev_ioctl,   /* ioctl */
  NULL,               /* mmap */
  NULL,               /* truncate */
  xl9555_dev_poll     /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  , NULL              /* unlink */
#endif
};

#endif /* CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void esp32s3_xl9555_irqworker(void *arg)
{
  FAR struct esp32s3_xl9555_s *xl = (FAR struct esp32s3_xl9555_s *)arg;
  bool value;
  int i;

  for (i = 0; i < 16; i++)
    {
      IOEXP_READPIN(xl->ioe, i, &value);
    }

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV
  if (xl->poll_list)
    {
      poll_notify(&xl->poll_list->fds, 1, POLLIN);
    }
#endif

  if (xl->intr_enabled)
    {
      while (!esp32s3_gpioread(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_PIN))
        {
        }

      esp32s3_gpioirqenable(xl->irq, xl->irq_type);
    }
}

static int esp32s3_xl9555_interrupt(int irq, FAR void *context, FAR void *arg)
{
  FAR struct esp32s3_xl9555_s *xl = (FAR struct esp32s3_xl9555_s *)arg;

  if (work_available(&xl->work))
    {
      esp32s3_gpioirqdisable(xl->irq);
#ifdef CONFIG_BOSS1_ESP32S3_XL9555_INT_LPWORK
      work_queue(LPWORK, &xl->work, esp32s3_xl9555_irqworker, xl, 0);
#else
      work_queue(HPWORK, &xl->work, esp32s3_xl9555_irqworker, xl, 0);
#endif
    }

  return OK;
}

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV

/****************************************************************************
 * Character Device Functions
 ****************************************************************************/

static int xl9555_dev_open(FAR struct file *filep)
{
  FAR struct inode *inode;
  FAR struct esp32s3_xl9555_s *dev;
  int ret;

  inode = filep->f_inode;
  DEBUGASSERT(inode->i_private);
  dev = inode->i_private;

  ret = nxmutex_lock(&dev->lock);
  if (ret < 0)
    {
      return ret;
    }

  filep->f_priv = dev;

  nxmutex_unlock(&dev->lock);
  return OK;
}

static int xl9555_dev_close(FAR struct file *filep)
{
  filep->f_priv = NULL;
  return OK;
}

static ssize_t xl9555_dev_read(FAR struct file *filep, FAR char *buffer,
                               size_t buflen)
{
  return 0;
}

static ssize_t xl9555_dev_write(FAR struct file *filep, FAR const char *buffer,
                                size_t buflen)
{
  return 0;
}

static int xl9555_dev_ioctl(FAR struct file *filep, int cmd,
                            unsigned long arg)
{
  FAR struct esp32s3_xl9555_s *dev;
  int ret;

  dev = filep->f_priv;
  if (!dev)
    {
      return -ENODEV;
    }

  ret = nxmutex_lock(&dev->lock);
  if (ret < 0)
    {
      return ret;
    }

  switch (cmd)
    {
      case XL9555_IOC_SETPIN:
        {
          FAR struct xl9555_pin_s *pin = (FAR struct xl9555_pin_s *)arg;
          if (!pin || pin->pin >= 16)
            {
              ret = -EINVAL;
              break;
            }

          if (pin->direction == XL9555_DIRECTION_IN)
            {
              ret = IOEXP_SETDIRECTION(dev->ioe, pin->pin,
                                       IOEXPANDER_DIRECTION_IN);
            }
          else
            {
              ret = IOEXP_SETDIRECTION(dev->ioe, pin->pin,
                                       IOEXPANDER_DIRECTION_OUT);
              if (ret >= 0)
                {
                  ret = IOEXP_WRITEPIN(dev->ioe, pin->pin, pin->value);
                }
            }
        }
        break;

      case XL9555_IOC_READPIN:
        {
          FAR struct xl9555_pin_s *pin = (FAR struct xl9555_pin_s *)arg;
          if (!pin || pin->pin >= 16)
            {
              ret = -EINVAL;
              break;
            }

          ret = IOEXP_READPIN(dev->ioe, pin->pin, &pin->value);
        }
        break;

      case XL9555_IOC_WRITEPIN:
        {
          FAR struct xl9555_pin_s *pin = (FAR struct xl9555_pin_s *)arg;
          if (!pin || pin->pin >= 16)
            {
              ret = -EINVAL;
              break;
            }

          ret = IOEXP_WRITEPIN(dev->ioe, pin->pin, pin->value);
        }
        break;

      default:
        ret = -ENOTTY;
        break;
    }

  nxmutex_unlock(&dev->lock);
  return ret;
}

static int xl9555_dev_poll(FAR struct file *filep,
                           FAR struct pollfd *fds, bool setup)
{
  FAR struct esp32s3_xl9555_s *dev;
  FAR struct xl9555_poll_s *poll_entry;
  FAR struct xl9555_poll_s *poll_prev;
  int ret = OK;

  if (!fds)
    {
      return -EINVAL;
    }

  dev = filep->f_priv;
  if (!dev)
    {
      return -ENODEV;
    }

  if (setup)
    {
      poll_entry = kmm_zalloc(sizeof(struct xl9555_poll_s));
      if (!poll_entry)
        {
          return -ENOMEM;
        }

      poll_entry->fds = fds;
      poll_entry->next = dev->poll_list;
      dev->poll_list = poll_entry;

      poll_notify(&fds, 1, POLLIN);
    }
  else
    {
      poll_prev = NULL;
      poll_entry = dev->poll_list;

      while (poll_entry)
        {
          if (poll_entry->fds == fds)
            {
              if (poll_prev)
                {
                  poll_prev->next = poll_entry->next;
                }
              else
                {
                  dev->poll_list = poll_entry->next;
                }

              kmm_free(poll_entry);
              break;
            }

          poll_prev = poll_entry;
          poll_entry = poll_entry->next;
        }
    }

  return ret;
}

#endif /* CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int esp32s3_xl9555_initialize(void)
{
  FAR struct i2c_master_s *i2c;
  FAR struct ioexpander_dev_s *ioe;
  struct xl9555_config_s config;
  int ret;
  int port;
  int irq;

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV
  char devpath[32];
#endif

#if defined(CONFIG_BOSS1_ESP32S3_XL9555_I2C0)
  port = 0;
#elif defined(CONFIG_BOSS1_ESP32S3_XL9555_I2C1)
  port = 1;
#else
#  error "I2C port not selected"
#endif

  g_xl9555 = kmm_zalloc(sizeof(struct esp32s3_xl9555_s));
  if (!g_xl9555)
    {
      syslog(LOG_ERR, "ERROR: Failed to allocate XL9555 data\n");
      return -ENOMEM;
    }

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV
  nxmutex_init(&g_xl9555->lock);
#endif

  config.frequency = CONFIG_BOSS1_ESP32S3_XL9555_FREQUENCY;
  config.address   = CONFIG_BOSS1_ESP32S3_XL9555_ADDRESS;

  i2c = esp32s3_i2cbus_initialize(port);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize I2C%d\n", port);
      ret = -ENODEV;
      goto errout_free;
    }

  ioe = xl9555_initialize(i2c, &config);
  if (ioe == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize XL9555 driver\n");
      ret = -ENODEV;
      goto errout_i2c;
    }

  g_xl9555->ioe = ioe;

  irq = BOSS1_ESP32S3_PIN2IRQ(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_PIN);
  if (irq < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to get IRQ for pin %d\n",
             CONFIG_BOSS1_ESP32S3_XL9555_IRQ_PIN);
      ret = irq;
      goto errout_i2c;
    }

  g_xl9555->irq = irq;

  esp32s3_configgpio(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_PIN, INPUT_PULLUP);

#if defined(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_RISING)
  g_xl9555->irq_type = RISING;
#elif defined(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_FALLING)
  g_xl9555->irq_type = FALLING;
#elif defined(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_CHANGE)
  g_xl9555->irq_type = CHANGE;
#elif defined(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_LOW)
  g_xl9555->irq_type = ONLOW;
#elif defined(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_HIGH)
  g_xl9555->irq_type = ONHIGH;
#endif

  ret = irq_attach(irq, esp32s3_xl9555_interrupt, g_xl9555);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: irq_attach failed: %d\n", ret);
      goto errout_i2c;
    }

  g_xl9555->intr_enabled = true;
  esp32s3_gpioirqenable(irq, g_xl9555->irq_type);

#ifdef CONFIG_BOSS1_ESP32S3_XL9555_CHARDEV
  snprintf(devpath, sizeof(devpath), "/dev/xl9555%d", 0);
  ret = register_driver(devpath, &g_xl9555_devops, 0666, g_xl9555);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to register char dev: %d\n", ret);
      goto errout_i2c;
    }

  syslog(LOG_INFO, "XL9555 char dev registered at %s\n", devpath);
#endif

  syslog(LOG_INFO, "XL9555 initialized successfully\n");
  return OK;

errout_i2c:
  esp32s3_i2cbus_uninitialize(i2c);

errout_free:
  kmm_free(g_xl9555);
  g_xl9555 = NULL;
  return ret;
}

#endif /* CONFIG_BOSS1_ESP32S3_XL9555 */
