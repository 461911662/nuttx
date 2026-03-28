/****************************************************************************
 * drivers/ioexpander/xl9555_int.h
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

#ifndef __DRIVERS_IOEXPANDER_XL9555_INT_H
#define __DRIVERS_IOEXPANDER_XL9555_INT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#if defined(CONFIG_IOEXPANDER) && defined(CONFIG_IOEXPANDER_XL9555_INT)

#include <nuttx/irq.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/kmalloc.h>
#include <nuttx/mutex.h>
#include <nuttx/wdog.h>
#include <nuttx/clock.h>
#include <nuttx/wqueue.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Prerequisites:
 *   CONFIG_I2C
 *     I2C support is required
 *   CONFIG_IOEXPANDER
 *     Enables support for the XL9555 I/O expander
 *
 * CONFIG_IOEXPANDER_XL9555_INT
 *   Enables support for the XL9555 driver with interrupt support
 * CONFIG_XL9555_INT_MULTIPLE
 *   Can be defined to support multiple XL9555 devices on board.
 * CONFIG_XL9555_INT_NCALLBACKS
 *   Maximum number of supported pin interrupt callbacks.
 */

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
#  ifndef CONFIG_XL9555_INT_NCALLBACKS
#    define CONFIG_XL9555_INT_NCALLBACKS 4
#  endif
#endif

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
#  ifndef CONFIG_SCHED_WORKQUEUE
#    error Work queue support required.  CONFIG_SCHED_WORKQUEUE must be selected.
#  endif
#endif

/* XL9555 Resources ********************************************************/

#define XL9555_INT_GPIO_NPINS 16 /* All pins can be used as GPIOs */

#ifndef CONFIG_I2C
#error "CONFIG_I2C is required by XL9555_INT"
#endif

#define XL9555_INT_MAXDEVS             8

/* I2C frequency */

#define XL9555_INT_I2C_MAXFREQUENCY   400000       /* 400KHz */

/* XL9555 Registers ********************************************************/

/* Register Addresses */

#define XL9555_INT_REG_INPUT0     0x00
#define XL9555_INT_REG_INPUT1     0x01
#define XL9555_INT_REG_OUTPUT0    0x02
#define XL9555_INT_REG_OUTPUT1    0x03
#define XL9555_INT_REG_POLINV0    0x04
#define XL9555_INT_REG_POLINV1    0x05
#define XL9555_INT_REG_CONFIG0    0x06
#define XL9555_INT_REG_CONFIG1    0x07

#define XL9555_INT_REG_INPUT      XL9555_INT_REG_INPUT0
#define XL9555_INT_REG_OUTPUT     XL9555_INT_REG_OUTPUT0
#define XL9555_INT_REG_POLINV     XL9555_INT_REG_POLINV0
#define XL9555_INT_REG_CONFIG     XL9555_INT_REG_CONFIG0

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
/* This type represents one registered pin interrupt callback */

struct xl9555_int_callback_s
{
  ioe_pinset_t pinset;                 /* Set of pin interrupts that will generate
                                         * the callback. */
  ioe_callback_t cbfunc;               /* The saved callback function pointer */
  FAR void *cbarg;                     /* Callback argument */
};
#endif

/* This structure represents the state of the XL9555 driver */

struct xl9555_int_dev_s
{
  struct ioexpander_dev_s         dev;      /* Nested structure to allow casting as public gpio
                                              * expander. */
#ifdef CONFIG_XL9555_INT_SHADOW_MODE
  uint8_t                        sreg[8];  /* Shadowed registers of the XL9555 */
#endif
#ifdef CONFIG_XL9555_INT_MULTIPLE
  FAR struct xl9555_int_dev_s   *flink;    /* Supports a singly linked list of drivers */
#endif
  FAR struct xl9555_int_config_s *config;   /* Board configuration data */
  FAR struct i2c_master_s        *i2c;      /* Saved I2C driver instance */
  mutex_t                         lock;     /* Mutual exclusion */

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  struct work_s                   work;     /* Supports the interrupt handling "bottom half" */

  /* Saved callback information for each I/O expander client */

  struct xl9555_int_callback_s cb[CONFIG_XL9555_INT_NCALLBACKS];
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR struct xl9555_int_dev_s *xl9555_int_initialize(FAR struct i2c_master_s *dev,
                                           FAR struct xl9555_int_config_s *config);

#endif /* CONFIG_IOEXPANDER && CONFIG_IOEXPANDER_XL9555_INT */
#endif /* __DRIVERS_IOEXPANDER_XL9555_INT_H */
