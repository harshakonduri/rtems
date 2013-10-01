/*
 *  COPYRIGHT (c) 1989-2011.
 *  On-Line Applications Research Corporation (OAR).
 *  CPYRIGHT (c) SREE HARSHA KONDURI
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */

#include "tmacros.h"
#include "test_support.h"

/* functions */

rtems_task Init(
  rtems_task_argument argument
);

/* configuration information */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_SCHEDULER_GLOBALEDF
#define CONFIGURE_SMP_APPLICATION
#define CONFIGURE_SMP_MAXIMUM_PROCESSORS   4 

#define CONFIGURE_MAXIMUM_TASKS            \
    (1 + CONFIGURE_SMP_MAXIMUM_PROCESSORS)
#define CONFIGURE_MAXIMUM_SEMAPHORES 2

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#include <rtems/confdefs.h>

/* global variables */

/*
 *  Keep the names and IDs in global variables so another task can use them.
 */
