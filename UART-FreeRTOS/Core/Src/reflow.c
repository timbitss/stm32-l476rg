/**
 * @file reflow.h
 * @author Timothy Nguyen
 * @brief Reflow Oven Controller
 * @version 0.1
 * @date 2021-07-22
 */

#include <string.h>

#include "reflow.h"
#include "pid.h"
#include "active.h"
#include "log.h"
#include "cmd.h"
#include "cmsis_os.h"

////////////////////////////////////////////////////////////////////////////////
// Common macros
////////////////////////////////////////////////////////////////////////////////

#define REFLOW_PROFILE_PHASES_CSV "PREHEAT", "SOAK", "RAMPUP", "PEAK", "COOLDOWN"

////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////

/* Reflow oven states. */
typedef enum
{
    RESET_STATE,

	/* Reflow profile phases. */
    PREHEAT_STATE,
    SOAK_STATE,
    RAMPUP_STATE,
    PEAK_STATE, 
    COOLDOWN_STATE,

	NUM_REFLOW_STATES,
	NUM_PROFILE_PHASES = NUM_REFLOW_STATES - 1
} Reflow_State;


/* Status code when returning from event handler. */
typedef enum
{
  TRAN_STATUS,    // State transition was taken.
  HANDLED_STATUS, // Event was handled but not state transition was taken.
  IGNORE_STATUS,  // Event was ignored.
  INIT_STATUS,    // Initial state transition was taken.
} Status;

/* Reflow profile phase characteristic. */
typedef struct
{
    enum
	{
	  REACHTEMP,  // Attain a specific temperature with maximum gradient.
	  REACHTIME,  // Run profile element for specified time.
	} const phase_type; // REACHTEMP or REACHTIME

    uint32_t reach_temp;
  	uint32_t reach_time;
} Reflow_Phase;

/* Reflow controller active object */
typedef struct{
	Active reflow_base; // Inherited base Active object class.

	/* Private attributes */
	TimeEvent reflow_time_evt; // Time event instance.
	Reflow_State state; // State variable.
	PID_t reflow_pid;   // Holds PID parameters.
	Reflow_Phase reflow_phases[NUM_PROFILE_PHASES]; // Reflow phase characteristics.
} Reflow_Active;

/* State machine callback function prototype */
typedef status (*ReflowAction)(Reflow_Active * const ao, Event const * const evt);

////////////////////////////////////////////////////////////////////////////////
// Private (static) function declarations
////////////////////////////////////////////////////////////////////////////////

static void reflow_evt_handler(Reflow_Active * const ao, Event const *const evt); // Event handler.

static inline void displayPIDParams(); // Display PID parameters.

static inline void displayProfileParams(); // Display reflow profile phase parameters.

static uint32_t reflow_get_params_cmd(uint32_t argc, const char **argv); // Get reflow oven controller parameters.

////////////////////////////////////////////////////////////////////////////////
// Private (static) variables 
////////////////////////////////////////////////////////////////////////////////

/* Reflow active object. */
static Reflow_Active reflow_ao = {
.reflow_phases = { {.phase_type = REACHTEMP, .reach_temp = 125},                       // Pre-heat
			       {.phase_type = REACHTIME, .reach_temp = 180, .reach_time = 120000}, // Soak
				   {.phase_type = REACHTEMP, .reach_temp = 225 },                      // Ramp-up
				   {.phase_type = REACHTIME, .reach_temp = 225, .reach_time = 5000},   // Peak
				   {.phase_type = REACHTEMP, .reach_temp = 35 }}                       // Cool-down
};

/* Unique module tag for logging information */
static const char *TAG = "REFLOW";

/* Names of reflow profile phases as null-terminated string constants. */
static const char *reflow_names[NUM_PROFILE_PHASES] = {REFLOW_PROFILE_PHASES_CSV};

/* Information about reflow commands. */
static const cmd_cmd_info reflow_cmd_infos = {
		.cmd_name = "get",
		.cb = &reflow_get_params_cmd,
		.help = "Display reflow oven parameters (pid, profile, or *)\r\nUsage: reflow get <param> "};

/* Client information for command module */
static cmd_client_info reflow_client_info = { .client_name = "reflow",         // Client name (first command line token)
		                                      .num_cmds = 1,
											  .cmds = &reflow_cmd_infos,
											  .num_u16_pms = 0,
											  .u16_pms = NULL,
											  .u16_pm_names = NULL};


