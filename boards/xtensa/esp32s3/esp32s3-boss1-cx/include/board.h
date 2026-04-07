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

/* WS2812 LED Strip definitions ***********************************************/

#define BOARD_WS2812_GPIO          18
#define BOARD_WS2812_RMT_CHANNEL    1

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

/* Speaker (NS4168) definitions *******************************************/

#define BOARD_SPK_ENABLE_PIN   0  /* XL9555 IO0 for speaker enable */
#define BOARD_SPK_I2S_PORT    1  /* I2S1 for NS4168 */

/* SDMMC (SD Card) definitions *****************************************/

#define BOARD_SDMMC_CD_GPIO     4   /* Card Detect */
#define BOARD_SDMMC_CMD_GPIO    5   /* Command */
#define BOARD_SDMMC_CLK_GPIO    6   /* Clock */
#define BOARD_SDMMC_DATA0_GPIO  7   /* Data0 */

/* Button definitions ***********************************************/

/* Keys are on XL9555 IO1-IO4 */
#define BUTTON_KEY0        0
#define BUTTON_KEY1        1
#define BUTTON_KEY2        2
#define BUTTON_KEY3        3

#define BUTTON_KEY0_BIT    (1 << BUTTON_KEY0)
#define BUTTON_KEY1_BIT    (1 << BUTTON_KEY1)
#define BUTTON_KEY2_BIT    (1 << BUTTON_KEY2)
#define BUTTON_KEY3_BIT    (1 << BUTTON_KEY3)

#define BOARD_BUTTON_NUM   4

/* LCD (ST7789 I80) definitions ***********************************************/

#ifdef CONFIG_ESP32S3_BOSS1_CX_LCD_I80
#define BOARD_LCD_I80_D0_GPIO     40
#define BOARD_LCD_I80_D1_GPIO     38
#define BOARD_LCD_I80_D2_GPIO     39
#define BOARD_LCD_I80_D3_GPIO     48
#define BOARD_LCD_I80_D4_GPIO     45
#define BOARD_LCD_I80_D5_GPIO     21
#define BOARD_LCD_I80_D6_GPIO     47
#define BOARD_LCD_I80_D7_GPIO     14
#define BOARD_LCD_I80_DC_GPIO      1
#define BOARD_LCD_I80_CS_GPIO      2
#define BOARD_LCD_I80_WR_GPIO     41
#define BOARD_LCD_I80_RST_XL9555_PIN  11
#define BOARD_LCD_I80_BL_XL9555_PIN  10
#endif

/* Touchscreen (FT6336U) definitions *****************************************/

#ifdef CONFIG_ESP32S3_BOSS1_CX_TOUCHSCREEN_FT6336U
#define BOARD_TOUCH_RST_XL9555_PIN   8
#define BOARD_TOUCH_INT_XL9555_PIN   9
#endif

#endif /* __BOARDS_XTENSA_ESP32S3_ESP32S3_BOSS1_CX_INCLUDE_BOARD_H */
