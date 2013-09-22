/**
 * @file
 *
 * @brief Global EDF Scheduler Implementation
 *
 * @ingroup ScoreSchedulerGlobalEDF
 */

/*
 * Copyright (c) 2013 embedded brains GmbH.
 *
 * Copyright (c) Sree Harsha Konduri
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.com/license/LICENSE.
 */

#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <rtems/score/schedulerglobaledf.h>
#include <rtems/score/schedulersimple.h>
#include <rtems/score/schedulerpriorityimpl.h>
#include <rtems/score/schedulersmpimpl.h>
#include <rtems/score/rbtree.h>
#include <rtems/score/rbtreeimpl.h>
#include <rtems/score/scheduleredfimpl.h>
#include <rtems/score/wkspace.h>
static Scheduler_global_EDF_Control *_Scheduler_global_EDF_Instance( void )
{
  return _Scheduler.information;
}

static int _Scheduler_EDF_RBTree_compare_function
(
  const RBTree_Node* n1,
  const RBTree_Node* n2
)
{
  Priority_Control value1 = _RBTree_Container_of
    (n1,Scheduler_global_EDF_perthread,Node)->thread->current_priority;
  Priority_Control value2 = _RBTree_Container_of
    (n2,Scheduler_global_EDF_perthread,Node)->thread->current_priority;
  /*
   * This function compares only numbers for the red-black tree,
   * but priorities have an opposite sense.
   */
  return (-1)*_Scheduler_Priority_compare(value1, value2);
}


void _Scheduler_global_EDF_Initialize( void )
{
  Scheduler_global_EDF_Control *self =  _Workspace_Allocate_or_fatal_error( sizeof( *self ));

  _RBTree_Initialize_empty( &self->ready,
			    &_Scheduler_EDF_RBTree_compare_function,
			    0
			  );
  _Chain_Initialize_empty( &self->scheduled );
  _SMP_lock_Initialize(&self->smp_lock_ready_queue);
  _Scheduler.information = self;
}

static void _Scheduler_global_EDF_Allocate_processor(
  Thread_Control *scheduled,
  Thread_Control *victim
)
{
  Per_CPU_Control *cpu_of_scheduled = scheduled->cpu;
  Per_CPU_Control *cpu_of_victim = victim->cpu;
  Thread_Control *heir;

  scheduled->is_scheduled = true;
  victim->is_scheduled = false;

   _Per_CPU_Acquire( cpu_of_scheduled );

  if ( scheduled->is_executing ) {
    heir = cpu_of_scheduled->heir;
    cpu_of_scheduled->heir = scheduled;
  } else {
    heir = scheduled;
  }

   _Per_CPU_Release( cpu_of_scheduled );

  if ( heir != victim ) {
      const Per_CPU_Control *cpu_of_executing = _Per_CPU_Get();
    heir->cpu = cpu_of_victim;
    cpu_of_victim->heir = heir;
    cpu_of_victim->dispatch_necessary = true;
    if ( cpu_of_victim != cpu_of_executing ) {
      _Per_CPU_Send_interrupt( cpu_of_victim );
    }
  }
}

static Thread_Control *_Scheduler_global_EDF_Get_lowest_scheduled(
  Scheduler_global_EDF_Control *self
)
{
  Thread_Control *lowest_ready = NULL;
  Chain_Control *scheduled = &self->scheduled;

  if ( !_Chain_Is_empty( scheduled ) ) {
    lowest_ready = (Thread_Control *) _Chain_Last( scheduled );
  }

  return lowest_ready;
}

static Thread_Control *_Scheduler_global_EDF_Get_highest_ready(
  Scheduler_global_EDF_Control *self
)
{
  Thread_Control *highest_ready = NULL;

  _SMP_lock_Acquire(&self->smp_lock_ready_queue);
  if ( !_RBTree_Is_null(&self->ready) ) {

    RBTree_Node *first = _RBTree_First(&self->ready, RBT_LEFT);
    Scheduler_global_EDF_perthread *sched_info =  _RBTree_Container_of(first,
								      Scheduler_global_EDF_perthread, Node);

    highest_ready = sched_info->thread;
  }
  _SMP_lock_Release(&self->smp_lock_ready_queue);
  return highest_ready;
}

