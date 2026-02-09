/****************************************************************************
 * arch/xtensa/src/boss1-esp32s3/esp32s3_ledc.c
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

#include <sys/param.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <errno.h>
#include <nuttx/config.h>

#include "esp32s3_clockconfig.h"
#include "esp32s3_gpio.h"
#include "esp32s3_ledc.h"
#include "esp32s3_pwm.h"

#include "xtensa.h"
#include "hardware/esp32s3_ledc.h"
#include "hardware/esp32s3_system.h"
#include "hardware/esp32s3_gpio_sigmap.h"
#include "esp32s3_irq.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LEDC total timers */

#define LEDC_TIMERS               (4)

/* LEDC total channels */

#if defined(CONFIG_PWM_NCHANNELS) && CONFIG_PWM_NCHANNELS > 1
#  define LEDC_CHANNELS           (8)
#else
#  define LEDC_CHANNELS           (4)
#endif

/* LEDC timer0 channels and offset */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM0
#  if defined(CONFIG_PWM_NCHANNELS) && CONFIG_PWM_NCHANNELS > 1
#    define LEDC_TIM0_CHANS       CONFIG_BOSS1_ESP32S3_LEDC_TIM0_CHANNELS
#  else
#    define LEDC_TIM0_CHANS       (1)
#  endif
#    define LEDC_TIM0_CHANS_OFF   (0)
#endif

/* LEDC timer1 channels and offset */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM1
#  if defined(CONFIG_PWM_NCHANNELS) && CONFIG_PWM_NCHANNELS > 1
#    define LEDC_TIM1_CHANS       CONFIG_BOSS1_ESP32S3_LEDC_TIM1_CHANNELS
#  else
#    define LEDC_TIM1_CHANS       (1)
#  endif
#  define LEDC_TIM1_CHANS_OFF     (LEDC_TIM0_CHANS_OFF + LEDC_TIM0_CHANS)
#endif

/* LEDC timer2 channels and offset */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM2
#  if defined(CONFIG_PWM_NCHANNELS) && CONFIG_PWM_NCHANNELS > 1
#    define LEDC_TIM2_CHANS       CONFIG_BOSS1_ESP32S3_LEDC_TIM2_CHANNELS
#  else
#    define LEDC_TIM2_CHANS       (1)
#  endif

#  define LEDC_TIM2_CHANS_OFF     (LEDC_TIM1_CHANS_OFF + LEDC_TIM1_CHANS)
#endif

/* LEDC timer3 channels and offset */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM3
#  if defined(CONFIG_PWM_NCHANNELS) && CONFIG_PWM_NCHANNELS > 1
#    define LEDC_TIM3_CHANS       CONFIG_BOSS1_ESP32S3_LEDC_TIM3_CHANNELS
#  else
#    define LEDC_TIM3_CHANS       (1)
#  endif

#  define LEDC_TIM3_CHANS_OFF     (LEDC_TIM2_CHANS_OFF + LEDC_TIM2_CHANS)
#endif

/* LEDC clock resource */

#define LEDC_CLK_RES_APB          (1)         /* APB clock */
#define LEDC_CLK_RES_RC_FAST      (2)         /* RC_FAST clock */
#define LEDC_CLK_RES_XTAL         (3)         /* XTAL clock */

/* LEDC clock source frequency */

#define LEDC_CLK_APB_FREQ         (80 * MHZ)      /* APB clock frequency */
#define LEDC_CLK_RC_FAST_FREQ     (17.5 * MHZ)    /* RC_FAST clock frequency */
#define LEDC_CLK_XTAL_FREQ        (40 * MHZ)      /* XTAL clock frequency */

/* LEDC timer max reload */

#define LEDC_RELOAD_MAX           (16384)    /* 2^14 */

/* LEDC timer max reload bit length */

#define LEDC_RELOAD_MAX_BIT_LEN   (14)

/* LEDC timer max clock divider parameter */

#define LEDC_CLKDIV_MAX           (1024)    /* 2^10 */

/* LEDC timer registers mapping */

#define LEDC_TIMER_REG(r, n)      ((r) + (n) * (LEDC_TIMER1_CONF_REG - \
                                                LEDC_TIMER0_CONF_REG))

/* LEDC timer channel registers mapping */

