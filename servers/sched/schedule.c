/* This file contains the scheduling policy for SCHED
 *
 * The entry points are:
 *   do_noquantum:        Called on behalf of process' that run out of quantum
 *   do_start_scheduling  Request to start scheduling a proc
 *   do_stop_scheduling   Request to stop scheduling a proc
 *   do_nice		  Request to change the nice level on a proc
 *   init_scheduling      Called from main.c to set up/prepare scheduling
 */
#include "sched.h"
#include "schedproc.h"
#include <assert.h>
#include <minix/com.h>
#include <machine/archtypes.h>
#include "kernel/proc.h" /* for queue constants */
#include <stdlib.h>

 //CS577 schedproc::priority deleted. Pass priority to kernel here
static int schedule_process(struct schedproc* rmp, unsigned flags, int priority);
static int lottery();

#ifdef _DEBUG_577
static int proc_count();
#endif // _DEBUG_577


#define SCHEDULE_CHANGE_QUANTUM	0x2
#define SCHEDULE_CHANGE_CPU	0x4

#define SCHEDULE_CHANGE_ALL	(	\
		SCHEDULE_CHANGE_QUANTUM	|	\
		SCHEDULE_CHANGE_CPU		\
		)

#define schedule_process_migrate(p)	\
	schedule_process(p, SCHEDULE_CHANGE_CPU)

#define CPU_DEAD	-1

#define cpu_is_available(c)	(cpu_proc[c] >= 0)

#define DEFAULT_USER_TIME_SLICE 200

/* processes created by RS are sysytem processes */
#define is_system_proc(p)	((p)->parent == RS_PROC_NR)

static unsigned cpu_proc[CONFIG_MAX_CPUS];

static void pick_cpu(struct schedproc* proc)
{
#ifdef CONFIG_SMP
    unsigned cpu, c;
    unsigned cpu_load = (unsigned)-1;

    if (machine.processors_count == 1) {
        proc->cpu = machine.bsp_id;
        return;
    }

    /* schedule sysytem processes only on the boot cpu */
    if (is_system_proc(proc)) {
        proc->cpu = machine.bsp_id;
        return;
    }

    /* if no other cpu available, try BSP */
    cpu = machine.bsp_id;
    for (c = 0; c < machine.processors_count; c++) {
        /* skip dead cpus */
        if (!cpu_is_available(c))
            continue;
        if (c != machine.bsp_id && cpu_load > cpu_proc[c]) {
            cpu_load = cpu_proc[c];
            cpu = c;
        }
    }
    proc->cpu = cpu;
    cpu_proc[cpu]++;
#else
    proc->cpu = 0;
#endif
}

/*===========================================================================*
 *				do_noquantum				     *
 *===========================================================================*/

int do_noquantum(message* m_ptr)
{
    register struct schedproc* rmp;
    int rv, proc_nr_n;

    if (sched_isokendpt(m_ptr->m_source, &proc_nr_n) != OK) {
        printf("SCHED: WARNING: got an invalid endpoint in OOQ msg %u.\n",
            m_ptr->m_source);
        return EBADEPT;
    }

    rmp = &schedproc[proc_nr_n];

    //Make sure it was in the lowest level of Kernel's PQ
    if ((rv = schedule_process(rmp, SCHEDULE_CHANGE_QUANTUM, MIN_USER_Q)) != OK) {
        return rv;
    }

    //CS577: Lottery here.
    lottery();

    return OK;
}

/*===========================================================================*
 *				do_stop_scheduling			     *
 *===========================================================================*/
int do_stop_scheduling(message* m_ptr)
{
    register struct schedproc* rmp;
    int proc_nr_n;

    /* check who can send you requests */
    if (!accept_message(m_ptr))
        return EPERM;

    if (sched_isokendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n) != OK) {
        printf("SCHED: WARNING: got an invalid endpoint in OOQ msg "
            "%ld\n", m_ptr->SCHEDULING_ENDPOINT);
        return EBADEPT;
    }

    rmp = &schedproc[proc_nr_n];
#ifdef CONFIG_SMP
    cpu_proc[rmp->cpu]--;
#endif
    rmp->flags = 0; /*&= ~IN_USE;*/

    return OK;
}

/*===========================================================================*
 *				do_start_scheduling			     *
 *===========================================================================*/
