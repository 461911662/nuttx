/****************************************************************************
 * arch/xtensa/src/esp32s3/esp32s3_lcd_i80.c
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
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/spinlock.h>
#include <nuttx/kmalloc.h>
#include <nuttx/semaphore.h>

#include "esp32s3_dma.h"
#include "esp32s3_irq.h"

#include "xtensa.h"
#include "hardware/esp32s3_gpio.h"
#include "periph_ctrl.h"

#include "esp32s3_lcd_i80.h"
#include "hal/lcd_hal.h"
#include "hal/lcd_ll.h"
#include "soc/clk_tree_defs.h"
#include "soc/lcd_periph.h"
#include "soc/gpio_sig_map.h"
#include "esp_private/periph_ctrl.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct esp32s3_lcd_i80_priv_s
{
  int dma_channel;
  struct esp32s3_dmadesc_s *dmadesc;
  int dmadesc_num;
  uint8_t *fb;
  sem_t tx_sem;
  spinlock_t lock;
  int cpu;
  int cpuint;
  lcd_hal_context_t hal;
};

typedef struct esp32s3_lcd_i80_priv_s esp32s3_lcd_i80_priv_t;

/****************************************************************************
 * External Functions
 ****************************************************************************/

extern void esp_rom_delay_us(uint32_t us);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static esp32s3_lcd_i80_priv_t g_lcd_priv;

static struct lcd_i80_s g_lcd_i80_dev;

static bool g_initialized = false;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: lcd_clock_config
 *
 * Description:
 *   Configure LCD_CAM clock (following ESP-IDF initialization).
 *
 ****************************************************************************/

static void lcd_clock_config(lcd_hal_context_t *hal)
{
  PERIPH_RCC_ACQUIRE_ATOMIC(lcd_periph_i80_signals.buses[0].module, rc_ref_count) {
    if (rc_ref_count == 0) {
      lcd_ll_enable_bus_clock(0, true);
      lcd_ll_reset_register(0);
    }
  }

  lcd_ll_enable_clock(hal->dev, true);

  /* set lcd_clk */
  lcd_ll_select_clk_src(hal->dev, LCD_CLK_SRC_PLL160M);
  lcd_ll_set_group_clock_coeff(hal->dev, 2, 0, 0);

  /* set pixel clk */
  lcd_ll_set_pixel_clock_prescale(hal->dev, 8);
  lcd_ll_set_clock_idle_level(hal->dev, true);
  lcd_ll_set_pixel_clock_edge(hal->dev, false);

  /* set dc level:
   * dc_idle_level = 0
   * dc_cmd_level = 0
   * dc_dummy_level = 0
   * dc_data_level = 1
  */
  lcd_ll_set_dc_level(hal->dev, 0, 0, 0, 1);
}

/****************************************************************************
 * Name: lcd_cam_config
 *
 * Description:
 *   Configure LCD_CAM controller mode (following ESP-IDF initialization).
 *
 ****************************************************************************/

static void lcd_cam_config(lcd_hal_context_t *hal)
{
  /* reset peripheral and FIFO after we select a correct clock source */
  lcd_ll_reset(hal->dev);
  lcd_ll_fifo_reset(hal->dev);

  lcd_ll_clear_interrupt_status(hal->dev, UINT32_MAX);

  /* select i80 mode */
  lcd_ll_enable_rgb_mode(hal->dev, false);

  /* set data format */
  lcd_ll_enable_rgb_yuv_convert(hal->dev, false); // disable YUV-RGB converter
  lcd_ll_set_dma_read_stride(hal->dev, 8);  // set how much data to read from DMA each time
  lcd_ll_set_swizzle_mode(hal->dev, LCD_LL_SWIZZLE_AB2BA); // sometime, we need to change the output data order: ABAB->BABA
  lcd_ll_enable_swizzle(hal->dev, true);
  lcd_ll_enable_output_always_on(hal->dev, true); // number of data cycles is controlled by DMA buffer size
}

/****************************************************************************
 * Name: lcd_dma_init
 *
 * Description:
 *   Initialize GDMA for LCD data transfer.
 *
 ****************************************************************************/

static int lcd_dma_init(uint32_t fb_size)
{
  esp32s3_lcd_i80_priv_t *priv = &g_lcd_priv;
  int ret = OK;

  priv->dma_channel = esp32s3_dma_request(ESP32S3_DMA_PERIPH_LCDCAM,
                                          10, 1, true);
  if (priv->dma_channel < 0)
    {
      lcderr("Failed to request DMA channel\n");
      return -ENOMEM;
    }

  esp32s3_dma_set_ext_memblk(priv->dma_channel, true,
                              ESP32S3_DMA_EXT_MEMBLK_64B);

  priv->dmadesc_num = (fb_size + ESP32S3_DMA_BUFFER_MAX_SIZE - 1) / ESP32S3_DMA_BUFFER_MAX_SIZE;
  if (priv->dmadesc_num < 1)
    {
      priv->dmadesc_num = 1;
    }

  priv->dmadesc = (struct esp32s3_dmadesc_s *)kmm_malloc(
      sizeof(struct esp32s3_dmadesc_s) * priv->dmadesc_num);
  if (!priv->dmadesc)
    {
      lcderr("Failed to allocate DMA descriptors\n");
      return -ENOMEM;
    }

  size_t cache_line_size = up_get_dcache_linesize();
  priv->fb = (uint8_t *)memalign(cache_line_size, fb_size);
  if (!priv->fb)
    {
      lcderr("Failed to allocate aligned frame buffer\n");
      return -ENOMEM;
    }

  memset(priv->fb, 0, fb_size);

  return OK;
}

