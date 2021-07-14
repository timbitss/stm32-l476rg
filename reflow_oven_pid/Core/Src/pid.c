/**
 * @file pid.c
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

#include "PID.h"

#define SAMESIGN(X, Y) ((X) <= 0) == ((Y) <= 0)

void PID_Init(PID_t *pid, float _Kp, float _Ki, float _Kd, float _tau,
               float _Ts, float out_max, float out_min, float int_max)
{

	/* Clear controller memory */
	pid->integral = 0.0f;
	pid->prev_error = 0.0f;
	pid->derivative = 0.0f;
	pid->prev_measurement = 0.0f;
	pid->out = 0.0f;

    /* Store controller parameters */
    pid->Kp = _Kp;
    pid->Ki = _Ki;
    pid->Kd = _Kd;
    pid->tau = _tau;
    pid->Ts = _Ts;
    pid->out_lim_max = out_max;
    pid->out_lim_min = out_min;
}

float PID_Calculate(PID_t *pid, float setpoint, float measurement)
{
    /* Compute error */
    float error = setpoint - measurement; 

    /* Compute proportional term */
    float proportional = pid->Kp * error;

    /* Compute integral term */
    if ((pid->out == pid->out_lim_max || pid->out == pid->out_lim_min) && SAMESIGN(pid->out, error)) 
    {
        pid->integral = pid->integral; /* Clamp integral term to avoid wind-up. */
    }
    else
    {
    	pid->integral = pid->integral + 0.5f * pid->Ki * pid->Ts * (error + pid->prev_error);
    }

	/* Compute filtered derivative term. 
     * Note: Taking derivative on measurement only. */
    pid->derivative = -(2.0f * pid->Kd * (measurement - pid->prev_measurement) 	
                        + (2.0f * pid->tau - pid->Ts) * pid->derivative)
                        / (2.0f * pid->tau + pid->Ts);

	/* Compute output */
    pid->out = proportional + pid->integral + pid->derivative;

    /* Floor output */
    if (pid->out > pid->out_lim_max) 
    {
        pid->out = pid->out_lim_max;
    }
    else if (pid->out < pid->out_lim_min)
    {
        pid->out = pid->out_lim_min;
    }

	/* Store error and measurement for next PID calculation. */
    pid->prev_error       = error;
    pid->prev_measurement = measurement;

	/* Return controller output */
    return pid->out;
}

void PID_Reset(PID_t *pid)
{
    pid->integral = 0.0f;
	pid->prev_error = 0.0f;
	pid->derivative = 0.0f;
	pid->prev_measurement = 0.0f;
	pid->out = 0.0f;
}
