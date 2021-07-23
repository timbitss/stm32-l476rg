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
#include "cmsis_os.h"

////////////////////////////////////////////////////////////////////////////////
// Private (static) function declarations
////////////////////////////////////////////////////////////////////////////////

static void Active_event_loop(void *argument); // Common event loop thread function.

static void TimeEvent_tick(void *argument); // Simulate a timer tick.

////////////////////////////////////////////////////////////////////////////////
// Private (static) variables
////////////////////////////////////////////////////////////////////////////////

/* Unique tag for logging information */
static const char *TAG = "ACTIVE";

/* View current Time_Event instances */
static TimeEvent *time_events[10];

/* Number of time events */
static uint8_t num_time_events;

/* Software timer instance */
osTimerId_t timer_inst;

////////////////////////////////////////////////////////////////////////////////
// Public (global) functions
////////////////////////////////////////////////////////////////////////////////

mod_err_t Active_ctor(Active *const ao, EventHandler evt_handler)
{
    if (evt_handler == NULL || ao == NULL)
    {
        return MOD_ERR_ARG;
    }

    ao->evt_handler = evt_handler;
    return MOD_OK;
}

mod_err_t Active_start(Active *const ao,
                       const osThreadAttr_t *const thread_attr,
                       uint32_t msg_count,
                       const osMessageQueueAttr_t *const queue_attr)
{
    ao->thread_id = osThreadNew(Active_event_loop, (void *)ao, thread_attr);
    ao->queue_id = osMessageQueueNew(msg_count, sizeof(Event *), queue_attr);

    ASSERT(ao->thread_id != NULL && ao->queue_id != NULL);

    return MOD_OK;
}

mod_err_t Active_post(Active *const ao, Event const *const evt)
{
    /* Put pointer to event object */
    osStatus_t err = osMessageQueuePut(ao->queue_id, &evt, 0U, 0U);
    if (err != osOK)
    {
        return MOD_ERR_TIMEOUT;
    }

    return MOD_OK;
}

void TimeEvent_ctor(TimeEvent *const time_evt, Signal sig, Active *ao)
{
    /* No critical section because it is presumed that all Time_Events
     * are created *before* multitasking has started. */
    time_evt->base.sig = sig;
    time_evt->ao = ao;
    time_evt->timeout = 0U;
    time_evt->reload = 0U;

    /* Start ms software timer. */
    if(num_time_events == 0)
    {
        timer_inst = osTimerNew(TimeEvent_tick, osTimerPeriodic, NULL, NULL);
        ASSERT(osTimerStart(timer_inst, 1) == osOK);
    }

    /* Register TimeEvent instance. */
    ASSERT(num_time_events < ARRAY_SIZE(time_events));
    time_events[num_time_events++] = time_evt;
}

void TimeEvent_arm(TimeEvent *const time_evt, uint32_t timeout, uint32_t reload)
{
    osKernelLock(); // Data shared between threads and timer ISR.
    time_evt->timeout = timeout;
    time_evt->reload = reload;
    osKernelUnlock();
}

void TimeEvent_disarm(TimeEvent *const time_evt)
{
    osKernelLock(); // Data shared between threads and timer ISR.
    time_evt->timeout = 0U;
    osKernelUnlock();
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
    static const Event initEvt = {.sig = INIT_SIG};
    ao->evt_handler(ao, &initEvt);

    /* Event loop */
    while (1)
    {
        /* Get pointer to event object. */
        Event *evt;
        osStatus_t err = osMessageQueueGet(ao->queue_id, &evt, NULL, osWaitForever);
        LOGI(TAG, "Event received.");
        if (err != osOK)
        {
            LOGE(TAG, "Message queue error.");
        }

        /* Dispatch to event handler and run to completion. */
        ao->evt_handler(ao, evt);
    }
}

/**
 * @brief Simulate a timer tick.
 * 
 * If TimeEvent instance's timeout value reaches zero on decrement,
 * a user-defined timeout signal is posted to the registered active object.
 *
 * @note This function should be called from within a 1 ms timer ISR
 *       or using a 1 ms OS-specific software timer.
 */
static void TimeEvent_tick(void *argument)
{
    for (uint8_t i = 0U; i < num_time_events; ++i)
    {
        TimeEvent *const t = time_events[i];
        ASSERT(t);
        
        if (t->timeout > 0U)
        { 
            t->timeout = t->timeout - 1; // Down-counting timer.
            if (t->timeout == 0U)
            {   
                Active_post(t->ao, &(t->base));
                t->timeout = t->reload;
            }
        }
    }
}
