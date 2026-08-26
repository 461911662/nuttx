/****************************************************************************
 * boards/xtensa/esp32s3/20260409-boss0/src/esp32s3-20260409-boss0.h
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

#ifndef __BOARDS_XTENSA_ESP32S3_20260409_BOSS0_SRC_ESP32S3_20260409_BOSS0_H
#define __BOARDS_XTENSA_ESP32S3_20260409_BOSS0_SRC_ESP32S3_20260409_BOSS0_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdint.h>
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/input/buttons.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_bringup
 ****************************************************************************/

int esp32s3_bringup(void);

#ifdef CONFIG_ARCH_LEDS
void esp32s3_led_initialize(void);
#endif

#ifdef CONFIG_ESP32S3_SPIFLASH
int board_spiflash_init(void);
#endif

#if defined(CONFIG_ESPRESSIF_I2S0) || defined(CONFIG_ESPRESSIF_I2S1)
int board_i2sdev_initialize(int port, bool enable_tx, bool enable_rx);
#endif

#ifdef CONFIG_20260409_BOSS0_GPIO_EXP
int esp32s3_gpioexp_initialize(void);
FAR struct ioexpander_dev_s *esp32s3_gpioexp_getioe(void);
#endif

#ifdef CONFIG_DEV_GPIO
int esp32s3_gpio_initialize(void);
#endif

#endif /* __BOARDS_XTENSA_ESP32S3_20260409_BOSS0_SRC_ESP32S3_20260409_BOSS0_H */
