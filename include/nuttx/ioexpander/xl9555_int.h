/****************************************************************************
 * include/nuttx/ioexpander/xl9555_int.h
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

#ifndef __INCLUDE_NUTTX_IOEXPANDER_XL9555_INT_H
#define __INCLUDE_NUTTX_IOEXPANDER_XL9555_INT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define XL9555_IOC_SETPIN     1
#define XL9555_IOC_READPIN    2
#define XL9555_IOC_WRITEPIN   3

#define XL9555_DIRECTION_IN   0
#define XL9555_DIRECTION_OUT  1

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* A reference to a structure of this type must be passed to the XL9555
 * driver when the driver is instantiated. This structure provides
 * information about the configuration of the XL9555 and provides some
 * board-specific hooks.
 *
 * Memory for this structure is provided by the caller.  It is not copied by
 * the driver and is presumed to persist while the driver is active. The
 * memory must be writeable because, under certain circumstances, the driver
 * may modify the frequency.
 */

struct xl9555_int_config_s
{
  uint8_t address;     /* 7-bit I2C address (only bits 0-6 used) */
  uint32_t frequency;  /* I2C frequency */

#ifdef CONFIG_IOEXPANDER_INT_ENABLE
  /* IRQ/GPIO access callbacks.  These operations all hidden behind
   * callbacks to isolate the XL9555 driver from differences in GPIO
   * interrupt handling by varying boards and MCUs.
   *
   * attach  - Attach the XL9555 interrupt handler to the GPIO interrupt
   * enable  - Enable or disable the GPIO interrupt
   */

  CODE int  (*attach)(FAR struct xl9555_int_config_s *state, xcpt_t isr,
                      FAR void *arg);
  CODE void (*enable)(FAR struct xl9555_int_config_s *state, bool enable);
#endif
};

struct xl9555_int_pin_s
{
  uint8_t pin;
  uint8_t direction;
  bool value;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: xl9555_int_initialize
 *
 * Description:
 *   Instantiate and configure the XL9555 device driver to use the provided
 *   I2C device instance.
 *
 * Input Parameters:
 *   dev     - An I2C driver instance
 *   config  - Persistent board configuration data
 *
 * Returned Value:
 *   an ioexpander_dev_s instance on success, NULL on failure.
 *
 ****************************************************************************/

FAR struct ioexpander_dev_s *xl9555_int_initialize(FAR struct i2c_master_s *dev,
                                        FAR struct xl9555_int_config_s *config);

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_IOEXPANDER_XL9555_INT_H */