static void _Scheduler_global_EDF_Move_from_scheduled_to_ready(
  Scheduler_global_EDF_Control *self, 
  RBTree_Control *ready_chain,
  Thread_Control *scheduled_to_ready
)
{
 Scheduler_global_EDF_perthread *sched_info =
    (Scheduler_global_EDF_perthread*) scheduled_to_ready->scheduler_info;
   RBTree_Node *node = &(sched_info->Node); 
 _Chain_Extract_unprotected( &scheduled_to_ready->Object.Node );
  _SMP_lock_Acquire(&self->smp_lock_ready_queue);

 _RBTree_Insert( ready_chain, node );
  sched_info->queue_state = SCHEDULER_EDF_QUEUE_STATE_YES;
  _SMP_lock_Release(&self->smp_lock_ready_queue);
 }

static void _Scheduler_global_EDF_Move_from_ready_to_scheduled(
  Scheduler_global_EDF_Control *self,
  Chain_Control *scheduled_chain,
  Thread_Control *ready_to_scheduled
)
{
    Scheduler_global_EDF_perthread *sched_info =
    (Scheduler_global_EDF_perthread*) ready_to_scheduled->scheduler_info;
  _SMP_lock_Acquire(&self->smp_lock_ready_queue);
   RBTree_Node *node = &(sched_info->Node);
   _RBTree_Extract_unprotected(&self->ready, node );
   sched_info->queue_state = SCHEDULER_EDF_QUEUE_STATE_NOT_PRESENTLY;
  _SMP_lock_Release(&self->smp_lock_ready_queue);
   _Scheduler_simple_Insert_priority_fifo( scheduled_chain, ready_to_scheduled );
}

static void _Scheduler_global_EDF_Insert(
  RBTree_Control *chain,
  Thread_Control *thread,
  RBTree_Node *node
)
{
  Scheduler_global_EDF_Control *self = _Scheduler_global_EDF_Instance();
   Scheduler_global_EDF_perthread *sched_info =
    (Scheduler_global_EDF_perthread*) thread->scheduler_info;
   sched_info->thread_location = THREAD_IN_READY_QUEUE;
  _SMP_lock_Acquire(&self->smp_lock_ready_queue);
   _RBTree_Insert( chain, node);
  sched_info->queue_state = SCHEDULER_EDF_QUEUE_STATE_YES;
  _SMP_lock_Release(&self->smp_lock_ready_queue);
}

static void _Scheduler_global_EDF_ChainInsert(
  Chain_Control *chain,
  Thread_Control *thread,
  Chain_Node_order order
)
{
 Scheduler_global_EDF_perthread *sched_info =
    (Scheduler_global_EDF_perthread*) thread->scheduler_info;

  sched_info->thread_location = THREAD_IN_SCHEDULED_QUEUE;
  _Chain_Insert_ordered_unprotected( chain, &thread->Object.Node, order );
}



static void _Scheduler_global_EDF_Enqueue_ordered(
  Scheduler_global_EDF_Control *self,
  Thread_Control *thread,
  Chain_Node_order order,
  RBTree_Node *node
)
{

  /*
   * The scheduled chain has exactly processor count nodes after
   * initialization, thus the lowest priority scheduled thread exists.
   */
  Scheduler_global_EDF_perthread *sched_info =
   (Scheduler_global_EDF_perthread*) thread->scheduler_info;
  RBTree_Node *node_thread = &(sched_info->Node);
  if(thread->is_in_the_air) {
    Thread_Control *highest_ready = _Scheduler_global_EDF_Get_highest_ready(self);
    thread->is_in_the_air = false;
    if (
      highest_ready != NULL
      && (_Scheduler_EDF_RBTree_compare_function(node_thread,node))
    ) {
      _Scheduler_SMP_Allocate_processor( highest_ready, thread );

       _Scheduler_global_EDF_Insert(&self->ready, thread, node );
      _Scheduler_global_EDF_Move_from_ready_to_scheduled( self,&self->scheduled, highest_ready );
    } else {
      thread->is_scheduled = true;

      _Scheduler_global_EDF_ChainInsert(&self->scheduled, thread, order );
    }
  } else {
    Thread_Control *lowest_scheduled =  _Scheduler_global_EDF_Get_lowest_scheduled(self);

     if ( ( *order )( &thread->Object.Node, &lowest_scheduled->Object.Node ) ) {
    _Scheduler_global_EDF_Allocate_processor( thread, lowest_scheduled );

    _Scheduler_global_EDF_ChainInsert( &self->scheduled, thread, order );

    _Scheduler_global_EDF_Move_from_scheduled_to_ready(
       self,
      &self->ready,
      lowest_scheduled
    );
     } else {
      _Scheduler_global_EDF_Insert( &self->ready, thread, node);
      }
  }
}

