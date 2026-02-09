/****************************************************************************
 * boss1_esp32s3_pwm.h
 *
 * BOSS1 ESP32-S3 specific PWM definitions
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 *
 ****************************************************************************/

#ifndef __BOSS1_ESP32S3_PWM_H
#define __BOSS1_ESP32S3_PWM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <nuttx/timers/pwm.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* PWM fade configuration for BOSS1 */

struct pwm_fade_s
{
  uint8_t channel;           /* Channel number */
  bool auto_reverse;         /* Auto reverse direction when fade ends */
  uint16_t target_duty;     /* Target duty cycle value (0-65535) */
  uint32_t duty_time;       /* Fade time in milliseconds */
};

/****************************************************************************
 * Public Definitions
 ****************************************************************************/

/* IOCTL Commands */

#define PWMIOC_START_FADE     0x1001  /* Start fade on channel */
#define PWMIOC_STOP_FADE      0x1002  /* Stop fade on channel */
#define PWMIOC_SET_DUTY       0x1003  /* Set duty cycle */

#endif /* __BOSS1_ESP32S3_PWM_H */
