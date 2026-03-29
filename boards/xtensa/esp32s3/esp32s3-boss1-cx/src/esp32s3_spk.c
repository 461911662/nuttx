/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/src/esp32s3_spk.c
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

#include <stdio.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/audio/audio.h>
#include <nuttx/audio/audio_i2s.h>
#include <nuttx/audio/i2s.h>
#include <nuttx/audio/pcm.h>
#include <nuttx/ioexpander/ioexpander.h>

#include <arch/board/board.h>

#include "espressif/esp_i2s.h"
#include "esp32s3-boss1-cx.h"
#include "esp32s3_spk.h"
#include "esp32s3_gpioexp.h"

#ifdef CONFIG_ESP32S3_BOSS1_CX_SPEAKER

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SPK_ENABLE_PIN  BOARD_SPK_ENABLE_PIN
#define SPK_I2S_PORT    BOARD_SPK_I2S_PORT

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct esp32s3_spk_dev_s
{
  struct audio_lowerhalf_s dev;              /* Exported audio device */
  FAR struct audio_lowerhalf_s *lower;     /* Lower audio device (audio_i2s) */
  FAR struct ioexpander_dev_s *ioe;         /* XL9555 ioexpander handle */
  uint8_t enable_pin;                        /* Speaker enable pin on XL9555 */
  bool spk_on;                              /* Speaker power state */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int spk_getcaps(FAR struct audio_lowerhalf_s *dev, int type,
                       FAR struct audio_caps_s *caps);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_configure(FAR struct audio_lowerhalf_s *dev,
                         FAR void *session,
                         FAR const struct audio_caps_s *caps);
#else
static int spk_configure(FAR struct audio_lowerhalf_s *dev,
                         FAR const struct audio_caps_s *caps);
#endif
static int spk_shutdown(FAR struct audio_lowerhalf_s *dev);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_start(FAR struct audio_lowerhalf_s *dev, FAR void *session);
#else
static int spk_start(FAR struct audio_lowerhalf_s *dev);
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_stop(FAR struct audio_lowerhalf_s *dev, FAR void *session);
#else
static int spk_stop(FAR struct audio_lowerhalf_s *dev);
#endif
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_pause(FAR struct audio_lowerhalf_s *dev, FAR void *session);
static int spk_resume(FAR struct audio_lowerhalf_s *dev, FAR void *session);
#else
static int spk_pause(FAR struct audio_lowerhalf_s *dev);
static int spk_resume(FAR struct audio_lowerhalf_s *dev);
#endif
#endif
static int spk_enqueuebuffer(FAR struct audio_lowerhalf_s *dev,
                              FAR struct ap_buffer_s *apb);
static int spk_ioctl(FAR struct audio_lowerhalf_s *dev, int cmd,
                      unsigned long arg);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_reserve(FAR struct audio_lowerhalf_s *dev,
                        FAR void **session);
#else
static int spk_reserve(FAR struct audio_lowerhalf_s *dev);
#endif
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_release(FAR struct audio_lowerhalf_s *dev,
                        FAR void *session);
#else
static int spk_release(FAR struct audio_lowerhalf_s *dev);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct audio_ops_s g_spk_ops =
{
  spk_getcaps,           /* getcaps        */
  spk_configure,        /* configure      */
  spk_shutdown,          /* shutdown       */
  spk_start,             /* start          */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  spk_stop,              /* stop           */
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  spk_pause,             /* pause          */
  spk_resume,            /* resume         */
#endif
  NULL,                  /* allocbuffer    */
  NULL,                  /* freebuffer     */
  spk_enqueuebuffer,     /* enqueue_buffer */
  NULL,                  /* cancel_buffer  */
  spk_ioctl,             /* ioctl          */
  NULL,                  /* read           */
  NULL,                  /* write          */
  spk_reserve,           /* reserve        */
  spk_release            /* release        */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int spk_getcaps(FAR struct audio_lowerhalf_s *dev, int type,
                       FAR struct audio_caps_s *caps)
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

  return priv->lower->ops->getcaps(priv->lower, type, caps);
}

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_configure(FAR struct audio_lowerhalf_s *dev,
                         FAR void *session,
                         FAR const struct audio_caps_s *caps)
#else
static int spk_configure(FAR struct audio_lowerhalf_s *dev,
                         FAR const struct audio_caps_s *caps)
#endif
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  return priv->lower->ops->configure(priv->lower, session, caps);
#else
  return priv->lower->ops->configure(priv->lower, caps);
#endif
}

