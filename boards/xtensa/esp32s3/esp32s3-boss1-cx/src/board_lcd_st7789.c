/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/board_lcd_st7789.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <debug.h>
#include <string.h>

#include <nuttx/arch.h>
#include <nuttx/sched.h>
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/lcd_dev.h>

#include <arch/board/board.h>

#include "esp32s3_gpio.h"
#include "esp32s3_lcd_i80.h"

#include "xtensa.h"
#include "hardware/esp32s3_gpio_sigmap.h"

#include "esp32s3-boss1-cx.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_ESP32S3_BOSS1_CX_LCD_I80_BL_XL9555_PIN
#  define CONFIG_ESP32S3_BOSS1_CX_LCD_I80_BL_XL9555_PIN 12
#endif

#ifndef CONFIG_ESP32S3_BOSS1_CX_LCD_I80_RST_XL9555_PIN
#  define CONFIG_ESP32S3_BOSS1_CX_LCD_I80_RST_XL9555_PIN 13
#endif

#define XL9555_IO_BL   CONFIG_ESP32S3_BOSS1_CX_LCD_I80_BL_XL9555_PIN
#define XL9555_IO_RST  CONFIG_ESP32S3_BOSS1_CX_LCD_I80_RST_XL9555_PIN

/* ST7789V Commands */