int do_start_scheduling(message* m_ptr)
{
    register struct schedproc* rmp;
    int rv, proc_nr_n, parent_nr_n;

    /* we can handle two kinds of messages here */
    assert(m_ptr->m_type == SCHEDULING_START ||
        m_ptr->m_type == SCHEDULING_INHERIT);

    /* check who can send you requests */
    if (!accept_message(m_ptr))
        return EPERM;

    /* Resolve endpoint to proc slot. */
    if ((rv = sched_isemtyendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n))
        != OK) {
        return rv;
    }
    rmp = &schedproc[proc_nr_n];

    /* Populate process slot */
    rmp->endpoint = m_ptr->SCHEDULING_ENDPOINT;
    rmp->parent = m_ptr->SCHEDULING_PARENT;

    //CS577 don't need max_priority anymore.
    // MAXPRIO now is PID in PM, see servers/pm/schedule.c::sched_start_user
    rmp->pm_pid = (unsigned)m_ptr->SCHEDULING_PMID;


    /* Inherit current priority and time slice from parent. Since there
     * is currently only one scheduler scheduling the whole system, this
     * value is local and we assert that the parent endpoint is valid */
    if (rmp->endpoint == rmp->parent) {
        /* We have a special case here for init, which is the first
           process scheduled, and the parent of itself. */
        rmp->time_slice = DEFAULT_USER_TIME_SLICE;
        rmp->num_lottery = USER_DEFAULT_LOTTERY_NUMBER;
        /*
         * Since kernel never changes the cpu of a process, all are
         * started on the BSP and the userspace scheduling hasn't
         * changed that yet either, we can be sure that BSP is the
         * processor where the processes run now.
         */
#ifdef CONFIG_SMP
        rmp->cpu = machine.bsp_id;
        /* FIXME set the cpu mask */
#endif
    }

    switch (m_ptr->m_type) {

    case SCHEDULING_START:
        /* We have a special case here for system processes, for which
         * quanum and priority are set explicitly rather than inherited
         * from the parent */
        rmp->time_slice = (unsigned)m_ptr->SCHEDULING_QUANTUM;
        rmp->num_lottery = USER_DEFAULT_LOTTERY_NUMBER;
        break;

    case SCHEDULING_INHERIT:
        /* Inherit current priority and time slice from parent. Since there
         * is currently only one scheduler scheduling the whole system, this
         * value is local and we assert that the parent endpoint is valid */
         //debug_print("do_start_scheduling(): PID:%d. Process Count:%d.\n", rmp->pm_pid, proc_count());
        if ((rv = sched_isokendpt(m_ptr->SCHEDULING_PARENT,
            &parent_nr_n)) != OK)
            return rv;
        //CS577: Should we inherit lottery? NO! Everyon is borned equal!
        rmp->num_lottery = USER_DEFAULT_LOTTERY_NUMBER;
        rmp->time_slice = schedproc[parent_nr_n].time_slice;
        break;

    default:
        /* not reachable */
        assert(0);
    }

    /* Take over scheduling the process. The kernel reply message populates
     * the processes current priority and its time slice */
    if ((rv = sys_schedctl(0, rmp->endpoint, 0, 0, 0)) != OK) {
        printf("Sched: Error taking over scheduling for %d, kernel said %d\n",
            rmp->endpoint, rv);
        return rv;
    }
    rmp->flags = IN_USE;

    /* Schedule the process, giving it some quantum */
    pick_cpu(rmp);
    while ((rv = schedule_process(rmp, SCHEDULE_CHANGE_ALL, USER_Q)) == EBADCPU) {
        /* don't try this CPU ever again */
        cpu_proc[rmp->cpu] = CPU_DEAD;
        pick_cpu(rmp);
    }

    if (rv != OK) {
        printf("Sched: Error while scheduling process, kernel replied %d\n",
            rv);
        return rv;
    }

    /* Mark ourselves as the new scheduler.
     * By default, processes are scheduled by the parents scheduler. In case
     * this scheduler would want to delegate scheduling to another
     * scheduler, it could do so and then write the endpoint of that
     * scheduler into SCHEDULING_SCHEDULER
     */

    m_ptr->SCHEDULING_SCHEDULER = SCHED_PROC_NR;

    return OK;
}


/*===========================================================================*
 *				CS577: do_lottery_number				     *
 *===========================================================================*/
int do_lottery_number(message* m_ptr)
{
    //Recieve message from any sender.
    int pid = m_ptr->SCHEDULING_LOTTERY_PID;
    int lottery_num = m_ptr->SCHEDULING_LOTTERY_NUMBER;
    debug_print("SCHED:do_lottery_number() is asked to assign %d lottery tickets to process %d.\n", lottery_num, pid);

    if (lottery_num < 0)
    {
        printf("WARNING: SCHED: Get negative lottery number:%d.\n", lottery_num);
        return EBADEPT;
    }

    struct schedproc* rmp = NULL;
    int proc_nr_n;

    for (proc_nr_n = 0, rmp = schedproc; proc_nr_n < NR_PROCS; proc_nr_n++, rmp++)
    {
        if (rmp->flags & IN_USE)
        {
            if (rmp->pm_pid == pid)
            {
                break;
            }
        }
    }
    if (proc_nr_n >= NR_PROCS)
    {
        printf("WARNING: SCHED: do_lottery_number() recieve invalid PID:%d.\n", pid);
        return EBADEPT;
    }
    rmp->num_lottery = lottery_num;
    return OK;
}


