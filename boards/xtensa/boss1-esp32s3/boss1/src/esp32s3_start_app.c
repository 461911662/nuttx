/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <syslog.h>
#include <errno.h>
#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <nuttx/kmalloc.h>

/****************************************************************************
 * DEFINES
 ****************************************************************************/
#define BOSS_APPLICATION_STACKSIZE  16240
#define BOSS_APPLICATION_PRIORITY   100

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int weak_function watch_ui_main(int argc, FAR char *argv[])
{
  syslog(LOG_INFO, "INFO: Not impletement yet\n");
  while(1){};
}

/****************************************************************************
 * Name: start_app_by_cpu
 *
 * Description:
 *   This function is called by platform-specific bringup logic to startup app
 *   from special cpu.
 *
 *   CONFIG_START_APP_BY_CPU=y :
 *     Called from esp32s3_bringup().
 *
 ****************************************************************************/

int start_app_by_cpu(uint8_t cpu)
{
  FAR struct tcb_s *tcb;
  pid_t pid;
  int ret;

  /* Allocate a TCB for the new task. */

  tcb = kmm_zalloc(sizeof(struct task_tcb_s));
  if (!tcb)
    {
      syslog(LOG_ERR, "ERROR: Failed to allocate TCB\n");
      return -ENOMEM;
    }

  /* Setup the task type */
  tcb->flags = TCB_FLAG_TTYPE_TASK | TCB_FLAG_CPU_LOCKED | TCB_FLAG_FREE_TCB;

  /* Initialize the task */

  ret = nxtask_init((FAR struct task_tcb_s *)tcb, "AppBringUp",
                    BOSS_APPLICATION_PRIORITY, NULL, BOSS_APPLICATION_STACKSIZE,
                    watch_ui_main, NULL, environ, NULL);
  if (ret < OK)
    {
      kmm_free(tcb);
      return ret;
    }

  /* Get the assigned pid before we start the task */

  pid = tcb->pid;

UNUSED(cpu);
  /* Change tcb->cpu */
#ifdef CONFIG_SMP
  tcb->cpu = cpu;
  tcb->lockcount = 1;
#else
  UNUSED(cpu);
#endif

  /* Activate the task */

  nxtask_activate(tcb);

  return pid;
}