#define setbits(bs, a)            modifyreg32(a, 0, bs)
#define resetbits(bs, a)          modifyreg32(a, bs, 0)

#define LEDC_CHAN_REG(r, n)       ((r) + (n) * (LEDC_CH1_CONF0_REG - \
                                                LEDC_CH0_CONF0_REG))

#define SET_TIMER_BITS(t, r, b)   setbits(b, LEDC_TIMER_REG(r, (t)->num));
#define SET_TIMER_REG(t, r, v)    putreg32(v, LEDC_TIMER_REG(r, (t)->num));

#define SET_CHAN_BITS(c, r, b)    setbits(b, LEDC_CHAN_REG(r, (c)->num));
#define SET_CHAN_REG(c, r, v)     putreg32(v, LEDC_CHAN_REG(r, (c)->num));

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* LEDC timer channel configuration (BOSS1 specific with fade support) */

struct esp32s3_ledc_chan_s
{
  const uint8_t num;                    /* Timer channel ID */
  const uint8_t pin;                    /* Timer channel GPIO pin number */
  uint16_t duty;                        /* Timer channel current duty */
  uint16_t target_duty;                 /* Target duty for fade */
  uint32_t duty_time;                   /* Fade time in ms */
  bool auto_reverse;                    /* Auto reverse when fade ends */
  bool fade_direction;                  /* Internal: current fade direction */
};

/* This structure represents the state of one LEDC timer */

struct esp32s3_ledc_s
{
  const struct pwm_ops_s *ops;          /* PWM operations */

  const uint8_t num;                    /* Timer ID */

  const uint8_t channels;               /* Timer channels number */
  struct esp32s3_ledc_chan_s *chans;    /* Timer channels pointer */

  uint32_t frequency;                   /* Timer current frequency */
  uint32_t reload;                      /* Timer current reload */

