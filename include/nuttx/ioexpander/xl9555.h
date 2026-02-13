/****************************************************************************
 * include/nuttx/ioexpander/xl9555.h
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

#ifndef __INCLUDE_NUTTX_IOEXPANDER_XL9555_H
#define __INCLUDE_NUTTX_IOEXPANDER_XL9555_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/ioexpander/ioexpander.h>
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

struct xl9555_config_s
{
  uint8_t address;
  uint32_t frequency;
};

struct xl9555_pin_s
{
  uint8_t pin;
  uint8_t direction;
  bool value;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR struct ioexpander_dev_s *xl9555_initialize(FAR struct i2c_master_s *i2cdev,
                                       FAR struct xl9555_config_s *config);

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_IOEXPANDER_XL9555_H */
