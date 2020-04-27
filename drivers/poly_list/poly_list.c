#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include "proto.h"

/*
 * Function prototypes for the poly_list driver.
 */
static int poly_list_open(message *m);
static int poly_list_close(message *m);
static struct device * poly_list_prepare(dev_t device);
static int data_update(endpoint_t endpt, int opcode, u64_t position,
    iovec_t *iov, unsigned int nr_req, endpoint_t user_endpt, unsigned int
    flags);

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int);
static int lu_state_restore(void);
EXTERN int sef_cb_lu_prepare(int state);
EXTERN int sef_cb_lu_state_isvalid(int state);
EXTERN void sef_cb_lu_state_dump(int state);
EXTERN bool dequeIsEmpty;

/* Entry points to the hello driver. */
static struct chardriver hello_tab =
{
    poly_list_open,
    poly_list_close,
    nop_ioctl,
    poly_list_prepare,
    data_update,
    nop_cleanup,
    nop_alarm,
    nop_cancel,
    nop_select,
    NULL
};

/** Represents the /dev/hello device. */
static struct device hello_device;

/** State variable to count the number of times the device has been opened. */
static int open_counter;

/** Buffer related variable **/
static Deque *deque;
static bool useQueue = TRUE;


static int poly_list_open(message *UNUSED(m))
{
    printf("poly_list_open(). Called %d time(s).\n", ++open_counter);
    return OK;
}

static int poly_list_close(message *UNUSED(m))
{
    printf("poly_list_close()\n");
    return OK;
}

static struct device * poly_list_prepare(dev_t UNUSED(dev))
{
    hello_device.dv_base = make64(0, 0);
    hello_device.dv_size = make64(strlen(HELLO_MESSAGE), 0);
    return &hello_device;
}

static int data_update(endpoint_t endpt, int opcode, u64_t position,
    iovec_t *iov, unsigned nr_req, endpoint_t UNUSED(user_endpt),
    unsigned int UNUSED(flags))
{
    int bytes, ret;
    int item;
    char readBuffer[WRITE_SIZE];

    printf("data_update()\n");

    if (nr_req != 1)
    {
        /* This should never trigger for character drivers at the moment. */
        printf("HELLO: vectored transfer request, using first element only\n");
    }

    bytes = strlen(HELLO_MESSAGE) - ex64lo(position) < iov->iov_size ?
            strlen(HELLO_MESSAGE) - ex64lo(position) : iov->iov_size;

    if (bytes <= 0)
    {
        return OK;
    }
    switch (opcode)
    {
        case DEV_GATHER_S:
            ret = sys_safecopyto(endpt, (cp_grant_id_t) iov->iov_addr, 0,
                                (vir_bytes) (HELLO_MESSAGE + ex64lo(position)),
                                 bytes);
            iov->iov_size -= bytes;
            break;

        case DEV_SCATTER_S:
            ret = sys_safecopyfrom(endpt, (cp_grant_id_t) iov->iov_addr, 0, 
                                (vir_bytes) readBuffer, WRITE_SIZE);
            readBuffer[WRITE_SIZE-1]='\0';
            bool isSuccess;
            if(readBuffer[0] == 'E'){
                char item_value[WRITE_SIZE-2];
                memcpy(item_value, &readBuffer[2], (WRITE_SIZE-2)*sizeof(char));
                item = atoi(item_value);
                printf("item_value: String value = %s, Int value = %d\n", item_value, item);
                
                if(useQueue){
                    isSuccess = insertLast(deque, item);
                } else{
                    isSuccess = insertFront(deque, item);
                }
            }
            else if(readBuffer[0] == 'D'){
                isSuccess = deleteFront(deque);
            }
            if(!isSuccess){
                printf("Fail!\n");
            }
            printDeque(deque);

            break;

        default:
            return EINVAL;
    }
    return ret;
}

static int sef_cb_lu_state_save(int UNUSED(state)) {
/* Save the state. */
    ds_publish_u32("open_counter", open_counter, DSF_OVERWRITE);

    return OK;
}

static int lu_state_restore() {
/* Restore the state. */
    u32_t value;

    ds_retrieve_u32("open_counter", &value);
    ds_delete_u32("open_counter");
    open_counter = (int) value;

    return OK;
}

static void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid);
    sef_setcb_lu_state_dump(sef_cb_lu_state_dump);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
/* Initialize the hello driver. */
    int do_announce_driver = TRUE;

    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            printf("%s", HELLO_MESSAGE);
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            useQueue = (!useQueue);
            // do_announce_driver = FALSE;
            printf("Live update.\n");
        break;

        case SEF_INIT_RESTART:
            printf("Hey, I've just been restarted!\n");
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        chardriver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

int main(void)
{
    /*
     * Perform initialization.
     */
    sef_local_startup();

    deque = initDeque();
   
    /*
     * Run the main loop.
     */
    chardriver_task(&hello_tab, CHARDRIVER_SYNC);
    return OK;
}