  /* Interrupt data */
  int8_t cpuint;                        /* Allocated CPU interrupt number, -1 if not allocated */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int pwm_setup(struct pwm_lowerhalf_s *dev);
static int pwm_shutdown(struct pwm_lowerhalf_s *dev);
static int pwm_start(struct pwm_lowerhalf_s *dev,
                     const struct pwm_info_s *info);
static int pwm_stop(struct pwm_lowerhalf_s *dev);
static int pwm_ioctl(struct pwm_lowerhalf_s *dev, int cmd,
                     unsigned long arg);
static int ledc_chan_irqhandler(int irq, FAR void *context, FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* LEDC PWM operations */

static const struct pwm_ops_s g_pwmops =
{
  .setup       = pwm_setup,
  .shutdown    = pwm_shutdown,
  .start       = pwm_start,
  .stop        = pwm_stop,
  .ioctl       = pwm_ioctl
};

/* LEDC channels table */

static struct esp32s3_ledc_chan_s g_ledc_chans[LEDC_CHANNELS] =
{
  {
    .num       = 0,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL0_PIN
  },

  {
    .num       = 1,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL1_PIN
  },

  {
    .num       = 2,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL2_PIN
  },

  {
    .num       = 3,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL3_PIN
  },

#if LEDC_CHANNELS > 4
  {
    .num       = 4,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL4_PIN
  },

  {
    .num       = 5,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL5_PIN
  },

  {
    .num       = 6,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL6_PIN
  },

  {
    .num       = 7,
    .pin       = CONFIG_BOSS1_ESP32S3_LEDC_CHANNEL7_PIN
  }
#endif
};

/* LEDC timer0 private data */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM0
static struct esp32s3_ledc_s g_pwm0dev =
{
  .ops         = &g_pwmops,
  .num         = 0,
  .channels    = LEDC_TIM0_CHANS,
  .chans       = &g_ledc_chans[LEDC_TIM0_CHANS_OFF],
  .cpuint      = -1
};
#endif /* CONFIG_BOSS1_ESP32S3_LEDC_TIM0 */

/* LEDC timer1 private data */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM1
static struct esp32s3_ledc_s g_pwm1dev =
{
  .ops         = &g_pwmops,
  .num         = 1,
  .channels    = LEDC_TIM1_CHANS,
  .chans       = &g_ledc_chans[LEDC_TIM1_CHANS_OFF],
  .cpuint      = -1
};
#endif /* CONFIG_BOSS1_ESP32S3_LEDC_TIM1 */

/* LEDC timer2 private data */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM2
static struct esp32s3_ledc_s g_pwm2dev =
{
  .ops         = &g_pwmops,
  .num         = 2,
  .channels    = LEDC_TIM2_CHANS,
  .chans       = &g_ledc_chans[LEDC_TIM2_CHANS_OFF],
  .cpuint      = -1
};
#endif /* CONFIG_BOSS1_ESP32S3_LEDC_TIM2 */

/* LEDC timer3 private data */

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM3
static struct esp32s3_ledc_s g_pwm3dev =
{
  .ops         = &g_pwmops,
  .num         = 3,
  .channels    = LEDC_TIM3_CHANS,
  .chans       = &g_ledc_chans[LEDC_TIM3_CHANS_OFF],
  .cpuint      = -1
};
#endif /* CONFIG_BOSS1_ESP32S3_LEDC_TIM3 */

/* Clock source */

static uint32_t clk_src = 0;

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * Name: ledc_enable_clk
 *
 * Description:
 *   Enable LEDC clock.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void ledc_enable_clk(void)
{
  irqstate_t flags;

  flags = enter_critical_section();

  if (clk_src == 0)
    {
      setbits(SYSTEM_LEDC_CLK_EN, SYSTEM_PERIP_CLK_EN0_REG);
      resetbits(SYSTEM_LEDC_RST, SYSTEM_PERIP_RST_EN0_REG);

      putreg32(LEDC_CLK_RES_APB, LEDC_CONF_REG);
      setbits(LEDC_CLK_EN, LEDC_CONF_REG);

      /* We set default clock is APB. */

      clk_src = LEDC_CLK_RES_APB;

      pwminfo("Enable ledc clock\n");
    }

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: ledc_disable_clk
 *
 * Description:
 *   Disable LEDC clock.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void ledc_disable_clk(void)
{
  irqstate_t flags;

  flags = enter_critical_section();

  if (clk_src != 0)
    {
      pwminfo("Disable ledc clock\n");

      setbits(SYSTEM_LEDC_RST, SYSTEM_PERIP_RST_EN0_REG);
      resetbits(SYSTEM_LEDC_CLK_EN, SYSTEM_PERIP_CLK_EN0_REG);
      clk_src = 0;
    }

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: setup_timer
 *
 * Description:
 *   Setup LEDC timer frequency and reload.
 *
 * Input Parameters:
 *   priv - A reference to the LEDC timer state structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void setup_timer(struct esp32s3_ledc_s *priv)
{
  irqstate_t flags;
  uint32_t regval;
  uint32_t reload;
  float prescaler;
  uint32_t integral_prescaler;
  uint32_t fractional_prescaler;
  uint8_t shift;
  uint64_t pwmclk = LEDC_CLK_APB_FREQ;

  /* Determine the using clock source and set pwmclk */

  switch (clk_src)
  {
  case LEDC_CLK_RES_APB:

    /* use APB clock */

    pwmclk = LEDC_CLK_APB_FREQ;
    break;

  case LEDC_CLK_RES_RC_FAST:

    /* use RC_FAST clock */

    pwmclk = LEDC_CLK_RC_FAST_FREQ;
    break;

  case LEDC_CLK_RES_XTAL:

    /* use XTAL clock */

    pwmclk = LEDC_CLK_XTAL_FREQ;
    break;

  default:
    pwmerr("Invalid clock source or no clock has been inited !");
    break;
  }

  /* Reset timer */

  SET_TIMER_BITS(priv, LEDC_TIMER0_CONF_REG, LEDC_TIMER0_RST);

  /* Calculate optimal values for the timer prescaler and for the timer
   * modulo register.  If 'frequency' is the desired frequency, then
   *
   *   tpmclk = pwmclk / presc
   *   frequency = tpmclk / reload
   *
   * ==>
   *
   *   reload = pwmclk / presc / frequency
   *
   * In ESP32S3, there are 3 clock resources for PWM:
   *
   *   1. APB clock (80 MHz)
   *   2. RC_FAST_CLK (17.5 MHz)
   *   3. XTAL_CLK (40 MHZ)
   *
   * We mostly use APB clock generally.
   *
   * There are many solutions to this, but the best solution will be the one
   * that has the largest reload value and the smallest prescaler value.
   * That is the solution that should give us the most accuracy in the timer
   * control.  Subject to:
   *
   *   2 <= presc  <= 2^14(16384)
   *   1 <= clkdiv <= 2^10
   *
   * clkdiv has 10-bit integral precision and 8-bit fractional precision, so
   * clkdiv = pwmclk / 16384 / frequency would be optimal.
   *
   * Example:
   *
   *  pwmclk    = 80 MHz
   *  frequency = 100 Hz
   *
   *  presc     = 80,000,000 * 256 / 16,384 / 100
   *            = 12,500
   *  timclk    = 80,000,000 / (12,500 / 256)
   *            = 1,638,400
   *  counter   = 1,638,400 / 100
   *            = 16,384
   *            = 2^14
   *  shift     = 14
   */

  /* Search the maximum value of timer reload value */

  for (reload = LEDC_RELOAD_MAX , shift = LEDC_RELOAD_MAX_BIT_LEN;
       reload > 1;
       reload = (reload >> 1), shift -= 1)
      {
        if (reload * priv->frequency <= pwmclk)
        {
          break;
        }
  }

  /* Caculate the prescaler */

  prescaler = (float)pwmclk / priv->frequency / reload;

  /* Get the integral part */

  integral_prescaler = (uint32_t) prescaler;

  /* Get the fractional part. To write to the registers, value need to be
  * multiply by 256
  */

  fractional_prescaler = (uint32_t)((prescaler - integral_prescaler) * 256);

  /* Prevent prescaler goto 0. In esp32 series, prescaler == 1 means
  * clock signal goes pass-through
  */

  if (integral_prescaler == 0) integral_prescaler = 1;

  pwminfo("PWM timer%" PRIu8 " frequency=%0.4f reload=%" PRIu32 " shift=%"
          PRIu32 " prescaler=%0.4f\n",
          priv->num, (float)pwmclk / reload / ((float)prescaler),
          reload, shift, (float)prescaler);

  /* Store reload for channel duty */

  priv->reload = reload;

  flags = enter_critical_section();

  /* Set timer clock divide and reload */

  regval = (shift << LEDC_TIMER0_DUTY_RES_S) |
           (fractional_prescaler << LEDC_CLK_DIV_TIMER0_S) |
           (integral_prescaler << LEDC_CLK_DIV_TIMER0_S << 8);
  SET_TIMER_REG(priv, LEDC_TIMER0_CONF_REG, regval);

  /* Update clock divide and reload to hardware */

  SET_TIMER_BITS(priv, LEDC_TIMER0_CONF_REG, LEDC_TIMER0_PARA_UP);

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: setup_channel
 *
 * Description:
 *   Setup LEDC timer channel duty.
 *
 * Input Parameters:
 *   priv - A reference to the LEDC timer state structure
 *   cn   - Timer channel number
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void setup_channel(struct esp32s3_ledc_s *priv, int cn)
{
  irqstate_t flags;
  uint32_t regval;
  struct esp32s3_ledc_chan_s *chan = &priv->chans[cn];

  /* Duty cycle:
   *
   * duty cycle = duty / 65536 * reload (fractional value)
   */

  regval = b16toi(chan->duty * priv->reload + b16HALF);

  pwminfo("channel=%" PRIu8 " duty=%" PRIu16 "(%0.4f) regval=%" PRIu32
          " reload=%" PRIu32 "\n",
          chan->num, chan->duty, (float)chan->duty / UINT16_MAX,
          regval, priv->reload);

  flags = enter_critical_section();

  /* Reset config 0 & 1 registers */

  SET_CHAN_REG(chan, LEDC_CH0_CONF0_REG, 0);
  SET_CHAN_REG(chan, LEDC_CH0_CONF1_REG, 0);

  /* Select the clock source */

  SET_CHAN_REG(chan, LEDC_CH0_CONF0_REG, priv->num);

  /* Set pulse phase 0 */

  SET_CHAN_REG(chan, LEDC_CH0_HPOINT_REG, 0);

  /* Duty register uses bits [18:4]  */

  SET_CHAN_REG(chan, LEDC_CH0_DUTY_REG, regval << 4);

  /* Start GPIO output  */

  SET_CHAN_BITS(chan, LEDC_CH0_CONF0_REG, LEDC_SIG_OUT_EN_CH0);

  /* Start Duty counter  */

  SET_CHAN_BITS(chan, LEDC_CH0_CONF1_REG, LEDC_DUTY_START_CH0);

  /* Update duty and phase to hardware */

  SET_CHAN_BITS(chan, LEDC_CH0_CONF0_REG, LEDC_PARA_UP_CH0);

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: pwm_setup
 *
 * Description:
 *   This method is called when the driver is opened.  The lower half driver
 *   should configure and initialize the device so that it is ready for use.
 *   It should not, however, output pulses until the start method is called.
 *
 * Input Parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_setup(struct pwm_lowerhalf_s *dev)
{
  struct esp32s3_ledc_s *priv = (struct esp32s3_ledc_s *)dev;

  pwminfo("PWM timer%u\n", priv->num);

  ledc_enable_clk();

  /* Initialize cpuint to -1 (not allocated) */
  priv->cpuint = -1;

  /* Setup channel GPIO pins */

  for (int i = 0; i < priv->channels; i++)
    {
      pwminfo("channel%d --> pin%d\n", priv->chans[i].num,
              priv->chans[i].pin);

      esp32s3_configgpio(priv->chans[i].pin, OUTPUT | PULLUP);
      esp32s3_gpio_matrix_out(priv->chans[i].pin,
                              LEDC_LS_SIG_OUT0_IDX + priv->chans[i].num,
                              0, 0);

      /* Initialize fade state */
      priv->chans[i].auto_reverse = false;
      priv->chans[i].target_duty = 0;
      priv->chans[i].duty_time = 0;
    }

  return 0;
}

/****************************************************************************
 * Name: pwm_shutdown
 *
 * Description:
 *   This method is called when the driver is closed.  The lower half driver
 *   stop pulsed output, free any resources, disable the timer hardware, and
 *   put the system into the lowest possible power usage state
 *
 * Input Parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_shutdown(struct pwm_lowerhalf_s *dev)
{
  struct esp32s3_ledc_s *priv = (struct esp32s3_ledc_s *)dev;
#ifdef CONFIG_PWM_NCHANNELS
  int channels = MIN(priv->channels, CONFIG_PWM_NCHANNELS);
#else
  int channels = 1;
#endif

  /* Stop timer */

  pwm_stop(dev);

  /* Clear timer and channel configuration */

  priv->frequency = 0;
  priv->reload    = 0;
  for (int i = 0; i < channels; i++)
    {
      priv->chans[i].duty = 0;
      priv->chans[i].auto_reverse = false;
      priv->chans[i].target_duty = 0;
      priv->chans[i].duty_time = 0;
    }

  /* Detach and free LEDC interrupt */
  if (priv->cpuint >= 0)
    {
      irq_detach(BOSS1_ESP32S3_IRQ_LEDC);
      esp32s3_teardown_irq(0, BOSS1_ESP32S3_PERIPH_LEDC, priv->cpuint);
      priv->cpuint = -1;
      pwminfo("LEDC interrupt detached and freed\n");
    }

  ledc_disable_clk();

  return 0;
}

/****************************************************************************
 * Name: pwm_start
 *
 * Description:
 *   (Re-)initialize the timer resources and start the pulsed output
 *
 * Input Parameters:
 *   dev  - A reference to the lower half PWM driver state structure
 *   info - A reference to the characteristics of the pulsed output
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_start(struct pwm_lowerhalf_s *dev,
                     const struct pwm_info_s *info)
{
  struct esp32s3_ledc_s *priv = (struct esp32s3_ledc_s *)dev;
#ifdef CONFIG_PWM_NCHANNELS
  int channels = MIN(priv->channels, CONFIG_PWM_NCHANNELS);
#else
  int channels = 1;
#endif

  pwminfo("PWM timer%d\n", priv->num);

   /* Update timer with given PWM timer frequency */

   pwminfo("freq: info=%u priv=%u\n", info->frequency, priv->frequency);

   if (priv->frequency != info->frequency)
     {
       pwminfo("Calling setup_timer\n");
       priv->frequency = info->frequency;
       setup_timer(priv);
     }

  /* Update timer with given PWM channel duty */

  for (int i = 0; i < channels; i++)
    {
#ifdef CONFIG_PWM_NCHANNELS
      if (priv->chans[i].duty != info->channels[i].duty)
#else
      if (priv->chans[i].duty != info[i].duty)
#endif
        {
#ifdef CONFIG_PWM_NCHANNELS
          priv->chans[i].duty = info->channels[i].duty;
#else
          priv->chans[i].duty = info[i].duty;
#endif
          setup_channel(priv, i);
        }
    }

  return 0;
}

/****************************************************************************
 * Name: pwm_stop
 *
 * Description:
 *   Stop the pulsed output and reset the timer resources.
 *
 * Input Parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_stop(struct pwm_lowerhalf_s *dev)
{
  irqstate_t flags;
  struct esp32s3_ledc_s *priv = (struct esp32s3_ledc_s *)dev;

  pwminfo("PWM timer%d\n", priv->num);

  flags = enter_critical_section();

  /* Stop timer */

  SET_TIMER_BITS(priv, LEDC_TIMER0_CONF_REG, LEDC_TIMER0_PAUSE);

  /* Reset timer */

  SET_TIMER_BITS(priv, LEDC_TIMER0_CONF_REG, LEDC_TIMER0_RST);

  leave_critical_section(flags);
  return 0;
}

/****************************************************************************
 * Name: pwm_ioctl
 *
 * Description:
 *   Lower-half logic may support platform-specific ioctl commands
 *
 * Input Parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *   cmd - The ioctl command
 *   arg - The argument accompanying the ioctl command
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_ioctl(struct pwm_lowerhalf_s *dev, int cmd,
                     unsigned long arg)
{
  struct esp32s3_ledc_s *priv = (struct esp32s3_ledc_s *)dev;
  irqstate_t flags;
  int ret = OK;

  pwminfo("PWM timer%d cmd=%d\n", priv->num, cmd);

  switch (cmd)
    {
      /* PWMIOC_START_FADE - Start fade on the channel */

      case PWMIOC_START_FADE:
        {
          struct pwm_fade_s *fade = (struct pwm_fade_s *)arg;
          uint8_t ch = fade->channel;

          if (ch >= priv->channels)
            {
              pwmerr("ERROR: Invalid channel %d\n", ch);
              return -EINVAL;
            }

          struct esp32s3_ledc_chan_s *chan = &priv->chans[ch];

          pwminfo("START_FADE: ch=%u, auto_rev=%d\n",
                  ch, fade->auto_reverse);
          pwminfo("  target_duty=%u, duty_time=%u\n",
                  (unsigned)fade->target_duty, (unsigned)fade->duty_time);

          flags = enter_critical_section();

          chan->auto_reverse = fade->auto_reverse;
          chan->duty_time = fade->duty_time;

          uint32_t current_reg = getreg32(LEDC_CHAN_REG(LEDC_CH0_DUTY_R_REG, ch)) >> 4;
          uint32_t target_reg = b16toi(fade->target_duty * priv->reload + b16HALF);
          bool fade_inc = (target_reg > current_reg);
          chan->fade_direction = fade_inc;
          chan->target_duty = target_reg;

          // sync current_reg -> duty_reg
          SET_CHAN_REG(chan, LEDC_CH0_DUTY_REG, current_reg << 4);
          SET_CHAN_BITS(chan, LEDC_CH0_CONF0_REG, LEDC_PARA_UP_CH0);

          uint32_t duty_change = (fade_inc ?
            (target_reg - current_reg) :
            (current_reg - target_reg));

          uint32_t num;
          uint32_t scale;
          uint32_t cycle;

          if (duty_change == 0)
            {
              num = 1;
              scale = 1;
              cycle = 1;
            }
          else if (chan->duty_time >= duty_change)
            {
              num = duty_change;
              scale = 1;
              cycle = chan->duty_time / duty_change;
            }
          else
            {
              num = chan->duty_time;
              scale = duty_change / chan->duty_time;
              cycle = 1;
            }

          pwminfo("START_FADE: ch=%u dir=%s change=%u n=%u s=%u c=%u\n",
                  ch, fade_inc ? "RISE" : "FALL",
                  (unsigned)duty_change, (unsigned)num, (unsigned)scale, (unsigned)cycle);

          SET_CHAN_REG(chan, LEDC_CH0_CONF1_REG, 0);

          uint32_t conf1_reg = LEDC_DUTY_START_CH0 |
                                (fade_inc ? LEDC_DUTY_INC_CH0 : 0) |
                                (num << LEDC_DUTY_NUM_CH0_S) |
                                (cycle << LEDC_DUTY_CYCLE_CH0_S) |
                                (scale << LEDC_DUTY_SCALE_CH0_S);

          SET_CHAN_REG(chan, LEDC_CH0_CONF1_REG, conf1_reg);
          SET_CHAN_BITS(chan, LEDC_CH0_CONF0_REG, LEDC_PARA_UP_CH0);

           /* Install interrupt handler and enable if auto_reverse is enabled */
           if (fade->auto_reverse && priv->cpuint < 0)
             {
               /* Allocate CPU interrupt */
               int cpuint = esp32s3_setup_irq(0, BOSS1_ESP32S3_PERIPH_LEDC, 1,
                                             BOSS1_ESP32S3_CPUINT_FLAG_IRAM);
               if (cpuint < 0)
                 {
                   pwmerr("ERROR: Failed to setup LEDC IRQ: %d\n", cpuint);
                   leave_critical_section(flags);
                   return cpuint;
                 }

               priv->cpuint = cpuint;

               /* Attach interrupt handler */
               ret = irq_attach(BOSS1_ESP32S3_IRQ_LEDC, ledc_chan_irqhandler, priv);
               if (ret < 0)
                 {
                   pwmerr("ERROR: Failed to attach LEDC IRQ: %d\n", ret);
                   esp32s3_teardown_irq(0, BOSS1_ESP32S3_PERIPH_LEDC, priv->cpuint);
                   priv->cpuint = -1;
                   leave_critical_section(flags);
                   return ret;
                 }

               /* Clear interrupt flag for this channel */
               setbits(LEDC_DUTY_CHNG_END_CH0_INT_CLR << ch, LEDC_INT_CLR_REG);

               /* Enable CPU interrupt */
               up_enable_irq(BOSS1_ESP32S3_IRQ_LEDC);

               /* Enable LEDC hardware interrupt for this channel */
               uint32_t int_ena = getreg32(LEDC_INT_ENA_REG);
               int_ena |= (LEDC_DUTY_CHNG_END_CH0_INT_ENA << ch);
               putreg32(int_ena, LEDC_INT_ENA_REG);
             }

           leave_critical_section(flags);
         }
         break;

      /* PWMIOC_STOP_FADE - Disable interrupt, detach, and stop fade */

      case PWMIOC_STOP_FADE:
        {
          uint8_t ch = (uint8_t)arg;

          if (ch >= priv->channels)
            {
              pwmerr("ERROR: Invalid channel %d\n", ch);
              return -EINVAL;
            }

          pwminfo("STOP_FADE: ch=%d\n", ch);

          SET_CHAN_REG(&priv->chans[ch], LEDC_CH0_CONF1_REG, 0);

          SET_CHAN_BITS(&priv->chans[ch], LEDC_CH0_CONF0_REG, LEDC_PARA_UP_CH0);

          priv->chans[ch].auto_reverse = false;
          priv->chans[ch].target_duty = 0;
          priv->chans[ch].duty_time = 0;

          /* Detach and free interrupt if previously allocated */
          if (priv->cpuint >= 0)
            {
              irq_detach(BOSS1_ESP32S3_IRQ_LEDC);
              esp32s3_teardown_irq(0, BOSS1_ESP32S3_PERIPH_LEDC, priv->cpuint);
              priv->cpuint = -1;
            }
        }
        break;

      default:
        ret = -ENOTTY;
        break;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_ledc_init
 *
 * Description:
 *   Initialize one LEDC timer for use with the upper_level PWM driver.
 *
 * Input Parameters:
 *   timer - A number identifying the timer use.
 *
 * Returned Value:
 *   On success, a pointer to the ESP32S3 LEDC lower half PWM driver is
 *   returned. NULL is returned on any failure.
 *
 ****************************************************************************/

struct pwm_lowerhalf_s *esp32s3_ledc_init(int timer)
{
  struct esp32s3_ledc_s *lower = NULL;

  pwminfo("TIM%u\n", timer);

  switch (timer)
    {
#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM0
      case 0:
        lower = &g_pwm0dev;
        break;
#endif

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM1
      case 1:
        lower = &g_pwm1dev;
        break;
#endif

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM2
      case 2:
        lower = &g_pwm2dev;
        break;
#endif

#ifdef CONFIG_BOSS1_ESP32S3_LEDC_TIM3
      case 3:
        lower = &g_pwm3dev;
        break;
#endif

      default:
        pwmerr("ERROR: No such timer configured %d\n", timer);
        lower = NULL;
        break;
    }

  return (struct pwm_lowerhalf_s *)lower;
}

/****************************************************************************
 * Name: ledc_chan_irqhandler
 *
 * Description:
 *   LEDC channel interrupt handler for duty change end.
 *   Handles auto-reverse functionality when fade completes.
 *
 ****************************************************************************/

static int IRAM_ATTR ledc_chan_irqhandler(int irq, FAR void *context, FAR void *arg)
{
  struct esp32s3_ledc_s *priv = (struct esp32s3_ledc_s *)arg;
  if (priv == NULL) {
    pwmerr("ERROR: LEDC IRQ handler called with NULL arg\n");
    return 0;
  }

  /* Read masked interrupt status */
  uint32_t int_st = getreg32(LEDC_INT_ST_REG);

  if (int_st == 0) {
    return 0;
  }

  // pwminfo("LEDC int_st=0x%08x\n", int_st);

  for (int ch = 0; ch < priv->channels; ch++)
    {
      /* Check if this channel triggered DUTY_CHNG_END interrupt */
      if ((int_st & (LEDC_DUTY_CHNG_END_CH0_INT_ST << ch)) == 0)
        {
          continue;
        }

      struct esp32s3_ledc_chan_s *chan = &priv->chans[ch];
      SET_CHAN_REG(chan, LEDC_CH0_CONF1_REG, 0);

      // pwminfo("IRQ: ch=%d auto_rev=%d\n", ch, chan->auto_reverse);

      if (chan->auto_reverse)
        {
          uint32_t current_reg = getreg32(LEDC_CHAN_REG(LEDC_CH0_DUTY_R_REG, ch)) >> 4;
          uint32_t target_reg = chan->fade_direction ? 0 :
            b16toi(chan->target_duty * priv->reload + b16HALF);
          chan->fade_direction = !chan->fade_direction;

          // pwminfo("AUTO_REVERSE: %s -> %s cur=%u tgt=%u\n",
          //        chan->fade_direction ? "FALL" : "RISE",
          //        chan->fade_direction ? "RISE" : "FALL",
          //        (unsigned)current_reg, (unsigned)target_reg);

          SET_CHAN_REG(chan, LEDC_CH0_DUTY_REG, current_reg << 4);
          SET_CHAN_BITS(chan, LEDC_CH0_CONF0_REG, LEDC_PARA_UP_CH0);

          uint32_t duty_change = (target_reg > current_reg) ?
            (target_reg - current_reg) : (current_reg - target_reg);

          uint32_t num;
          uint32_t scale;
          uint32_t cycle;

          if (duty_change == 0)
            {
              num = 1;
              scale = 1;
              cycle = 1;
            }
          else if (chan->duty_time >= duty_change)
            {
              num = duty_change;
              scale = 1;
              cycle = chan->duty_time / duty_change;
            }
          else
            {
              num = chan->duty_time;
              scale = duty_change / chan->duty_time;
              cycle = 1;
            }

          //  pwminfo("AUTO_REVERSE: n=%u s=%u c=%u\n",
          //         (unsigned)num, (unsigned)scale, (unsigned)cycle);

          uint32_t conf1_reg = LEDC_DUTY_START_CH0 |
                                (chan->fade_direction ? LEDC_DUTY_INC_CH0 : 0) |
                                (num << LEDC_DUTY_NUM_CH0_S) |
                                (cycle << LEDC_DUTY_CYCLE_CH0_S) |
                                (scale << LEDC_DUTY_SCALE_CH0_S);

          SET_CHAN_REG(chan, LEDC_CH0_CONF1_REG, conf1_reg);
          SET_CHAN_BITS(chan, LEDC_CH0_CONF0_REG, LEDC_PARA_UP_CH0);
        }
      /* Clear interrupt flag for this channel */
      setbits(LEDC_DUTY_CHNG_END_CH0_INT_CLR << ch, LEDC_INT_CLR_REG);
    }

  return 0;
}