/* State machine table */
static const ReflowAction Reflow_state_table[NUM_REFLOW_STATES][NUM_REFLOW_SIGS] = {
		      /* INIT_SIG,      ENTRY_SIG, EXIT_SIG, START_REFLOW_SIG, REACH_TIME_SIG, REACH_TEMP_SIG, STOP_REFLOW_SIG */
/* RESET  	*/ { Reflow_INIT,   Reflow_reset_START, Reflow_ignore,         Reflow_ignore,             Reflow_ignore },
/* PREHEAT 	*/ { Reflow_ignore, Reflow_ignore,      Reflow_ignore,         Reflow_preheat_REACHTEMP,  Reflow_STOP   },
/* SOAK 	*/ { Reflow_ignore, Reflow_ignore,      Reflow_soak_REACHTIME, Reflow_ignore,             Reflow_STOP   },
/* RAMPUP 	*/ { Reflow_ignore, Reflow_ignore,      Reflow_ignore,         Reflow_rampup_REACHTEMP,   Reflow_STOP   },
/* PEAK 	*/ { Reflow_ignore, Reflow_ignore,      Reflow_peak_REACHTIME, Reflow_ignore,             Reflow_STOP   },
/* COOLDOWN */ { Reflow_ignore, Reflow_ignore,      Reflow_ignore,         Reflow_cooldown_REACHTEMP, Reflow_STOP   }
};

////////////////////////////////////////////////////////////////////////////////
// Public (global) variables and externs  
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Public (global) functions 
////////////////////////////////////////////////////////////////////////////////

void reflow_init()
{
	Active_ctor((Active *) &reflow_ao, (EventHandler)reflow_evt_handler);

	static const PID_cfg_t reflow_pid_cfg = {.Kp = KP_INIT,
								   	   	     .Ki = KI_INIT,
											 .Kd = KD_INIT,
											 .tau = TAU_INIT,
											 .Ts = TS_INIT,
											 .out_max = OUT_MAX_INIT,
											 .out_min = OUT_MIN_INIT};
	PID_Init(&reflow_ao.reflow_pid, &reflow_pid_cfg);

	TimeEvent_ctor(&reflow_ao.reflow_time_evt, REACH_TIME_SIG, (Active *)&reflow_ao);

    cmd_register(&reflow_client_info);

	LOGI(TAG, "Initialized reflow module.");
}

void reflow_start()
{
	osThreadAttr_t reflow_thread_attr = { .stack_size = REFLOW_THREAD_STACK_SZ };
	Active_start((Active *) &reflow_ao, &reflow_thread_attr,  5, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Private (static) functions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Display PID, profile parameters, or both to user.
 */
static uint32_t reflow_get_params_cmd(uint32_t argc, const char **argv)
{
	if(argc != 1)
	{
		LOGW(TAG, "Expecting single token only.");
		return -1;
	}

	if(strcasecmp(argv[0], "*") == 0)
	{
		displayPIDParams();
		displayProfileParams();
	}
	else if(strcasecmp(argv[0], "pid") == 0)
	{
		displayPIDParams();
	}
	else if(strcasecmp(argv[0], "profile") == 0)
	{
		displayProfileParams();
	}
	else
	{
		LOGW(TAG, "Invalid argument: %s", argv[0]);
		return -1;
	}
	return 0;
}

static void reflow_evt_handler(Reflow_Active * const ao, Event const *const evt)
{
	/* Use state table to handle events */
	int prev_state = ao->state; // Store previous state for later.

	ASSERT((ao->state < NUM_REFLOW_STATES) && (evt->sig < NUM_REFLOW_SIGS));
	Status stat = Reflow_state_table[ao->state][evt->sig](ao, evt);

	/**
	 * Execute exit action of previous state
	 * and entry action of current state if
	 * state transition was taken.
	 */
	if (stat == TRAN_STATUS)
	{
		ASSERT(ao->state < NUM_REFLOW_STATES);
		Reflow_state_table[prev_state][EXIT_SIG](ao, (Event *)0);
		Reflow_state_table[ao->state][ENTRY_SIG](ao, (Event *)0);
	}
	else if (stat == INIT_STATUS)
	{
		/* Execute entry action of initial state. */
		Reflow_state_table[ao->state][ENTRY_SIG](ao, (Event *)0);
	}
}

static inline void displayPIDParams()
{
	LOG("Kp: %.2f\tKi: %.2f\tKd: %.2f\tTau: %.2f\r\n"
		"Sampling Period: %.2f s\tMax Limit: %.2f\tMin Limit: %.2f\r\n",
		 reflow_ao.reflow_pid.Kp, reflow_ao.reflow_pid.Ki, reflow_ao.reflow_pid.Kd,
		 reflow_ao.reflow_pid.tau, reflow_ao.reflow_pid.Ts,
		 reflow_ao.reflow_pid.out_lim_max, reflow_ao.reflow_pid.out_lim_min);
}

static inline void displayProfileParams()
{
	for(uint8_t i = 0; i < NUM_PROFILE_PHASES; i++)
	{
		LOG("Phase: %s\tType: %s\tReach Temp: %lu deg C\tReach Time: %lu s\r\n",
			 reflow_names[i], reflow_ao.reflow_phases[i].phase_type == REACHTEMP ? "REACHTEMP" : "REACHTIME",
			 reflow_ao.reflow_phases[i].reach_temp,
			 reflow_ao.reflow_phases[i].reach_time/1000);
	}
}
