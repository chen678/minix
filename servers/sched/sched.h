/* This is the master header for the Scheduler.  It includes some other files
 * and defines the principal constants.
 */
#define _SYSTEM		1	/* tell headers that this is the kernel */

/* The following are so basic, all the *.c files get them automatically. */
#include <minix/config.h>	/* MUST be first */
#include <sys/types.h>
#include <minix/const.h>

#include <minix/syslib.h>
#include <minix/sysutil.h>
#include <minix/timers.h>

#include <errno.h>

#include "proto.h"

extern struct machine machine;		/* machine info */

//#define _DEBUG_577
#ifdef _DEBUG_577
#define debug_print(f_, ...) do{printf((f_), __VA_ARGS__);}while(0)
#else
#define debug_print(f_, ...) do{}while(0)
#endif // _DEBUG577
