/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/include/board.h
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

#ifndef __BOARDS_XTENSA_ESP32S3_ESP32S3_BOSS1_CX_INCLUDE_BOARD_H
#define __BOARDS_XTENSA_ESP32S3_ESP32S3_BOSS1_CX_INCLUDE_BOARD_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clocking *****************************************************************/

/* The ESP32-S3 BOSS1-CX board is fitted with a 40MHz crystal */

#define BOARD_XTAL_FREQUENCY    40000000

#ifdef CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ
#  define BOARD_CLOCK_FREQUENCY (CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ * 1000000)
#else
#  define BOARD_CLOCK_FREQUENCY 80000000
#endif

/* LED definitions **********************************************************/

/* Define how many LEDs this board has (needed by userleds) */

#define BOARD_NLEDS         1

#define BOARD_LED1 /* LED1 - BOSS1-CX uses GPIO15 */

/* LED GPIO - BOSS1-CX uses GPIO15 */

#define GPIO_LED1  15

/* LED bits for use with board_userled_all() */

#define BOARD_LED_1_BIT   (1 << BOARD_LED1)


/* These LEDs are not used by the board port unless CONFIG_ARCH_LEDS is
 * defined.  In that case, the usage by the board port is defined in
 * include/board.h and src/esp32s3_autoleds.c. The LEDs are used to encode
 * OS-related events as follows:
 *
 *  SYMBOL                MEANING                         LED1 STATE
 *  -----------------------  --------------------------  ----
 *  LED_STARTED          NuttX has been started           OFF
 *  LED_HEAPALLOCATE     Heap has been allocated          OFF
 *  LED_IRQSENABLED     Interrupts enabled               OFF
 *  LED_STACKCREATED    Idle stack created               ON
 *  LED_INIRQ           In an interrupt                  N/C
 *  LED_SIGNAL          In a signal handler              N/C
 *  LED_ASSERTION       An assertion failed              N/C
 *  LED_PANIC           The system has crashed        Blinking
 */

#define LED_STARTED       0
#define LED_HEAPALLOCATE  0
#define LED_IRQSENABLED   0
#define LED_STACKCREATED  0
#define LED_INIRQ         2
#define LED_SIGNAL        2
#define LED_ASSERTION     2
#define LED_PANIC         3

/* IR Transmitter definitions *************************************************/

#define BOARD_IR_TX_GPIO       16
#define BOARD_IR_TX_CHANNEL    0

/* GPIO pins used by the GPIO Subsystem */

#define BOARD_NGPIOOUT    0 /* Amount of GPIO Output pins */
#define BOARD_NGPIOIN     0
#define BOARD_NGPIOINT    0

/* I2C0 pins definitions ********************************************/

#define BOARD_I2C0_SCL_GPIO     11
#define BOARD_I2C0_SDA_GPIO     10

/* I2S0 (LMD4030 Microphone) definitions ********************************/

#define BOARD_I2S0_BCLKPIN   3
#define BOARD_I2S0_DINPIN    42

/* I2S1 (NS4168 Audio) definitions ******************************************/

#define BOARD_I2S1_BCLKPIN   46
#define BOARD_I2S1_WSPIN     9
#define BOARD_I2S1_DOUTPIN    8

#endif /* __BOARDS_XTENSA_ESP32S3_ESP32S3_BOSS1_CX_INCLUDE_BOARD_H */
