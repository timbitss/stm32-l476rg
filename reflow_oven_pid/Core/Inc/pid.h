/**
 * @file pid.h
 * @author Timothy Nguyen
 * @brief PID Controller
 * @version 0.1
 * @date 2021-07-12
 * 
 *      Adapted from Philip Salmony's PID controller implementation: https://github.com/pms67/PID
 * 
 *      Integrator anti-windup method based on Bryan Douglas' PID video: https://www.youtube.com/watch?v=NVLXCwc8HzM
 * 
 */

#ifndef _PID_H_
#define _PID_H_

typedef struct {

	/* Controller gains */
	float Kp;
	float Ki;
	float Kd;

	/* Derivative low-pass filter time constant */
	float tau;

	/* Output limits */
	float out_lim_max;
	float out_lim_min;

	/* Sample time (in seconds) */
	float Ts;

	/* Controller "memory" */
	float integral;
	float derivative;
    float prev_error;			/* Required for integrator */
	float prev_measurement;		/* Required for differentiator */

	/* Controller output */
	float out;

} PID_t;

/**
 * @brief Set/update PID controller parameters and erase controller memory.
 * 
 * @param pid PID structure containing PID parameters and controller memory.
 * @param _Kp Proportional gain.
 * @param _Ki Integral gain.
 * @param _Kd Derivative gain.
 * @param _tau Low-pass filter time-constant for filtered derivative term.
 * @param _Ts Sample period in seconds.
 * @param out_max Maximum output limit.
 * @param out_min Minimum output limit.
 */
void  PID_Init(PID_t *pid, float _Kp, float _Ki, float _Kd, float _tau,
               float _Ts, float out_max, float out_min, float int_max);

/**
 * @brief Perform PID iteration.
 * 
 * @note  Setpoint and measurement arguments must have the same units.
 * @param pid PID structure containing controller parameter.
 * @param setpoint Setpoint value for current iteration.
 * @param measurement Measured value for current iteration.
 * @return float Output of PID calculation.
 */
float PID_Calculate(PID_t *pid, float setpoint, float measurement);

/**
 * @brief Clear PID memory but retain controller parameters.
 * 
 * @param pid PID structure containing PID parameters and controller memory.
 */
void PID_Reset(PID_t *pid);

#endif