#define ST7789_SWRESET    0x01
#define ST7789_SLPOUT     0x11
#define ST7789_COLMOD     0x3a
#define ST7789_MADCTL     0x36
#define ST7789_DISPON     0x29
#define ST7789_CASET      0x2a
#define ST7789_RASET      0x2b
#define ST7789_RAMWR      0x2c
#define ST7789_RAMCTRL    0xb0
#define ST7789_PORCTRL    0xb2
#define ST7789_GCTRL      0xb7
#define ST7789_VCOMS      0xbb
#define ST7789_LCMCTRL    0xc0
#define ST7789_VDVVRHEN   0xc2
#define ST7789_VRHS       0xc3
#define ST7789_VDVS       0xc4
#define ST7789_PWCTR1     0xd0
#define ST7789_PWCTR2     0xd1
#define ST7789_PWCTR6     0xfc
#define ST7789_INVON      0x21
#define ST7789_NORON      0x13

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct st7789_lcd_s
{
  struct lcd_dev_s dev;             /* LCD device interface - must be first */
  FAR struct lcd_i80_s *lcd_i80;
  FAR struct ioexpander_dev_s *ioe;
  uint8_t pixel_size;
  uint8_t bpp;
  uint16_t width;
  uint16_t height;
  uint8_t refcount;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct st7789_lcd_s g_st7789_lcd;

/****************************************************************************
 * Name: st7789_send_init_sequence
 *
 * Description:
 *   Send ST7789V initialization sequence to configure the LCD controller.
 *   This includes power settings, display timing, and color format.
 *
 * Input Parameters:
 *   lcd - Pointer to lcd_i80_s handle
 *
 ****************************************************************************/

static void st7789_send_init_sequence(FAR struct lcd_i80_s *lcd)
{
  /* Software reset */
  lcd->send_cmd_with_param(ST7789_SWRESET, NULL, 0);

  /* Exit sleep mode */
  lcd->send_cmd_with_param(ST7789_SLPOUT, NULL, 0);

  /* Set color mode: 0x55 = 16-bit/pixel (RGB565) */
  uint8_t colmod_param[] = {0x55};
  lcd->send_cmd_with_param(ST7789_COLMOD, colmod_param, 1);

  /* Set memory access control: rotation 90° (MX=1, MV=1) */
  uint8_t madctl_param[] = {0x60};
  lcd->send_cmd_with_param(ST7789_MADCTL, madctl_param, 1);

  /* Set porch control: back/front porch = 0x0c, 0x0c; manual mode */
  uint8_t porctrl_param[] = {0x0c, 0x0c, 0x00, 0x33, 0x33};
  lcd->send_cmd_with_param(ST7789_PORCTRL, porctrl_param, 5);

  /* Set gate control: VGH=13.26V, VGL=-10.43V */
  uint8_t gctrl_param[] = {0x35};
  lcd->send_cmd_with_param(ST7789_GCTRL, gctrl_param, 1);

  /* Set VCOMS voltage: VCOM = 0.9V */
  uint8_t vcoms_param[] = {0x28};
  lcd->send_cmd_with_param(ST7789_VCOMS, vcoms_param, 1);

  /* Set LCM control: LCM = normal mode */
  uint8_t lcmctrl_param[] = {0x0c};
  lcd->send_cmd_with_param(ST7789_LCMCTRL, lcmctrl_param, 1);

  /* Enable VDV and VRH from VSYS: VDV=0x01, VRH=0xff */
  uint8_t vdvvrhen_param[] = {0x01, 0xff};
  lcd->send_cmd_with_param(ST7789_VDVVRHEN, vdvvrhen_param, 2);

  /* Set VRH voltage: VRH = 0x10 (4.3V) */
  uint8_t vrhs_param[] = {0x10};
  lcd->send_cmd_with_param(ST7789_VRHS, vrhs_param, 1);

  /* Set VDV: VDV = 0x20 */
  uint8_t vdv_param[] = {0x20};
  lcd->send_cmd_with_param(ST7789_VDVS, vdv_param, 1);

  /* Set power control 1: AVDD=6.8V, AVCL=-4.8V, VGH=13.5V */
  uint8_t pwctr1_param[] = {0xa4, 0xa1};
  lcd->send_cmd_with_param(ST7789_PWCTR1, pwctr1_param, 2);

  /* Set power control 2: VOP=0x64 */
  uint8_t pwctr2_param[] = {0x64};
  lcd->send_cmd_with_param(ST7789_PWCTR2, pwctr2_param, 1);

  /* Set power control 6: VDV=0x23 */
  uint8_t pwctr6_param[] = {0x23};
  lcd->send_cmd_with_param(ST7789_PWCTR6, pwctr6_param, 1);

  /* Enable display inversion */
  lcd->send_cmd_with_param(ST7789_INVON, NULL, 0);

  /* Set normal mode (not partial) */
  lcd->send_cmd_with_param(ST7789_NORON, NULL, 0);

  /* Enable display */
  lcd->send_cmd_with_param(ST7789_DISPON, NULL, 0);
}

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int st7789_getvideoinfo(FAR struct lcd_dev_s *dev,
                               FAR struct fb_videoinfo_s *vinfo);
static int st7789_getplaneinfo(FAR struct lcd_dev_s *dev, unsigned int planeno,
                               FAR struct lcd_planeinfo_s *pinfo);
static int st7789_getareaalign(FAR struct lcd_dev_s *dev,
                               FAR struct lcddev_area_align_s *align);
static int st7789_putrun(FAR struct lcd_dev_s *dev,
                         fb_coord_t row, fb_coord_t col,
                         FAR const uint8_t *buffer, size_t npixels);
static int st7789_putarea(FAR struct lcd_dev_s *dev,
                          fb_coord_t row_start, fb_coord_t row_end,
                          fb_coord_t col_start, fb_coord_t col_end,
                          FAR const uint8_t *buffer, fb_coord_t stride);

static int st7789_getvideoinfo(FAR struct lcd_dev_s *dev,
                               FAR struct fb_videoinfo_s *vinfo)
{
  lcdinfo("dev=%p vinfo=%p\n", dev, vinfo);
  if (dev && vinfo)
    {
      struct fb_videoinfo_s videoinfo =
      {
        .fmt     = FB_FMT_RGB16_565,
        .xres    = CONFIG_ESP32S3_BOSS1_CX_LCD_I80_WIDTH,
        .yres    = CONFIG_ESP32S3_BOSS1_CX_LCD_I80_HEIGHT,
        .nplanes = 1,
      };

      memcpy(vinfo, &videoinfo, sizeof(struct fb_videoinfo_s));
      return OK;
    }

  lcderr("ERROR: Returning EINVAL\n");
  return -EINVAL;
}

static int st7789_getplaneinfo(FAR struct lcd_dev_s *dev, unsigned int planeno,
                               FAR struct lcd_planeinfo_s *pinfo)
{
  lcdinfo("dev=%p planeno=%u pinfo=%p\n", dev, planeno, pinfo);
  if (dev && planeno == 0 && pinfo)
    {
      struct st7789_lcd_s *priv = (struct st7789_lcd_s *)dev;

      pinfo->putrun  = st7789_putrun;
      pinfo->putarea = st7789_putarea;
      pinfo->getrun  = NULL;
      pinfo->getarea = NULL;
      pinfo->buffer = NULL;
      pinfo->bpp    = priv->bpp;
      pinfo->dev    = dev;

      return OK;
    }

  lcderr("ERROR: Returning EINVAL\n");
  return -EINVAL;
}

static int st7789_getareaalign(FAR struct lcd_dev_s *dev,
                               FAR struct lcddev_area_align_s *align)
{
  if (dev && align)
    {
      align->row_start_align = 0;
      align->height_align   = 0;
      align->col_start_align = 0;
      align->width_align    = 0;
      align->buf_align      = 0;
      return OK;
    }

  return -EINVAL;
}

static int st7789_putrun(FAR struct lcd_dev_s *dev,
                         fb_coord_t row, fb_coord_t col,
                         FAR const uint8_t *buffer, size_t npixels)
{
  struct st7789_lcd_s *priv = (struct st7789_lcd_s *)dev;
  FAR struct lcd_i80_s *lcd = priv->lcd_i80;

  uint16_t col_end = col + npixels - 1;

  uint8_t col_start_msb = (col >> 8) & 0xff;
  uint8_t col_start_lsb = col & 0xff;
  uint8_t col_end_msb   = (col_end >> 8) & 0xff;
  uint8_t col_end_lsb   = col_end & 0xff;
  uint8_t caset_param[] =
    {
      col_start_msb, col_start_lsb,
      col_end_msb,   col_end_lsb
    };

  uint8_t row_msb = (row >> 8) & 0xff;
  uint8_t row_lsb = row & 0xff;
  uint8_t raset_param[] =
    {
      row_msb, row_lsb,
      row_msb, row_lsb
    };

  lcd->send_cmd_with_param(ST7789_CASET, caset_param, 4);
  lcd->send_cmd_with_param(ST7789_RASET, raset_param, 4);

  lcd->send_data(ST7789_RAMWR, buffer, npixels * priv->pixel_size);

  return OK;
}

static int st7789_putarea(FAR struct lcd_dev_s *dev,
                          fb_coord_t row_start, fb_coord_t row_end,
                          fb_coord_t col_start, fb_coord_t col_end,
                          FAR const uint8_t *buffer, fb_coord_t stride)
{
  struct st7789_lcd_s *priv = (struct st7789_lcd_s *)dev;
  FAR struct lcd_i80_s *lcd = priv->lcd_i80;

  uint16_t width  = col_end - col_start + 1;
  uint16_t height = row_end - row_start + 1;
  uint32_t expected_stride = width * priv->pixel_size;
  uint32_t size = width * height * priv->pixel_size;

  uint8_t col_start_msb = (col_start >> 8) & 0xff;
  uint8_t col_start_lsb = col_start & 0xff;
  uint8_t col_end_msb   = (col_end >> 8) & 0xff;
  uint8_t col_end_lsb   = col_end & 0xff;
  uint8_t caset_param[] =
    {
      col_start_msb, col_start_lsb,
      col_end_msb,   col_end_lsb
    };

  uint8_t row_start_msb = (row_start >> 8) & 0xff;
  uint8_t row_start_lsb = row_start & 0xff;
  uint8_t row_end_msb   = (row_end >> 8) & 0xff;
  uint8_t row_end_lsb   = row_end & 0xff;
  uint8_t raset_param[] =
    {
      row_start_msb, row_start_lsb,
      row_end_msb,   row_end_lsb
    };

  lcd->send_cmd_with_param(ST7789_CASET, caset_param, 4);
  lcd->send_cmd_with_param(ST7789_RASET, raset_param, 4);

  if (stride == expected_stride)
    {
      lcd->send_data(ST7789_RAMWR, buffer, size);
    }
  else
    {
      FAR uint8_t *fb = lcd->get_free_fb(LCD_I80_WAIT_FOREVER);

      for (uint16_t row = 0; row < height; row++)
        {
          memcpy(fb + row * expected_stride,
                 buffer + row * stride,
                 expected_stride);
        }

      lcd->send_data(ST7789_RAMWR, fb, size);
    }

  return OK;
}

static int st7789_getpower(FAR struct lcd_dev_s *dev)
{
  struct st7789_lcd_s *priv = (struct st7789_lcd_s *)dev;

  return (priv->refcount > 0) ? CONFIG_LCD_MAXPOWER : 0;
}

static int st7789_setpower(FAR struct lcd_dev_s *dev, int power)
{
  struct st7789_lcd_s *priv = (struct st7789_lcd_s *)dev;
  int ret = OK;

  if (power > 0)
    {
      ret = IOEXP_WRITEPIN(priv->ioe, XL9555_IO_BL, 1);
    }
  else
    {
      ret = IOEXP_WRITEPIN(priv->ioe, XL9555_IO_BL, 0);
    }

  return ret;
}

static int st7789_open(FAR struct lcd_dev_s *dev)
{
  struct st7789_lcd_s *priv = (struct st7789_lcd_s *)dev;

  if (priv->refcount == 0)
    {
      IOEXP_WRITEPIN(priv->ioe, XL9555_IO_BL, 1);
    }

  priv->refcount++;
  return OK;
}

static int st7789_close(FAR struct lcd_dev_s *dev)
{
  struct st7789_lcd_s *priv = (struct st7789_lcd_s *)dev;

  if (priv->refcount > 0)
    {
      priv->refcount--;
    }

  if (priv->refcount == 0)
    {
      IOEXP_WRITEPIN(priv->ioe, XL9555_IO_BL, 0);
    }

  return OK;
}



/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_lcd_initialize
 *
 * Description:
 *   Initialize LCD I80 interface and ST7789V controller.
 *   This function is called by board bringup code.
 *
 ****************************************************************************/

int board_lcd_initialize(void)
{
  struct st7789_lcd_s *priv = &g_st7789_lcd;
  int ret;

  lcdinfo("Initializing LCD ST7789 (8-bit I80)\n");

  priv->dev.getvideoinfo  = st7789_getvideoinfo;
  priv->dev.getplaneinfo  = st7789_getplaneinfo;
  priv->dev.getareaalign  = st7789_getareaalign;
  priv->dev.getpower      = st7789_getpower;
  priv->dev.setpower      = st7789_setpower;
  priv->dev.open          = st7789_open;
  priv->dev.close         = st7789_close;

  priv->ioe = esp32s3_gpioexp_getioe();
  if (priv->ioe == NULL)
    {
      lcderr("Failed to get GPIO expander handle\n");
      return -ENODEV;
    }

  ret = IOEXP_SETDIRECTION(priv->ioe, XL9555_IO_BL, IOEXPANDER_DIRECTION_OUT);
  if (ret < 0)
    {
      lcderr("Failed to set BL pin direction: %d\n", ret);
      return ret;
    }

  ret = IOEXP_WRITEPIN(priv->ioe, XL9555_IO_BL, 0);
  if (ret < 0)
    {
      lcderr("Failed to set BL pin value: %d\n", ret);
      return ret;
    }

  ret = IOEXP_SETDIRECTION(priv->ioe, XL9555_IO_RST, IOEXPANDER_DIRECTION_OUT);
  if (ret < 0)
    {
      lcderr("Failed to set RST pin direction: %d\n", ret);
      return ret;
    }

  ret = IOEXP_WRITEPIN(priv->ioe, XL9555_IO_RST, 1);
  if (ret < 0)
    {
      lcderr("Failed to set RST pin value: %d\n", ret);
      return ret;
    }

#define LCD_GPIO_CONFIG(pin, sig) \
  do \
    { \
      esp32s3_configgpio(pin, OUTPUT); \
      esp32s3_gpio_matrix_out(pin, sig, 0, 0); \
    } \
  while (0)

  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D0_PIN, LCD_DATA_OUT0_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D1_PIN, LCD_DATA_OUT1_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D2_PIN, LCD_DATA_OUT2_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D3_PIN, LCD_DATA_OUT3_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D4_PIN, LCD_DATA_OUT4_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D5_PIN, LCD_DATA_OUT5_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D6_PIN, LCD_DATA_OUT6_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_D7_PIN, LCD_DATA_OUT7_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_DC_PIN, LCD_DC_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_CS_PIN, LCD_CS_IDX);
  LCD_GPIO_CONFIG(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_WR_PIN, LCD_PCLK_IDX);

  ret = IOEXP_WRITEPIN(priv->ioe, XL9555_IO_RST, 0);
  if (ret < 0)
    {
      lcderr("Failed to reset LCD: %d\n", ret);
      return ret;
    }

  nxsched_usleep(10 * 1000);

  ret = IOEXP_WRITEPIN(priv->ioe, XL9555_IO_RST, 1);
  if (ret < 0)
    {
      lcderr("Failed to release LCD reset: %d\n", ret);
      return ret;
    }

  nxsched_usleep(120 * 1000);

  priv->lcd_i80 = lcd_i80_init(CONFIG_ESP32S3_BOSS1_CX_LCD_I80_WIDTH,
                                CONFIG_ESP32S3_BOSS1_CX_LCD_I80_HEIGHT,
                                CONFIG_ESP32S3_BOSS1_CX_LCD_I80_BPP / 8);
  if (priv->lcd_i80 == NULL)
    {
      lcderr("Failed to initialize LCD controller\n");
      return -ENODEV;
    }

  priv->pixel_size = CONFIG_ESP32S3_BOSS1_CX_LCD_I80_BPP / 8;
  priv->bpp        = CONFIG_ESP32S3_BOSS1_CX_LCD_I80_BPP;
  priv->width      = CONFIG_ESP32S3_BOSS1_CX_LCD_I80_WIDTH;
  priv->height     = CONFIG_ESP32S3_BOSS1_CX_LCD_I80_HEIGHT;
  priv->refcount   = 0;

  st7789_send_init_sequence(priv->lcd_i80);

  ret = lcddev_register(0);
  if (ret < 0)
    {
      lcderr("Failed to register LCD device: %d\n", ret);
      return ret;
    }

  lcdinfo("LCD ST7789 initialized successfully\n");

  return OK;
}

/****************************************************************************
 * Name: board_lcd_getdev
 *
 * Description:
 *   Return a reference to the LCD device object for the specified LCD.
 *   This allows support for multiple LCD devices.
 *
 * Input Parameters:
 *   lcddev - LCD device number (0 = first LCD)
 *
 * Returned Value:
 *   Pointer to LCD device struct on success, NULL on failure
 *
 ****************************************************************************/

FAR struct lcd_dev_s *board_lcd_getdev(int lcddev)
{
  if (lcddev == 0)
    {
      return &g_st7789_lcd.dev;
    }

  return NULL;
}

/****************************************************************************
 * Name: board_lcd_uninitialize
 *
 * Description:
 *   Uninitialize the LCD support.
 *
 ****************************************************************************/

void board_lcd_uninitialize(void)
{
  struct st7789_lcd_s *priv = &g_st7789_lcd;

  IOEXP_WRITEPIN(priv->ioe, XL9555_IO_BL, 0);
}
