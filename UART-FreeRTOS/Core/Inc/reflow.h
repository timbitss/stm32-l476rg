/**
 * @file reflow.h
 * @author Timothy Nguyen
 * @brief Reflow Oven Controller
 * @version 0.1
 * @date 2021-07-22
 */

#ifndef __REFLOW_H__
#define __REFLOW_H__

#include "active.h"

/* Configuration parameters */
#define REFLOW_THREAD_STACK_SZ 1024

#define KP_INIT 10.0f 		 // Kp gain.
#define KI_INIT 0.0f  		 // Ki gain.
#define KD_INIT 0.0f  		 // Kd gain.
#define TAU_INIT 1.0f 		 // Low-pass filter time constant.
#define TS_INIT  0.5f 		 // Sampling period (s).
#define OUT_MAX_INIT 4095.0f // Maximum output saturation limit.
#define OUT_MIN_INIT 0.0f    // Minimum output saturation limit.

/* Reflow controller signals. */
enum ReflowSignal
{
    START_REFLOW_SIG = USER_SIG, // Start reflow process.
    REACH_TIME_SIG,			     // Timeout event.
	REACH_TEMP_SIG,			     // Reached specific temperature.
    STOP_REFLOW_SIG,				 // Stop reflow process.

	NUM_REFLOW_SIGS
};

/**
 * @brief Initialize reflow oven controller.
 */
void reflow_init();

/**
 * @brief Start reflow oven active object instance.
 *
 * This function does not start the scheduler.
 */
void reflow_start();

#endif