static int spk_shutdown(FAR struct audio_lowerhalf_s *dev)
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

  /* Ensure speaker is off */

  if (priv->spk_on)
    {
      IOEXP_WRITEPIN(priv->ioe, priv->enable_pin, false);
      priv->spk_on = false;
    }

  return priv->lower->ops->shutdown(priv->lower);
}

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_start(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int spk_start(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;
  int ret;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = priv->lower->ops->start(priv->lower, session);
#else
  ret = priv->lower->ops->start(priv->lower);
#endif
  if (ret == OK)
    {
      /* Enable speaker */

      IOEXP_WRITEPIN(priv->ioe, priv->enable_pin, true);
      priv->spk_on = true;
      audinfo("Speaker enabled\n");
    }

  return ret;
}

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_stop(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int spk_stop(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;
  int ret;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = priv->lower->ops->stop(priv->lower, session);
#else
  ret = priv->lower->ops->stop(priv->lower);
#endif
  if (ret == OK)
    {
      /* Disable speaker */

      IOEXP_WRITEPIN(priv->ioe, priv->enable_pin, false);
      priv->spk_on = false;
      audinfo("Speaker disabled\n");
    }

  return ret;
}
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_pause(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int spk_pause(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  return priv->lower->ops->pause(priv->lower, session);
#else
  return priv->lower->ops->pause(priv->lower);
#endif
}

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_resume(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int spk_resume(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  return priv->lower->ops->resume(priv->lower, session);
#else
  return priv->lower->ops->resume(priv->lower);
#endif
}
#endif

static int spk_enqueuebuffer(FAR struct audio_lowerhalf_s *dev,
                            FAR struct ap_buffer_s *apb)
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

  return priv->lower->ops->enqueuebuffer(priv->lower, apb);
}

static int spk_ioctl(FAR struct audio_lowerhalf_s *dev, int cmd,
                     unsigned long arg)
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

  return priv->lower->ops->ioctl(priv->lower, cmd, arg);
}

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_reserve(FAR struct audio_lowerhalf_s *dev,
                        FAR void **session)
#else
static int spk_reserve(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  return priv->lower->ops->reserve(priv->lower, session);
#else
  return priv->lower->ops->reserve(priv->lower);
#endif
}

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int spk_release(FAR struct audio_lowerhalf_s *dev,
                        FAR void *session)
#else
static int spk_release(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct esp32s3_spk_dev_s *priv = (FAR struct esp32s3_spk_dev_s *)dev;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  return priv->lower->ops->release(priv->lower, session);
#else
  return priv->lower->ops->release(priv->lower);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_spk_initialize
 *
 * Description:
 *   Initialize the speaker (NS4168) audio path with I2S1 and speaker
 *   power control via XL9555 IO0.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned to indicate the nature of the failure.
 *
 ****************************************************************************/

int esp32s3_spk_initialize(void)
{
  FAR struct esp32s3_spk_dev_s *priv;
  FAR struct ioexpander_dev_s *ioe;
  FAR struct i2s_dev_s *i2s;
  FAR struct audio_lowerhalf_s *audio_i2s;
  FAR struct audio_lowerhalf_s *pcm;
  char devname[8];
  int ret;

  /* Get XL9555 ioexpander handle */

  ioe = esp32s3_gpioexp_getioe();
  if (ioe == NULL)
    {
      auderr("ERROR: Failed to get GPIO expander handle\n");
      return -ENODEV;
    }

  /* Allocate speaker device structure */

  priv = kmm_zalloc(sizeof(struct esp32s3_spk_dev_s));
  if (priv == NULL)
    {
      auderr("ERROR: Failed to allocate speaker device\n");
      return -ENOMEM;
    }

  priv->ioe = ioe;
  if (priv->ioe == NULL)
    {
      auderr("ERROR: Failed to get GPIO expander handle\n");
      kmm_free(priv);
      return -ENODEV;
    }

  priv->enable_pin = SPK_ENABLE_PIN;
  priv->spk_on = false;

  /* Configure speaker enable pin as output, initially disabled */

  ret = IOEXP_SETDIRECTION(priv->ioe, priv->enable_pin,
                            IOEXPANDER_DIRECTION_OUT);
  if (ret < 0)
    {
      auderr("ERROR: Failed to set IO%d direction: %d\n",
             priv->enable_pin, ret);
      kmm_free(priv);
      return ret;
    }

  ret = IOEXP_WRITEPIN(priv->ioe, priv->enable_pin, false);
  if (ret < 0)
    {
      auderr("ERROR: Failed to set IO%d initial state: %d\n",
             priv->enable_pin, ret);
      kmm_free(priv);
      return ret;
    }

  /* Initialize I2S1 */

  i2s = esp_i2sbus_initialize(SPK_I2S_PORT);
  if (i2s == NULL)
    {
      auderr("ERROR: Failed to initialize I2S%d\n", SPK_I2S_PORT);
      kmm_free(priv);
      return -ENODEV;
    }

  /* Create audio I2S device for TX (playback) */

  audio_i2s = audio_i2s_initialize(i2s, true);
  if (audio_i2s == NULL)
    {
      auderr("ERROR: Failed to initialize audio I2S\n");
      kmm_free(priv);
      return -ENODEV;
    }

  /* Create PCM decoder wrapper */

  pcm = pcm_decode_initialize(audio_i2s);
  if (pcm == NULL)
    {
      auderr("ERROR: Failed to create PCM decoder\n");
      kmm_free(priv);
      return -ENODEV;
    }

  /* Initialize our speaker wrapper */

  priv->lower = pcm;
  priv->dev.ops = &g_spk_ops;

  /* Register audio device */

  snprintf(devname, sizeof(devname), "pcm%d", SPK_I2S_PORT);
  ret = audio_register(devname, &priv->dev);
  if (ret < 0)
    {
      auderr("ERROR: Failed to register /dev/%s device: %d\n",
             devname, ret);
      kmm_free(priv);
      return ret;
    }

  audinfo("Speaker driver initialized, /dev/%s registered\n", devname);
  return OK;
}
#endif /* CONFIG_ESP32S3_BOSS1_CX_SPEAKER */