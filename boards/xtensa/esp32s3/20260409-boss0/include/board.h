/****************************************************************************
 * boards/xtensa/esp32s3/20260409-boss0/include/board.h
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

#ifndef __BOARDS_XTENSA_ESP32S3_20260409_BOSS0_INCLUDE_BOARD_H
#define __BOARDS_XTENSA_ESP32S3_20260409_BOSS0_INCLUDE_BOARD_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clocking *****************************************************************/

/* The ESP32-S3 20260409-boss0 board is fitted with a 40MHz crystal */

#define BOARD_XTAL_FREQUENCY    40000000

#ifdef CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ
#  define BOARD_CLOCK_FREQUENCY (CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ * 1000000)
#else
#  define BOARD_CLOCK_FREQUENCY 80000000
#endif

/* LED definitions **********************************************************/

#define BOARD_NLEDS         1

#define BOARD_LED1          0

#define BOARD_LED_1_BIT     (1 << BOARD_LED1)

#define LED_STARTED       0
#define LED_HEAPALLOCATE  0
#define LED_IRQSENABLED   0
#define LED_STACKCREATED  0
#define LED_INIRQ         2
#define LED_SIGNAL        2
#define LED_ASSERTION     2
#define LED_PANIC         3

/* GPIO pins used by the GPIO Subsystem */

#define BOARD_NGPIOOUT    0
#define BOARD_NGPIOIN     0
#define BOARD_NGPIOINT    0

/* I2C0 pins definitions (XL9555 IO Expander) */

#define BOARD_I2C0_SCL_GPIO     1
#define BOARD_I2C0_SDA_GPIO     2
#define BOARD_I2C0_INT_GPIO     16

/* XL9555 IO Expander */

#define BOARD_XL9555_I2C_ADDR    0x20

/* Button definitions - XL9555 P00 */

#define BOARD_BUTTON0       0
#define BOARD_BUTTON0_BIT   (1 << BOARD_BUTTON0)
#define BOARD_BUTTON_NUM    1

/* XL9555 IO Pin definitions */

#define BOARD_XL9555_IO_P00   0
#define BOARD_XL9555_IO_P01   1
#define BOARD_XL9555_IO_P02   2
#define BOARD_XL9555_IO_P03   3
#define BOARD_XL9555_IO_P04   4
#define BOARD_XL9555_IO_P05   5
#define BOARD_XL9555_IO_P06   6
#define BOARD_XL9555_IO_P07   7
#define BOARD_XL9555_IO_P10   8
#define BOARD_XL9555_IO_P11   9
#define BOARD_XL9555_IO_P12   10
#define BOARD_XL9555_IO_P13   11
#define BOARD_XL9555_IO_P14   12
#define BOARD_XL9555_IO_P15   13
#define BOARD_XL9555_IO_P16   14
#define BOARD_XL9555_IO_P17   15

#endif /* __BOARDS_XTENSA_ESP32S3_20260409_BOSS0_INCLUDE_BOARD_H */
