/****************************************************************************
 * arch/xtensa/src/esp32s3/esp32s3_lcd_i80.h
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

#ifndef __ARCH_XTENSA_SRC_ESP32S3_ESP32S3_LCD_I80_H
#define __ARCH_XTENSA_SRC_ESP32S3_ESP32S3_LCD_I80_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LCD_I80_WAIT_FOREVER UINT32_MAX

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct lcd_i80_s
{
  uint16_t width;
  uint16_t height;
  uint8_t  pixel_size;
  uint32_t fb_size;

  void (*send_cmd_with_param)(uint8_t cmd, const uint8_t *param, size_t param_len);
  void (*send_data)(uint8_t cmd, FAR const uint8_t *data, size_t size);
  FAR uint8_t *(*get_free_fb)(uint32_t timeout);
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: lcd_i80_init
 *
 * Description:
 *   Initialize LCD_CAM controller (clock, DMA, interrupt).
 *   GPIO configuration must be done by board layer before calling
 *   this function.
 *
 * Returned Value:
 *   Pointer to lcd_i80_s handle on success, NULL on failure
 *
 ****************************************************************************/

FAR struct lcd_i80_s *lcd_i80_init(uint16_t width, uint16_t height,
                                    uint8_t pixel_size);

#endif /* __ARCH_XTENSA_SRC_ESP32S3_ESP32S3_LCD_I80_H */