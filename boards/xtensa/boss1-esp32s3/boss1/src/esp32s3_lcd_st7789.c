/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-box/src/esp32s3_board_lcd_st7789.c
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

#include <stdio.h>
#include <stdbool.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/signal.h>
#include <nuttx/spi/spi.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/st7789.h>

#include <arch/board/board.h>

#include "esp32s3_gpio.h"
#include "esp32s3_spi.h"
#include "hardware/esp32s3_gpio_sigmap.h"

#include "esp32s3-boss1.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define putreg32(v,a)     (*(volatile uint32_t *)(a) = (v))
#define DR_REG_SPI2_BASE                        0x60024000
#define REG_SPI_BASE(i)         (((i)>=2) ? (DR_REG_SPI2_BASE + (i-2) * 0x1000) : (0))    // GPSPI2 and GPSPI3
/* SPI_USER_REG register
 * SPI USER control register
 */
#define SPI_USER_REG(i) (REG_SPI_BASE(i) + 0x10)

/* SPI_SIO : R/W; bitpos: [17]; default: 0;
 * Set the bit to enable 3-line half duplex communication mosi and miso
 * signals share the same pin. 1: enable 0: disable. Can be configured in
 * CONF state.
 */
#define SPI_SIO_M  (SPI_SIO_V << SPI_SIO_S)
#define SPI_SIO_V  0x00000001
#define SPI_SIO_S  17

/* SPI_USR_MOSI : R/W; bitpos: [27]; default: 0;
 * This bit enable the write-data phase of an operation. Can be configured
 * in CONF state.
 */
#define SPI_USR_MOSI_M  (SPI_USR_MOSI_V << SPI_USR_MOSI_S)
#define SPI_USR_MOSI_V  0x00000001
#define SPI_USR_MOSI_S  27

/* SPI_USR_MISO : R/W; bitpos: [28]; default: 0;
 * This bit enable the read-data phase of an operation. Can be configured in
 * CONF state.
 */
#define SPI_USR_MISO_M  (SPI_USR_MISO_V << SPI_USR_MISO_S)
#define SPI_USR_MISO_V  0x00000001
#define SPI_USR_MISO_S  28

/* SPI_CS_HOLD : R/W; bitpos: [6]; default: 1;
 * spi cs keep low when spi is in  done  phase. 1: enable 0: disable. Can be
 * configured in CONF state.
 */
#define SPI_CS_HOLD_M  (SPI_CS_HOLD_V << SPI_CS_HOLD_S)
#define SPI_CS_HOLD_V  0x00000001
#define SPI_CS_HOLD_S  6

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* sync arch/xtensa/src/esp32s3/esp32s3_spi.c */
struct esp32s3_spi_config_s
{
  uint32_t clk_freq;          /* SPI default clock frequency */
  uint32_t width;             /* SPI default width */
  enum spi_mode_e mode;       /* SPI default mode */

  uint8_t id;                 /* ESP32-S3 SPI device ID: SPIx {2,3} */
  uint8_t cs_pin;             /* GPIO configuration for CS */
  uint8_t mosi_pin;           /* GPIO configuration for MOSI */
  uint8_t miso_pin;           /* GPIO configuration for MISO */
  uint8_t clk_pin;            /* GPIO configuration for CLK */
#ifdef CONFIG_ESP32S3_SPI_DMA
  uint8_t periph;             /* Peripheral ID */
  uint8_t irq;                /* Interrupt ID */
#endif
  uint32_t clk_bit;           /* Clock enable bit */
  uint32_t rst_bit;           /* SPI reset bit */
#ifdef CONFIG_ESP32S3_SPI_DMA
  uint32_t dma_clk_bit;       /* DMA clock enable bit */
  uint32_t dma_rst_bit;       /* DMA reset bit */
  uint8_t dma_periph;         /* DMA peripheral */
#endif
  uint32_t cs_insig;          /* SPI CS input signal index */
  uint32_t cs_outsig;         /* SPI CS output signal index */
  uint32_t mosi_insig;        /* SPI MOSI input signal index */
  uint32_t mosi_outsig;       /* SPI MOSI output signal index */
  uint32_t miso_insig;        /* SPI MISO input signal index */
  uint32_t miso_outsig;       /* SPI MISO output signal index */
  uint32_t clk_insig;         /* SPI CLK input signal index */
  uint32_t clk_outsig;        /* SPI CLK output signal index */
};

struct esp32s3_spi_priv_s
{
  /* Externally visible part of the SPI interface */

  struct spi_dev_s spi_dev;

  /* Port configuration */

