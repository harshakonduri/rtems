/**
 * @file
 *
 * @brief Global EDF Scheduler API
 *
 * @ingroup ScoreSchedulerGlobalEDF
 */

/*
 *  Copyright (C) 2011 On-Line Applications Research Corporation (OAR).
 *
 *  Copyright (c) 2013 embedded brains GmbH.
 *
 *  Copyright (c) 2013 Sree Harsha Konduri
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_SCHEDULERGLOBAL_EDF_H
#define _RTEMS_SCORE_SCHEDULERGLOBAL_EDF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rtems/score/scheduler.h>
#include <rtems/score/schedulersimple.h>
#include <rtems/score/schedulerpriority.h>
#include <rtems/score/scheduleredf.h>
#include <rtems/score/smplock.h>
/**
 * @defgroup ScoreSchedulerGlobalEDF Global EDF Scheduler
 *
 * @ingroup ScoreScheduler
 *
 * The G-EDF Scheduler extends the EDF Scheduler for a SMP System with more than
 * one Processor/Core. It allocates a processor for the processor count
 * ready threads based on their deadlines/priorities.  The thread priority/deadline is
 * the only information to determine the scheduling decision.
 * Threads with an allocated processor are in the scheduled chain.  After
 * initialization the scheduled chain has exactly processor count nodes.  Each
 * processor has exactly one allocated thread after initialization. 
 * The ready queue is a Red-Black Tree that is built on the deadline/priority of the threads.
 * All enqueue and extract operations may exchange threads with the scheduled
 * chain.  One thread will be added and another will be removed.  The scheduled chain
 * and ready queue are ordered according to the thread priority order.  The
 * insert operations for the scheduled chain are O(count of number of cores), and the
 * ready queue insert operations take O(logarithm(number of ready threads)).
 *
 * The thread preempt mode will be ignored.
 *
 * @{
 */

typedef enum {
    THREAD_IN_READY_QUEUE, 
    THREAD_IN_SCHEDULED_QUEUE,
  } THREAD_PLACED_IN;

typedef struct {
  /* Ready Queue using the RedBlack Tree Structure*/
  RBTree_Control ready;
  /* Chain Control that uses the scheduled tasks */
  Chain_Control scheduled;
   /* THREAD LOCATION IDENTIFICATION*/
  THREAD_PLACED_IN thread_location;
  /*Instance of SMP_lock for locking ready queue*/
  SMP_lock_Control smp_lock_ready_queue;

} Scheduler_global_EDF_Control;

  typedef struct {
  /* Use an integer to identify whether a task is on ready or scheduled queues */
  /* THREAD LOCATION IDENTIFICATION*/
  THREAD_PLACED_IN thread_location;

  /* The state of tasks in the queue */
  Scheduler_EDF_Queue_state queue_state;

  /* RBTree Node information*/
  RBTree_Node Node;  /* Use an integer to identify whether a task is on ready or scheduled queues */

    /* Thread information of every thread*/
  Thread_Control *thread;

  } Scheduler_global_EDF_Per_thread;

/**
 * @brief Entry points for the Global EDF Scheduler.
 */
#define SCHEDULER_GLOBAL_EDF_ENTRY_POINTS \
  { \
    _Scheduler_global_EDF_Initialize, \
    _Scheduler_global_EDF_Schedule, \
    _Scheduler_global_EDF_Yield, \
    _Scheduler_global_EDF_Extract, \
    _Scheduler_global_EDF_Allocate, \
    _Scheduler_EDF_Free, \
    _Scheduler_EDF_Update, \
    _Scheduler_global_EDF_Enqueue_priority_fifo, \
    _Scheduler_global_EDF_Enqueue_priority_lifo, \
    _Scheduler_global_EDF_Extract, \
    _Scheduler_EDF_Priority_compare, \
    _Scheduler_EDF_Release_job, \
    _Scheduler_default_Tick, \
    _Scheduler_global_EDF_Start_idle \
  }

void _Scheduler_global_EDF_Initialize( void );

void _Scheduler_global_EDF_Enqueue_priority_fifo( Thread_Control *thread );

void _Scheduler_global_EDF_Enqueue_priority_lifo( Thread_Control *thread );

void _Scheduler_global_EDF_Extract( Thread_Control *thread );

void *_Scheduler_global_EDF_Allocate( Thread_Control *thread);// added Allocate to smp scheduler.

void _Scheduler_global_EDF_Yield( Thread_Control *thread );

void _Scheduler_global_EDF_Schedule( Thread_Control *thread );

void _Scheduler_global_EDF_Start_idle(
  Thread_Control *thread,
  Per_CPU_Control *cpu
);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
/* end of include file */