/****************************************************************************
 * Name: lcd_interrupt_handler
 *
 * Description:
 *   LCD_CAM interrupt handler.
 *
 ****************************************************************************/

static int IRAM_ATTR lcd_interrupt_handler(int irq, void *context, void *arg)
{
  esp32s3_lcd_i80_priv_t *priv = &g_lcd_priv;
  uint32_t status;

  status = getreg32(LCD_CAM_LC_DMA_INT_ST_REG);

  if (status & LCD_CAM_LCD_TRANS_DONE_INT_ST_M)
    {
      nxsem_post(&priv->tx_sem);
    }

  lcd_ll_clear_interrupt_status(priv->hal.dev, status);

  return OK;
}

/****************************************************************************
 * Name: lcd_i80_get_free_fb
 *
 * Description:
 *   Get pointer to frame buffer for filling.
 *   This function waits for previous DMA transfer to complete,
 *   then returns the frame buffer pointer.
 *
 * Input:
 *   timeout - Wait timeout in milliseconds
 *             0: return immediately (non-blocking)
 *             LCD_I80_WAIT_FOREVER: wait indefinitely
 *
 * Returned Value:
 *   Pointer to frame buffer on success, NULL on timeout
 *
 ****************************************************************************/

static FAR uint8_t *lcd_i80_get_free_fb(uint32_t timeout)
{
  esp32s3_lcd_i80_priv_t *priv = &g_lcd_priv;
  int ret;

  if (timeout == LCD_I80_WAIT_FOREVER)
    {
      ret = nxsem_wait(&priv->tx_sem);
    }
  else
    {
      ret = nxsem_tickwait(&priv->tx_sem, timeout);
    }

  if (ret < 0)
    {
      return NULL;
    }

  return priv->fb;
}

/****************************************************************************
 * Name: lcd_i80_send_cmd_with_param
 *
 * Description:
 *   Send command with parameters to LCD via LCD_CAM I80 interface.
 *   This function automatically gets fb, copies parameters to fb, and triggers DMA.
 *   Completion is signaled via semaphore.
 *
 * Input:
 *   cmd       - Command value
 *   param     - Pointer to parameter data
 *   param_len - Number of parameter bytes
 *
 ****************************************************************************/

static void lcd_i80_send_cmd_with_param(uint8_t cmd, const uint8_t *param, size_t param_len)
{
  esp32s3_lcd_i80_priv_t *priv = &g_lcd_priv;

  uint32_t dummy_cycles = 0;
  uint32_t cmd_cycles = 1;
  uint32_t data_cycles = param ? 1 : 0;

  /* Wait for previous transfer to complete */
  lcd_i80_get_free_fb(LCD_I80_WAIT_FOREVER);

  /* reconfig lcd_cam */
  lcd_ll_reverse_dma_data_bit_order(priv->hal.dev, false);
  lcd_ll_enable_swizzle(priv->hal.dev, false);

  lcd_ll_set_command(priv->hal.dev, 8, cmd);
  lcd_ll_set_phase_cycles(priv->hal.dev, cmd_cycles, dummy_cycles, data_cycles);
  lcd_ll_set_blank_cycles(priv->hal.dev, 1, 1);

  /* clear fifo data */
  lcd_ll_fifo_reset(priv->hal.dev);

  if (param != NULL && param_len > 0)
    {
      memcpy(priv->fb, param, param_len);

      up_flush_dcache((uintptr_t)priv->fb, (uintptr_t)(priv->fb + param_len));

      esp32s3_dma_setup(priv->dmadesc, 1,
                        priv->fb, param_len,
                        true, priv->dma_channel);
      esp32s3_dma_load(priv->dmadesc, priv->dma_channel, true);

      esp32s3_dma_enable(priv->dma_channel, true);

      // delay 1us is sufficient for DMA to pass data to LCD FIFO
      // in fact, this is only needed when LCD pixel clock is set too high
      esp_rom_delay_us(1);
    }

  lcd_ll_start(priv->hal.dev);
}

