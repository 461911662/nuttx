/****************************************************************************
 * drivers/ioexpander/xl9555.h
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

#ifndef __DRIVERS_IOEXPANDER_XL9555_H
#define __DRIVERS_IOEXPANDER_XL9555_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#if defined(CONFIG_IOEXPANDER) && defined(CONFIG_IOEXPANDER_XL9555)

#include <nuttx/irq.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/kmalloc.h>
#include <nuttx/mutex.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct xl9555_config_s
{
  uint8_t address;
  uint32_t frequency;
};

struct xl9555_dev_s
{
  struct ioexpander_dev_s      dev;
#ifdef CONFIG_XL9555_SHADOW_MODE
  uint8_t                      sreg[8];
#endif
#ifdef CONFIG_XL9555_MULTIPLE
  FAR struct xl9555_dev_s     *flink;
#endif
  FAR struct xl9555_config_s  *config;
  FAR struct i2c_master_s     *i2c;
  mutex_t                      lock;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR struct xl9555_dev_s *xl9555_initialize(FAR struct i2c_master_s *dev,
                                           FAR struct xl9555_config_s *config);

#endif /* CONFIG_IOEXPANDER && CONFIG_IOEXPANDER_XL9555 */
#endif /* __DRIVERS_IOEXPANDER_XL9555_H */
