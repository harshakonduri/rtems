/*
 *  COPYRIGHT (c) 1989-2011.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define CONFIGURE_INIT
#include "system.h"

#include <inttypes.h>
rtems_task Init(rtems_task_argument arg)
{
  rtems_name        name;
  rtems_id          period;
  rtems_status_code status;

  name = rtems_build_name( 'P', 'E', 'R', 'D' );

  status = rtems_rate_monotonic_create( name, &period );
  if ( status != RTEMS_SUCCESSFUL ) {
       printf( "rtems_monotonic_create failed with status of %d.\n", status );
    exit( 1 );
  }

  while ( 1 ) {
    if ( rtems_rate_monotonic_period( period, 100 ) == RTEMS_TIMEOUT )
      break; 
  }

  status = rtems_rate_monotonic_delete( period );   /* missed period so delete period and SELF */
  if ( status != RTEMS_SUCCESSFUL ) {
    printf( "rtems_rate_monotonic_delete failed with status of %d.\n", status );
    exit( 1 );
  }

  status = rtems_task_delete( RTEMS_SELF );    
  printf( "rtems_task_delete returned with status of %d.\n", status );
  exit( 1 );
}
