/**
 * @file active.h
 * @author Timothy Nguyen
 * @brief Generic Active object framework.
 * @version 0.1
 * @date 2021-07-20
 * 
 * Event and active object sizing.
 */

#include "active.h"
#include "log.h"

////////////////////////////////////////////////////////////////////////////////
// Private (static) function declarations
////////////////////////////////////////////////////////////////////////////////

static void Active_event_loop(void *argument); // Common event loop thread function.

////////////////////////////////////////////////////////////////////////////////
// Public (global) functions
////////////////////////////////////////////////////////////////////////////////

mod_err_t Active_ctor(Active * const ao, EventHandler evt_handler)
{
    if(evt_handler == NULL || ao == NULL)
    {
        return MOD_ERR_ARG;
    }

    ao->evt_handler = evt_handler;
    return MOD_OK;
}

mod_err_t Active_start(Active * const ao,
                       const osThreadAttr_t * const thread_attr,
                       uint32_t msg_count, 
                       const osMessageQueueAttr_t * const queue_attr)
{
    ao->thread_id = osThreadNew(Active_event_loop, (void *)ao, thread_attr);
    ao->queue_id = osMessageQueueNew(msg_count, sizeof(Event*), queue_attr);

    if(ao->thread_id == NULL || ao->queue_id == NULL)
    {
        return MOD_ERR;
    }
    
    return MOD_OK;
}

mod_err_t Active_post(Active * const ao, Event const * const evt)
{
	/* Put pointer to event object */
    osStatus_t err = osMessageQueuePut(ao->queue_id, &evt, 0U, 0U);
    if(err != osOK)
    {
        return MOD_ERR;
    }
    
    return MOD_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Private (static) functions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Event loop thread function for ALL active objects.
 *
 * @param argument Starting argument passed to function.
 *                 In this case, an Active *.
 */
static void Active_event_loop(void *argument)
{
    Active *ao = (Active *)argument;

    /* Initialize the AO */
    static const Event initEvt = { .sig = INIT_SIG };
    ao->evt_handler(ao, &initEvt);

    /* Event loop */
    while (1)
    {
        /* Get pointer to event object. */
        Event* evt;
        osStatus_t err = osMessageQueueGet(ao->queue_id, &evt, NULL, osWaitForever);
        if(err != osOK)
        {
            LOG("Error: Message queue read\r\n");
        }

        /* Dispatch to event handler and run to completion. */
        ao->evt_handler(ao, evt);
    }
}