/****************************************************************************
 * Name: lcd_i80_send_data
 *
 * Description:
 *   Send command and pixel data to LCD via LCD_CAM I80 interface.
 *   This function sends a command followed by pixel data via DMA.
 *
 * Input:
 *   cmd  - LCD command (e.g., ST7789_RAMWR)
 *   data - Pointer to pixel data buffer
 *   size - Number of bytes to transfer
 *
 ****************************************************************************/

static void lcd_i80_send_data(uint8_t cmd, FAR const uint8_t *data, size_t size)
{
  esp32s3_lcd_i80_priv_t *priv = &g_lcd_priv;
  uint32_t ret;

  if (size == 0)
    {
      return;
    }

  /* Wait for previous transfer to complete, then copy data */

  if (priv->fb != data)
    {
      lcd_i80_get_free_fb(LCD_I80_WAIT_FOREVER);
      memcpy(priv->fb, data, size);
    }

  up_flush_dcache((uintptr_t)priv->fb, (uintptr_t)(priv->fb + size));

  /* Configure DMA - single transfer for entire buffer */

  ret = esp32s3_dma_setup(priv->dmadesc, priv->dmadesc_num, priv->fb, size,
                          true, priv->dma_channel);
  if (ret != size)
    {
      return;
    }

  esp32s3_dma_load(priv->dmadesc, priv->dma_channel, true);

  /* reconfig lcd_cam */

  lcd_ll_reverse_dma_data_bit_order(priv->hal.dev, false);
  lcd_ll_enable_swizzle(priv->hal.dev, true);

  /* Configure command and data phases */

  lcd_ll_set_command(priv->hal.dev, 8, cmd);
  lcd_ll_set_phase_cycles(priv->hal.dev, 1, 0, 1);
  lcd_ll_set_blank_cycles(priv->hal.dev, 1, 1);

  lcd_ll_fifo_reset(priv->hal.dev);
  esp32s3_dma_enable(priv->dma_channel, true);
  esp_rom_delay_us(1);

  lcd_ll_start(priv->hal.dev);
}

/****************************************************************************
 * Public Functions
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
                                   uint8_t pixel_size)
{
  esp32s3_lcd_i80_priv_t *priv = &g_lcd_priv;
  uint32_t fb_size;
  int ret;
  irqstate_t flags;

  if (g_initialized)
    {
      return &g_lcd_i80_dev;
    }

  fb_size = (uint32_t)width * height * pixel_size;

  flags = spin_lock_irqsave(&priv->lock);

  lcd_hal_init(&priv->hal, 0);
  lcd_clock_config(&priv->hal);
  lcd_cam_config(&priv->hal);

  ret = nxsem_init(&priv->tx_sem, 0, 1);
  if (ret < 0)
    {
      lcderr("Failed to initialize semaphore\n");
      spin_unlock_irqrestore(&priv->lock, flags);
      return NULL;
    }

  ret = lcd_dma_init(fb_size);
  if (ret < 0)
    {
      lcderr("Failed to initialize DMA\n");
      goto err_with_lock;
    }

  lcd_ll_enable_interrupt(priv->hal.dev, LCD_LL_EVENT_TRANS_DONE, true);

  priv->cpu = this_cpu();
  priv->cpuint = esp32s3_setup_irq(priv->cpu,
                                    ESP32S3_PERIPH_LCD_CAM,
                                    ESP32S3_INT_PRIO_DEF,
                                    ESP32S3_CPUINT_LEVEL);
  if (priv->cpuint < 0)
    {
      lcderr("Failed to setup IRQ\n");
      ret = priv->cpuint;
      goto err_with_dma;
    }

  ret = irq_attach(ESP32S3_IRQ_LCD_CAM, lcd_interrupt_handler, NULL);
  if (ret < 0)
    {
      lcderr("Failed to attach IRQ\n");
      goto err_with_irq;
    }

  up_enable_irq(ESP32S3_IRQ_LCD_CAM);

  g_lcd_i80_dev.width      = width;
  g_lcd_i80_dev.height     = height;
  g_lcd_i80_dev.pixel_size = pixel_size;
  g_lcd_i80_dev.fb_size    = fb_size;

  g_lcd_i80_dev.send_cmd_with_param = lcd_i80_send_cmd_with_param;
  g_lcd_i80_dev.send_data    = lcd_i80_send_data;
  g_lcd_i80_dev.get_free_fb = lcd_i80_get_free_fb;

  g_initialized = true;

  spin_unlock_irqrestore(&priv->lock, flags);

  lcdinfo("LCD I80 controller initialized successfully\n");

  return &g_lcd_i80_dev;

err_with_irq:
  esp32s3_teardown_irq(priv->cpu, ESP32S3_PERIPH_LCD_CAM, priv->cpuint);

err_with_dma:
  if (priv->dmadesc)
    {
      kmm_free(priv->dmadesc);
    }
  kmm_free(priv->fb);
  esp32s3_dma_release(priv->dma_channel);

err_with_lock:
  nxsem_destroy(&priv->tx_sem);
  spin_unlock_irqrestore(&priv->lock, flags);
  return NULL;
}