void *_Scheduler_global_EDF_Allocate( Thread_Control *the_thread)
{
  void *sched;
  Scheduler_global_EDF_perthread *schinfo;

  sched = _Workspace_Allocate( sizeof(Scheduler_global_EDF_perthread) );

  if ( sched ) {
    the_thread->scheduler_info = sched;
    schinfo = (Scheduler_global_EDF_perthread *)(the_thread->scheduler_info);
    schinfo->thread = the_thread;
    schinfo->queue_state = SCHEDULER_EDF_QUEUE_STATE_NEVER_HAS_BEEN;
  }

  return sched;
}

void _Scheduler_global_EDF_Enqueue_priority_lifo( Thread_Control *thread )
{
  _Scheduler_global_EDF_Enqueue_priority_fifo(thread);
}

void _Scheduler_global_EDF_Enqueue_priority_fifo( Thread_Control *thread )
{
  Scheduler_global_EDF_Control *self = _Scheduler_global_EDF_Instance();
 Scheduler_global_EDF_perthread *sched_info =
    (Scheduler_global_EDF_perthread*) thread->scheduler_info;
  RBTree_Node *node = &(sched_info->Node);

  _Scheduler_global_EDF_Enqueue_ordered(self,//Global EDF Control
				       thread,//Thread
				       _Scheduler_simple_Insert_priority_fifo_order,//Chain Order
				       node // RB Tree Node
				       );
}

void _Scheduler_global_EDF_Extract( Thread_Control *thread )
{
  Scheduler_global_EDF_Control *self = _Scheduler_global_EDF_Instance();

  _Chain_Extract_unprotected( &thread->Object.Node );
  _SMP_lock_Acquire(&self->smp_lock_ready_queue);

  if ( thread->is_scheduled ) {
    RBTree_Node *first = _RBTree_First(&self->ready, RBT_LEFT);
    Scheduler_global_EDF_perthread *sched_info =  _RBTree_Container_of(first, Scheduler_global_EDF_perthread, Node);

  _SMP_lock_Release(&self->smp_lock_ready_queue);
    Thread_Control *highest_ready = sched_info->thread;

    _Scheduler_global_EDF_Allocate_processor( highest_ready, thread );

    _Scheduler_global_EDF_Move_from_ready_to_scheduled(
	self,
      	&self->scheduled,
      	highest_ready
    );
  }
}

void _Scheduler_global_EDF_Yield( Thread_Control *thread )
{
  ISR_Level level;

  _ISR_Disable( level );

  _Scheduler_global_EDF_Extract( thread );
  _Scheduler_global_EDF_Enqueue_priority_fifo( thread );

  _ISR_Enable( level );
}
static void  _Scheduler_global_EDF_Schedule_highest_ready(
Scheduler_global_EDF_Control *self,
Thread_Control *victim
)
{
 Thread_Control *highest_ready =  _Scheduler_global_EDF_Get_highest_ready(self);

 _Scheduler_global_EDF_Allocate_processor(highest_ready, victim);
 _Scheduler_global_EDF_Move_from_ready_to_scheduled(
  self,
  &self->scheduled,
  highest_ready);
}
static void _Scheduler_global_EDF_helper_Schedule(
  Scheduler_global_EDF_Control *self,
  Thread_Control *thread
)
{
  /*  if ( thread->is_in_the_air ) {
    thread->is_in_the_air = false;

    _Scheduler_global_EDF_Schedule_highest_ready(
      self,
      thread
    );
    }*/

   _Scheduler_global_EDF_Schedule_highest_ready(
      self,
      thread
    );
}

void _Scheduler_global_EDF_Schedule( Thread_Control *thread )
{
  Scheduler_global_EDF_Control *self = _Scheduler_global_EDF_Instance();
  _Scheduler_global_EDF_helper_Schedule(self,
				thread
				);
}

void _Scheduler_global_EDF_Start_idle(
  Thread_Control *thread,
  Per_CPU_Control *cpu
)
{
  Scheduler_global_EDF_Control *self = _Scheduler_global_EDF_Instance();
  thread->is_scheduled = true;
  thread->cpu = cpu;
  _Chain_Append_unprotected( &self->scheduled, &thread->Object.Node );
}