/*===========================================================================*
 *				do_nice					     *
 *===========================================================================*/
int do_nice(message* m_ptr)
{
    int proc_nr_n;

    /* check who can send you requests */
    if (!accept_message(m_ptr))
        return EPERM;

    if (sched_isokendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n) != OK) {
        printf("SCHED: WARNING: got an invalid endpoint in OOQ msg "
            "%ld\n", m_ptr->SCHEDULING_ENDPOINT);
        return EBADEPT;
    }

    //CS577
    //Remove nice()'s functionality to make sure we have full control.
    return OK;
}

/*===========================================================================*
 *				schedule_process			     *
 *===========================================================================*/
static int schedule_process(struct schedproc* rmp, unsigned flags, int priority)
{
    int err;
    int new_prio, new_quantum, new_cpu;

    pick_cpu(rmp);

    if (flags & SCHEDULE_CHANGE_QUANTUM)
        new_quantum = rmp->time_slice;
    else
        new_quantum = -1;

    if (flags & SCHEDULE_CHANGE_CPU)
        new_cpu = rmp->cpu;
    else
        new_cpu = -1;

    if ((err = sys_schedule(rmp->endpoint, priority,
        new_quantum, new_cpu)) != OK) {
        printf("PM: An error occurred when trying to schedule %d: %d\n",
            rmp->endpoint, err);
    }

    return err;
}

/*===========================================================================*
 *				CS 577: lottery		     *
 *===========================================================================*/
static int lottery()
{
    struct schedproc* rmp = NULL;
    int proc_nr_n;
    unsigned int lottery_sum = 0;
    int rv = 0;

    for (proc_nr_n = 0, rmp = schedproc; proc_nr_n < NR_PROCS; proc_nr_n++, rmp++)
    {
        if (rmp->flags & IN_USE)
        {
            lottery_sum += rmp->num_lottery;
        }
    }

    //LOTTERY TIME!
    int lottery = random() % lottery_sum + 1; //From 1 to lottery_sum inclusive.

    //Find the lucky one
    lottery_sum = 0;
    char hit = 0;
    for (proc_nr_n = 0, rmp = schedproc; proc_nr_n < NR_PROCS; proc_nr_n++, rmp++)
    {
        if (rmp->flags & IN_USE)
        {
            //debug_print("DEBUG: current sum:%d, process %d has %d tickets.\n", lottery_sum, rmp->pm_pid, rmp->num_lottery);
            lottery_sum += rmp->num_lottery;
            if (lottery_sum >= lottery)
            {
                rv = schedule_process(rmp, SCHEDULE_CHANGE_QUANTUM, MAX_USER_Q);
                hit = 1;
                break;
            }
        }
    }

    if (!hit)
    {
        printf("SCHED: Warning, no one wins lottery. Lottery number is:%d.\n", lottery);
    }
    else
    {
        debug_print("SCHED: lottery():\nProcess %d wins the lottery with result %d. It holds: %d tickets.Current Running Processes:%d\n", rmp->pm_pid, lottery, rmp->num_lottery, proc_count());
    }

    return rv;
}


/*===========================================================================*
 *                               CS:577 getuptime			    	     *
 *===========================================================================*/
int do_getticks(message* m_ptr)
{
    message m;
    int s;
    m.m_type = SYS_TIMES;		/* request time information */
    m.T_ENDPT = NONE;			/* ignore process times */
    s = _kernel_call(SYS_TIMES, &m);
    m_ptr->SCHEDULING_GETTICKS = m.T_BOOT_TICKS;
    return OK;
}

#ifdef _DEBUG_577
/*===========================================================================*
 *				CS 577: proc_count		     *
 *===========================================================================*/
static int proc_count()
{
    register struct schedproc* rmp = NULL;
    int proc_nr;
    int ret = 0;

    for (proc_nr = 0, rmp = schedproc; proc_nr < NR_PROCS; proc_nr++, rmp++)
    {
        if (rmp->flags & IN_USE)
        {
            ++ret;
        }
    }
    return ret;
}
#endif // _DEBUG_577