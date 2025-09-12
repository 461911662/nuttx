/**
 * @file bt_ioctl.h
 * @brief boss1 esp32 bluetooth ictol cmds
 * @attention 此代码不能用于商业用途
*/

#ifndef __ARCH_XTENSA_BOSS1_ESP32_BTIOCTL_H
#define __ARCH_XTENSA_BOSS1_ESP32_BTIOCTL_H

/****************************************************************************
 * INCLUDES
 ****************************************************************************/
#include <stdbool.h>
#include <nuttx/fs/ioctl.h>

/****************************************************************************
 * DEFINES
 ****************************************************************************/
#define _BTIOCBASE      (0x8c00) /* Wireless modules ioctl network commands */

#define _BTIOCVALID(c) (_IOC_TYPE(c)==_BTIOCBASE)
#define _BTIOC(nr)     _IOC(_BTIOCBASE,nr)

#define BIOC_POWERON                     _BTIOC(0)
#define BIOC_POWEROFF                    _BTIOC(1)
#define BIOC_GETSENDOK                   _BTIOC(2)

struct btparam_s
{
  union {
    bool is_host_send; /* 表示host可以发送数据包 */
  } resp;
};

#endif /* __ARCH_XTENSA_BOSS1_ESP32_BTIOCTL_H */