  const struct esp32s3_spi_config_s *config;
  int refs;             /* Reference count */
  mutex_t lock;         /* Held while chip is selected for mutual exclusion */
#ifdef CONFIG_ESP32S3_SPI_DMA
  sem_t sem_isr;        /* Interrupt wait semaphore */
  int cpu;              /* CPU ID */
  int cpuint;           /* SPI interrupt ID */
  int32_t dma_channel;  /* Channel assigned by the GDMA driver */

  /* DMA RX/TX description */

  struct esp32s3_dmadesc_s *dma_rxdesc;
  struct esp32s3_dmadesc_s *dma_txdesc;
#endif
  uint32_t frequency;   /* Requested clock frequency */
  uint32_t actual;      /* Actual clock frequency */
  enum spi_mode_e mode; /* Actual SPI hardware mode */
  uint8_t nbits;        /* Actual SPI send/receive bits once transmission */
};

static struct spi_dev_s *g_spidev;
static struct lcd_dev_s *g_lcd;

#ifdef CONFIG_ESP32S3_SPI2_3WIRE_HALF
static void esp32s3_spibus_3wire_half_initialize()
{
  uint32_t regval;
  struct esp32s3_spi_priv_s* priv = (struct esp32s3_spi_priv_s*)g_spidev;
  const struct esp32s3_spi_config_s *config = priv->config;

  // regval = SPI_SIO_M | SPI_USR_MISO_M | SPI_USR_MOSI_M | SPI_CS_HOLD_M; /* read/write */
  regval = SPI_SIO_M | SPI_USR_MOSI_M | SPI_CS_HOLD_M; /* write */
  putreg32(regval, SPI_USER_REG(priv->config->id));

  esp32s3_configgpio(config->mosi_pin, OUTPUT|INPUT);
  esp32s3_gpio_matrix_out(config->mosi_pin, config->mosi_outsig, 0, 0);
  esp32s3_gpio_matrix_in(config->mosi_pin, config->mosi_insig, 0);

  /* cancel miso signal pin */
  esp32s3_gpio_matrix_in(0x3a, config->miso_insig, 0);

  esp32s3_configgpio(config->clk_pin, OUTPUT|INPUT);
  esp32s3_gpio_matrix_out(config->clk_pin, config->clk_outsig, 0, 0);
  esp32s3_gpio_matrix_in(config->clk_pin, config->clk_insig, 0);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  board_lcd_initialize
 *
 * Description:
 *   Initialize the LCD video hardware.  The initial state of the LCD is
 *   fully initialized, display memory cleared, and the LCD ready to use, but
 *   with the power setting at 0 (full off).
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_lcd_initialize(void)
{
  /* Initialize non-SPI GPIOs */

  esp32s3_configgpio(GPIO_LCD_DC, OUTPUT);
  esp32s3_configgpio(GPIO_LCD_RST, OUTPUT);
  esp32s3_configgpio(GPIO_LCD_BCKL, OUTPUT);

  /* Reset LCD */

  esp32s3_gpiowrite(GPIO_LCD_RST, false);
  nxsig_usleep(10 * 1000);
  esp32s3_gpiowrite(GPIO_LCD_RST, true);
  nxsig_usleep(10 * 1000);

  /* Turn on LCD backlight */

  esp32s3_gpiowrite(GPIO_LCD_BCKL, true);

  g_spidev = esp32s3_spibus_initialize(DISPLAY_SPI);
  if (!g_spidev)
    {
      lcderr("ERROR: Failed to initialize SPI port %d\n", DISPLAY_SPI);
      return -ENODEV;
    }

#ifdef CONFIG_ESP32S3_SPI2_3WIRE_HALF
  esp32s3_spibus_3wire_half_initialize();
#endif

  g_lcd = st7789_lcdinitialize(g_spidev);
  if (!g_lcd)
    {
      lcderr("ERROR: st7789_lcdinitialize() failed\n");
      return -ENODEV;
    }

  return OK;
}

/****************************************************************************
 * Name:  board_lcd_getdev
 *
 * Description:
 *   Return a a reference to the LCD object for the specified LCD.  This
 *   allows support for multiple LCD devices.
 *
 * Input Parameters:
 *   devno - LCD device nmber
 *
 * Returned Value:
 *   LCD device pointer if success or NULL if failed.
 *
 ****************************************************************************/

struct lcd_dev_s *board_lcd_getdev(int devno)
{
  if (!g_lcd)
    {
      lcderr("ERROR: Failed to bind SPI port 4 to LCD %d\n", devno);
    }
  else
    {
      lcdinfo("SPI port %d bound to LCD %d\n", DISPLAY_SPI, devno);
      return g_lcd;
    }

  return NULL;
}

/****************************************************************************
 * Name:  board_lcd_uninitialize
 *
 * Description:
 *   Uninitialize the LCD support
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void board_lcd_uninitialize(void)
{
  /* Turn the display off */

  g_lcd->setpower(g_lcd, 0);
}
