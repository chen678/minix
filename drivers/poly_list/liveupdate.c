#include <minix/drivers.h>
#include "proto.h"

/* State management variables. */
EXTERN bool dequeIsEmpty;

/* Custom states definition. */
#define POLY_LIST_ENQUEUE_ON_THE_OTHER_END    (SEF_LU_STATE_CUSTOM_BASE + 0)
#define POLY_LIST_STATE_IS_CUSTOM(s)   ((s) == POLY_LIST_ENQUEUE_ON_THE_OTHER_END)

/*===========================================================================*
 *       			 sef_cb_lu_prepare 	 	             *
 *===========================================================================*/
int sef_cb_lu_prepare(int state)
{
  int is_ready;

  /* Check if we are ready for the target state. */
  is_ready = FALSE;
  switch(state) {
      /* Standard states. */
      case SEF_LU_STATE_REQUEST_FREE:
          is_ready = TRUE;
      break;

      case SEF_LU_STATE_PROTOCOL_FREE:
          is_ready = TRUE;
      break;

      /* Custom states. */
      case POLY_LIST_ENQUEUE_ON_THE_OTHER_END:
          is_ready = dequeIsEmpty;
      break;
  }

  /* Tell SEF if we are ready. */
  return is_ready ? OK : ENOTREADY;
}

/*===========================================================================*
 *      		  sef_cb_lu_state_isvalid		             *
 *===========================================================================*/
int sef_cb_lu_state_isvalid(int state)
{
  return SEF_LU_STATE_IS_STANDARD(state) || POLY_LIST_STATE_IS_CUSTOM(state);
}

/*===========================================================================*
 *      		   sef_cb_lu_state_dump         	             *
 *===========================================================================*/
void sef_cb_lu_state_dump(int state)
{
  sef_lu_dprint("poly list: live update state = %d\n", state);

  sef_lu_dprint("poly list: SEF_LU_STATE_WORK_FREE(%d) reached = %d\n", 
      SEF_LU_STATE_WORK_FREE, TRUE);
  sef_lu_dprint("poly list: SEF_LU_STATE_REQUEST_FREE(%d) reached = %d\n", 
      SEF_LU_STATE_REQUEST_FREE, TRUE);
  sef_lu_dprint("poly list: SEF_LU_STATE_PROTOCOL_FREE(%d) reached = %d\n", 
      SEF_LU_STATE_PROTOCOL_FREE, TRUE);
  sef_lu_dprint("poly list: POLY_LIST_ENQUEUE_ON_THE_OTHER_END(%d) reached = %d\n", 
      POLY_LIST_ENQUEUE_ON_THE_OTHER_END, dequeIsEmpty);
}
