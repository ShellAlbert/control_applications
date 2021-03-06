#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <rtdm/rtdm.h>
#include <native/task.h>
#include <native/sem.h>
#include <native/mutex.h>
#include <native/timer.h>
#include <rtdk.h>
#include <pthread.h>

#include "ecrt.h"

#define SM_FRAME_PERIOD_NS      4000000 // 4ms.

static unsigned int cycle_ns = SM_FRAME_PERIOD_NS;

/* EtherCAT Configuration pointer */
static ec_master_t *master = NULL;

/* FIXME:  Declares per slave device. */
static ec_slave_config_t *slave0 = NULL;
static ec_slave_config_t *slave1 = NULL;
static ec_slave_config_t *slave2 = NULL;

/* FIXME: Process Data Offset in the domain. 
   Declares per slave device. */

unsigned int slave0_6041_00;
unsigned int slave0_6064_00;
unsigned int slave0_6077_00;
unsigned int slave0_6061_00;
unsigned int slave0_6040_00;
unsigned int slave0_607a_00;
unsigned int slave0_60ff_00;
unsigned int slave0_6071_00;
unsigned int slave0_6060_00;

unsigned int slave1_6041_00;
unsigned int slave1_6064_00;
unsigned int slave1_6077_00;
unsigned int slave1_6061_00;
unsigned int slave1_6040_00;
unsigned int slave1_607a_00;
unsigned int slave1_60ff_00;
unsigned int slave1_6071_00;
unsigned int slave1_6060_00;

unsigned int slave2_6041_00;
unsigned int slave2_6064_00;
unsigned int slave2_6077_00;
unsigned int slave2_6061_00;
unsigned int slave2_6040_00;
unsigned int slave2_607a_00;
unsigned int slave2_60ff_00;
unsigned int slave2_6071_00;
unsigned int slave2_6060_00;


ec_pdo_entry_info_t slave_0_pdo_entries[] = {
    {0x6040, 0x00, 16}, /* Controlword */
    {0x607a, 0x00, 32}, /* Target Position */
    {0x60ff, 0x00, 32}, /* Target Velocity */
    {0x6071, 0x00, 16}, /* Target Torque */
    {0x6060, 0x00, 8}, /* Mode of Operation */
    {0x0000, 0x00, 8}, /* None */
    {0x6041, 0x00, 16}, /* Statusword */
    {0x6064, 0x00, 32}, /* Position actual value */
    {0x6077, 0x00, 16}, /* Torque actual value */
    {0x6061, 0x00, 8}, /* Modes of operation display */
    {0x0000, 0x00, 8}, /* None */
};

ec_pdo_info_t slave_0_pdos[] = {
    {0x1600, 6, slave_0_pdo_entries + 0}, /* Receive PDO mapping */
    {0x1a00, 5, slave_0_pdo_entries + 6}, /* Transmit PDO mapping */
};

ec_sync_info_t slave_0_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, slave_0_pdos + 0, EC_WD_DISABLE},
    {3, EC_DIR_INPUT, 1, slave_0_pdos + 1, EC_WD_DISABLE},
    {0xff}
};

ec_pdo_entry_info_t slave_1_pdo_entries[] = {
    {0x6040, 0x00, 16}, /* Controlword */
    {0x607a, 0x00, 32}, /* Target Position */
    {0x60ff, 0x00, 32}, /* Target Velocity */
    {0x6071, 0x00, 16}, /* Target Torque */
    {0x6060, 0x00, 8}, /* Mode of Operation */
    {0x0000, 0x00, 8}, /* None */
    {0x6041, 0x00, 16}, /* Statusword */
    {0x6064, 0x00, 32}, /* Position actual value */
    {0x6077, 0x00, 16}, /* Torque actual value */
    {0x6061, 0x00, 8}, /* Modes of operation display */
    {0x0000, 0x00, 8}, /* None */
};

ec_pdo_info_t slave_1_pdos[] = {
    {0x1600, 6, slave_1_pdo_entries + 0}, /* Receive PDO mapping */
    {0x1a00, 5, slave_1_pdo_entries + 6}, /* Transmit PDO mapping */
};

ec_sync_info_t slave_1_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, slave_1_pdos + 0, EC_WD_DISABLE},
    {3, EC_DIR_INPUT, 1, slave_1_pdos + 1, EC_WD_DISABLE},
    {0xff}
};

ec_pdo_entry_info_t slave_2_pdo_entries[] = {
    {0x6040, 0x00, 16}, /* Controlword */
    {0x607a, 0x00, 32}, /* Target Position */
    {0x60ff, 0x00, 32}, /* Target Velocity */
    {0x6071, 0x00, 16}, /* Target Torque */
    {0x6060, 0x00, 8}, /* Mode of Operation */
    {0x0000, 0x00, 8}, /* None */
    {0x6041, 0x00, 16}, /* Statusword */
    {0x6064, 0x00, 32}, /* Position actual value */
    {0x6077, 0x00, 16}, /* Torque actual value */
    {0x6061, 0x00, 8}, /* Modes of operation display */
    {0x0000, 0x00, 8}, /* None */
};

ec_pdo_info_t slave_2_pdos[] = {
    {0x1600, 6, slave_2_pdo_entries + 0}, /* Receive PDO mapping */
    {0x1a00, 5, slave_2_pdo_entries + 6}, /* Transmit PDO mapping */
};

ec_sync_info_t slave_2_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, slave_2_pdos + 0, EC_WD_DISABLE},
    {3, EC_DIR_INPUT, 1, slave_2_pdos + 1, EC_WD_DISABLE},
    {0xff}
};

static ec_domain_t *domain1 = NULL;
uint8_t *domain1_pd = NULL;
const static ec_pdo_entry_reg_t domain1_regs[] = {
		{0, 0, 0x00000625, 0x69686555, 0x6041, 0, &slave0_6041_00},
		{0, 0, 0x00000625, 0x69686555, 0x6064, 0, &slave0_6064_00},
		{0, 0, 0x00000625, 0x69686555, 0x6077, 0, &slave0_6077_00},
		{0, 0, 0x00000625, 0x69686555, 0x6061, 0, &slave0_6061_00},
		{0, 0, 0x00000625, 0x69686555, 0x6040, 0, &slave0_6040_00},
		{0, 0, 0x00000625, 0x69686555, 0x607a, 0, &slave0_607a_00},
		{0, 0, 0x00000625, 0x69686555, 0x60ff, 0, &slave0_60ff_00},
		{0, 0, 0x00000625, 0x69686555, 0x6071, 0, &slave0_6071_00},
		{0, 0, 0x00000625, 0x69686555, 0x6060, 0, &slave0_6060_00},
		
		{0, 1, 0x00000625, 0x69686555, 0x6041, 0, &slave1_6041_00},
		{0, 1, 0x00000625, 0x69686555, 0x6064, 0, &slave1_6064_00},
		{0, 1, 0x00000625, 0x69686555, 0x6077, 0, &slave1_6077_00},
		{0, 1, 0x00000625, 0x69686555, 0x6061, 0, &slave1_6061_00},
		{0, 1, 0x00000625, 0x69686555, 0x6040, 0, &slave1_6040_00},
		{0, 1, 0x00000625, 0x69686555, 0x607a, 0, &slave1_607a_00},
		{0, 1, 0x00000625, 0x69686555, 0x60ff, 0, &slave1_60ff_00},
		{0, 1, 0x00000625, 0x69686555, 0x6071, 0, &slave1_6071_00},
		{0, 1, 0x00000625, 0x69686555, 0x6060, 0, &slave1_6060_00},

		{0, 2, 0x00000625, 0x69686555, 0x6041, 0, &slave2_6041_00},
		{0, 2, 0x00000625, 0x69686555, 0x6064, 0, &slave2_6064_00},
		{0, 2, 0x00000625, 0x69686555, 0x6077, 0, &slave2_6077_00},
		{0, 2, 0x00000625, 0x69686555, 0x6061, 0, &slave2_6061_00},
		{0, 2, 0x00000625, 0x69686555, 0x6040, 0, &slave2_6040_00},
		{0, 2, 0x00000625, 0x69686555, 0x607a, 0, &slave2_607a_00},
		{0, 2, 0x00000625, 0x69686555, 0x60ff, 0, &slave2_60ff_00},
		{0, 2, 0x00000625, 0x69686555, 0x6071, 0, &slave2_6071_00},
		{0, 2, 0x00000625, 0x69686555, 0x6060, 0, &slave2_6060_00},
		{}
};

typedef enum {
	__Unknown,
	__NotReadyToSwitchOn,
	__SwitchOnDisabled,
	__ReadyToSwitchOn,
	__SwitchedOn,
	__OperationEnabled,
	__QuickStopActive,
	__FaultReactionActive,
	__Fault,
} __CIA402NodeState;

static uint16_t __InactiveMask = 0x4f;
static uint16_t __ActiveMask = 0x6f;

void __retrieve_0_0 (void);
void __retrieve_cia402 (__CIA402NodeState *state, unsigned short *sw);
void __publish_0_0 (void);
void __publish_cia402 (__CIA402NodeState *state, unsigned short *cw);

/* Distributed Clock */
#define NSEC_PER_SEC	1000000000ULL
#define SYNC0_DC_EVT_PERIOD_NS	4000000	// SYNC0 compensation cycle, 4ms.
#define SYNC0_SHIFT	150000 // SYNC0 shift interval.
#define SYNC1_DC_EVT_PERIOD_NS	0	// SYNC1 compensation cycle, 4ms.
#define SYNC1_SHIFT	0 // SYNC1 shift interval.
#define DC_FILTER_CNT	1024

static uint64_t dc_start_time_ns = 0LL;
static uint64_t dc_time_ns = 0;
static uint8_t 	dc_started = 0;
static int32_t	dc_diff_ns = 0;
static int32_t	prev_dc_diff_ns = 0;
static int64_t	dc_diff_total_ns = 0LL;
static int64_t	dc_delta_total_ns = 0LL;
static int		dc_filter_idx = 0;
static int64_t	dc_adjust_ns = 0;
static int64_t	sys_time_base = 0LL;
static uint64_t	dc_first_app_time = 0LL;
unsigned long long 	frame_period_ns = 0LL;
uint64_t wakeup_time = 0LL;

void dc_init (void);
uint64_t sys_time_ns (void);
uint64_t cal_1st_sleep_time (void);
RTIME sys_time_2_cnt (uint64_t time);
RTIME cal_sleep_time (uint64_t wakeup_time);
void sync_dc (void);
void update_master_clock (void);

/*
 * Return the sign of a number
 * ie -1 for -ve value, 0 for 0, +1 for +ve value
 * \ret val the sign of the value
 */
#define sign(val) \
        ({ typeof (val) _val = (val); \
        ((_val > 0) - (_val < 0)); })
/* DC here */

/* Handle exception */
static unsigned int sig_alarms = 0;
void signal_handler(int signum) {
	switch (signum) {
	case SIGALRM:
		sig_alarms++;
		break;
	}
}

int first_sent = 0;
RT_TASK my_task;
void cleanup_all(void) {
	printf("delete my_task\n");
	rt_task_delete(&my_task);
	ecrt_release_master(master);
}

int run; /* Execution flag for a cyclic task. */
void catch_signal(int sig) {
	run = 0;

	// Wait until task stops
	rt_task_join(&my_task);

	cleanup_all();
	printf("exit by signal\n");
	exit(0);
	return;
}

/* For monitoring Working Counter changes. */
ec_domain_state_t domain_state = {};
void check_domain_state (ec_domain_t *domain)
{
	ec_domain_state_t ds;

	ecrt_domain_state(domain, &ds);

	if (ds.working_counter != domain_state.working_counter)
		rt_printf("Domain: WC %u.\n", ds.working_counter);
	if (ds.wc_state != domain_state.wc_state)
		rt_printf("Domain: State %u.\n", ds.wc_state);

	domain_state = ds;
}

unsigned short statusWord0;
unsigned short controlWord0;
int actualPosition0;
int actualTorque0;
char modeOfOperationDisplay0;

unsigned short statusWord1;
unsigned short controlWord1;
int actualPosition1;
int actualTorque1;
char modeOfOperationDisplay1;

unsigned short statusWord2;
unsigned short controlWord2;
int actualPosition2;
int actualTorque2;
char modeOfOperationDisplay2;

void __retrieve_0_0 ()
{

	if (first_sent) {
		ecrt_master_receive (master);
		ecrt_domain_process (domain1);

		statusWord0 = EC_READ_U16(domain1_pd + slave0_6041_00);
		actualPosition0 = EC_READ_S32(domain1_pd + slave0_6064_00);
		actualTorque0 = EC_READ_S32(domain1_pd + slave0_6077_00);
		modeOfOperationDisplay0 = EC_READ_S8(domain1_pd + slave0_6061_00);
		
		statusWord1 = EC_READ_U16(domain1_pd + slave1_6041_00);
		actualPosition1 = EC_READ_S32(domain1_pd + slave1_6064_00);
		actualTorque1 = EC_READ_S32(domain1_pd + slave1_6077_00);
		modeOfOperationDisplay1 = EC_READ_S8(domain1_pd + slave1_6061_00);
		
		statusWord2 = EC_READ_U16(domain1_pd + slave2_6041_00);
		actualPosition2 = EC_READ_S32(domain1_pd + slave2_6064_00);
		actualTorque2 = EC_READ_S32(domain1_pd + slave2_6077_00);
		modeOfOperationDisplay2 = EC_READ_S8(domain1_pd + slave2_6061_00);
	}
}


void __retrieve_cia402 (__CIA402NodeState *state, unsigned short *sw)
{
	uint16_t statusword_inactive = *sw & __InactiveMask;
	uint16_t statusword_active = *sw & __ActiveMask;

	switch (statusword_inactive) {
		case 0x00:
			*state = __NotReadyToSwitchOn;
			break;
		case 0x40:
			*state = __SwitchOnDisabled;
			break;
		case 0x0f:
			*state = __FaultReactionActive;
			break;
		case 0x08:
			*state = __Fault;
			break;
		default:
			break;
	}
	switch (statusword_active) {
		case 0x21:
			*state = __ReadyToSwitchOn;
			break;
		case 0x23:
			*state = __SwitchedOn;
			break;
		case 0x27:
			*state = __OperationEnabled;
			break;
		case 0x07:
			*state = __QuickStopActive;
			break;
		default:
			break;
	}

	if (*state == __Unknown)
		return;
}

void __publish_cia402 (__CIA402NodeState *state, unsigned short *cw)
{
	uint8_t power = 1;

	// CiA 402 state transition computation.
	switch (*state) {
		case __SwitchOnDisabled:
			*cw = (*cw & ~0x87) | 0x06;
			break;
		case __ReadyToSwitchOn:
		case __OperationEnabled:
			if (!power) {
				*cw = (*cw & ~0x8f) | 0x07;
				break;
			}
		case __SwitchedOn:
			if (power) {
				*cw = (*cw & ~0x8f) | 0x1f;
			}
			break;
		case __Fault:
			*cw = (*cw & ~0x8f) | 0x80;
			break;
		default:
			break;
	}
}

static RTIME _last_occur=0;
static RTIME _last_publish=0;
RTIME _current_lag=0;
RTIME _max_jitter=0;

static inline RTIME max(RTIME a,RTIME b) {return a>b?a:b;}


void __publish_0_0 ()
{
	// Slave 0 RXPDOs
	EC_WRITE_U16(domain1_pd + slave0_6040_00, controlWord0);
	EC_WRITE_S32(domain1_pd + slave0_607a_00, 0x000000FF);
	EC_WRITE_S32(domain1_pd + slave0_60ff_00, 0x000FFFFF);
	EC_WRITE_S32(domain1_pd + slave0_6071_00, 0x000000FF);
	EC_WRITE_S8(domain1_pd + slave0_6060_00, 0x09);
	
	EC_WRITE_U16(domain1_pd + slave1_6040_00, controlWord1);
	EC_WRITE_S32(domain1_pd + slave1_607a_00, 0x000000FF);
	EC_WRITE_S32(domain1_pd + slave1_60ff_00, 0x000FFFFF);
	EC_WRITE_S32(domain1_pd + slave1_6071_00, 0x000000FF);
	EC_WRITE_S8(domain1_pd + slave1_6060_00, 0x09);
	
	EC_WRITE_U16(domain1_pd + slave2_6040_00, controlWord2);
	EC_WRITE_S32(domain1_pd + slave2_607a_00, 0x000000FF);
	EC_WRITE_S32(domain1_pd + slave2_60ff_00, 0x000FFFFF);
	EC_WRITE_S32(domain1_pd + slave2_6071_00, 0x000000FF);
	EC_WRITE_S8(domain1_pd + slave2_6060_00, 0x09);

	ecrt_domain_queue(domain1);
    {
        RTIME current_time = sys_time_ns();
        // Limit spining max 1/5 of common_ticktime
        RTIME maxdeadline = current_time + (cycle_ns / 5);
        RTIME deadline = _last_occur ? 
            _last_occur + cycle_ns : 
            current_time + _max_jitter; 
        if(deadline > maxdeadline) deadline = maxdeadline;
        _current_lag = deadline - current_time;
        if(_last_publish != 0){
            RTIME period = current_time - _last_publish;
            if(period > cycle_ns)
                _max_jitter = max(_max_jitter, period - cycle_ns);
            else
                _max_jitter = max(_max_jitter, cycle_ns - period);
        }
        _last_publish = current_time;
        _last_occur = current_time;
        while(current_time < deadline) {
            _last_occur = current_time; //Drift backward by default
            current_time = sys_time_ns();
        }
        if( _max_jitter * 10 < cycle_ns && _current_lag < _max_jitter){
            //Consuming security margin ?
            _last_occur = current_time; //Drift forward
        }
    }

    sync_dc ();
    ecrt_master_send(master);
    update_master_clock();
	first_sent = 1;
}

__CIA402NodeState state_0;
__CIA402NodeState state_1;
__CIA402NodeState state_2;
/* Xenomai task body */
void my_task_proc(void *arg)
{
	int ret;
	
	run = 1;

	/* Set Xenomai task execution mode */
	ret = rt_task_set_mode(0, T_CONFORMING, NULL);
	if (ret) {
		rt_printf("error while rt_task_set_mode, code %d\n",ret);
		return;
	}

	RTIME wakeup_cnt;
	wakeup_time = cal_1st_sleep_time ();
	wakeup_cnt = cal_sleep_time (wakeup_time);
	rt_task_sleep_until (wakeup_cnt);

	/* Start pdo exchange loop until user stop */
	while (run) {
	
		__retrieve_0_0();
		__retrieve_cia402(&state_0, &statusWord0);
		__retrieve_cia402(&state_1, &statusWord1);
		__retrieve_cia402(&state_2, &statusWord2);
		__publish_cia402(&state_0, &controlWord0);
		__publish_cia402(&state_1, &controlWord1);
		__publish_cia402(&state_2, &controlWord2);
		__publish_0_0();

		/* Wait until next release point */
		wakeup_time = wakeup_time + SM_FRAME_PERIOD_NS;
		wakeup_cnt = cal_sleep_time(wakeup_time);
		rt_task_sleep_until (wakeup_cnt);
	
	}
}

/*
 * Update the master time based on ref slaves time diff
 * called after the ethercat frame is sent to avoid time jitter in
 * sync_distributed_clocks()
 */
void update_master_clock(void)
{
    // calc drift (via un-normalised time diff)
    int32_t delta = dc_diff_ns - prev_dc_diff_ns;
    prev_dc_diff_ns = dc_diff_ns;

    // normalise the time diff
    dc_diff_ns = dc_diff_ns >= 0 ?
            ((dc_diff_ns + (int32_t)(frame_period_ns / 2)) %
                    (int32_t)frame_period_ns) - (frame_period_ns / 2) :
                    ((dc_diff_ns - (int32_t)(frame_period_ns / 2)) %
                            (int32_t)frame_period_ns) - (frame_period_ns / 2) ;

    // only update if primary master
    if (dc_started) {
        // add to totals
        dc_diff_total_ns += dc_diff_ns;
        dc_delta_total_ns += delta;
        dc_filter_idx++;

        if (dc_filter_idx >= DC_FILTER_CNT) {
            dc_adjust_ns += dc_delta_total_ns >= 0 ?
                    ((dc_delta_total_ns + (DC_FILTER_CNT / 2)) / DC_FILTER_CNT) :
                    ((dc_delta_total_ns - (DC_FILTER_CNT / 2)) / DC_FILTER_CNT) ;

            // and add adjustment for general diff (to pull in drift)
            dc_adjust_ns += sign(dc_diff_total_ns / DC_FILTER_CNT);

            // limit crazy numbers (0.1% of std cycle time)
            if (dc_adjust_ns < -1000) {
                dc_adjust_ns = -1000;
            }
            if (dc_adjust_ns > 1000) {
                dc_adjust_ns =  1000;
            }
            // reset
            dc_diff_total_ns = 0LL;
            dc_delta_total_ns = 0LL;
            dc_filter_idx = 0;
        }
        // add cycles adjustment to time base (including a spot adjustment)
        sys_time_base += dc_adjust_ns + sign(dc_diff_ns);
    }
    else {
        dc_started = (dc_diff_ns != 0);

        if (dc_started) {
#if 1
            // output first diff
            fprintf(stderr, "First master diff: %d\n", dc_diff_ns);
#endif
            // record the time of this initial cycle
            dc_start_time_ns = dc_time_ns;
        }
    }
}

void sync_dc (void)
{
	uint32_t ref_time = 0;
	RTIME prev_app_time = dc_time_ns;

	if (!ecrt_master_reference_clock_time (master, &ref_time)) {
		dc_diff_ns = (uint32_t) prev_app_time - ref_time;
	}

	ecrt_master_sync_slave_clocks (master);

	dc_time_ns = sys_time_ns ();
	ecrt_master_application_time (master, dc_time_ns);
}


/* Convert system time to Xenomai time in count via system_time_base.
*/
RTIME sys_time_2_cnt (uint64_t time)
{
	RTIME ret;

	if ((sys_time_base < 0) && ((uint64_t) (-sys_time_base) > time)) {
		fprintf (stderr, "%s() error: system_time base is less than \
						system time (system_time_base: %lld, time: %llu\n",
						__func__, sys_time_base, time);
		ret = time;
	}
	else {
		ret = time + sys_time_base;
	}

	return (RTIME) rt_timer_ns2ticks(ret);
}

RTIME cal_sleep_time (uint64_t wakeup_time)
{
	RTIME wakeup_cnt = sys_time_2_cnt(wakeup_time);
	RTIME current_cnt = rt_timer_read();

	if ((wakeup_cnt < current_cnt) || (wakeup_cnt > current_cnt + (50 * frame_period_ns))) {
		fprintf (stderr, "%s(): unexpected wake time!! \
			  	 wakeup count = %lld\t current count = %lld\n",
				 __func__, wakeup_cnt, current_cnt);
	}

	return wakeup_cnt;
}

/*
   Calculate the 1st sleep time to sync ref clock and master clock.
*/
uint64_t cal_1st_sleep_time (void)
{
	uint64_t dc_remainder = 0LL;
	uint64_t dc_phase_set_time = 0LL;

	dc_phase_set_time = sys_time_ns() + frame_period_ns * 10;
	dc_remainder = (dc_phase_set_time - dc_first_app_time) % frame_period_ns;

	return dc_phase_set_time + frame_period_ns - dc_remainder;
}

/* Get the time (ns) for the current CPU, 
   adjusted by system_time_base. 
   Rather than call rt_timer_read () directly, 
   Use this function instead. 
*/
uint64_t sys_time_ns(void)
{

	RTIME now = rt_timer_read();

	if (unlikely(sys_time_base > (SRTIME) now)) {
		fprintf (stderr, "%s() error: system_time base is greater than" 
						"system time (system_time_base: %lld, time: %llu\n",
						__func__, sys_time_base, now);
		return now;
	}
	else {
		return now - sys_time_base;
	}
}

void dc_init (void)
{
	/* Set DC compensation period, same as frame cycle */
	frame_period_ns = SM_FRAME_PERIOD_NS;

	/* Set initial master time */ 
	dc_start_time_ns = sys_time_ns();
	fprintf(stderr, "dc_start_time_ns: %lld\n", dc_start_time_ns);


	dc_time_ns = dc_start_time_ns;
	dc_first_app_time = dc_start_time_ns;

	ecrt_master_application_time (master, dc_start_time_ns);

}



int main(int argc,char **argv)
{
	int ret;
	uint32_t abort_code;

	/* Lock all currently mapped pages to prevent page swapping */
	mlockall(MCL_CURRENT | MCL_FUTURE);

	/* Register signal handler */
	signal(SIGTERM, catch_signal);
	signal(SIGINT, catch_signal);

	/* Request EtherCAT master */
	master = ecrt_request_master(0);
	if (!master) return -1;

	/* Create domains for PDO exchange. */
	domain1 = ecrt_master_create_domain(master);
	if (!domain1) return -1;
	
	// slaves PDO configuration
	if (!(slave0 = ecrt_master_slave_config(master, 0, 0, 0x00000625, 0x69686555))) {
		fprintf(stderr, "Failed to get slave 0 and position 0.\n");
		return -1;
	}
	if (ecrt_slave_config_pdos(slave0, EC_END, slave_0_syncs)) {
		fprintf(stderr, "Failed to configure PDOs for slave 0 and position 0.\n");
		return -1;
	}
	// slaves PDO configuration
	if (!(slave1 = ecrt_master_slave_config(master, 0, 1, 0x00000625, 0x69686555))) {
		fprintf(stderr, "Failed to get slave 1 and position 1.\n");
		return -1;
	}
	if (ecrt_slave_config_pdos(slave1, EC_END, slave_1_syncs)) {
		fprintf(stderr, "Failed to configure PDOs for slave 1 and position 1.\n");
		return -1;
	}
	// slaves PDO configuration
	if (!(slave2 = ecrt_master_slave_config(master, 0, 2, 0x00000625, 0x69686555))) {
		fprintf(stderr, "Failed to get slave 2 and position 2.\n");
		return -1;
	}
	if (ecrt_slave_config_pdos(slave1, EC_END, slave_1_syncs)) {
		fprintf(stderr, "Failed to configure PDOs for slave 2 and position 2.\n");
		return -1;
	}

	/* Setup EtherCAT Master transmit interval(us), 
	   usually same as the period of control task. */
	ecrt_master_set_send_interval(master, cycle_ns / 1000);

	/* Register pdo entry to the domains */
	if (ecrt_domain_reg_pdo_entry_list(domain1, domain1_regs)) {
		fprintf(stderr, "PDO entry registration failed!\n");
		return -1;
	}

	/* Setup Drive operation mode (CSV) via SDO.*/
	{
		/* FIXME: Use following codes when multiple drives are required. */
		uint8_t value[1];
		EC_WRITE_S8((uint8_t *)value, 0x09);
		if(ecrt_master_sdo_download(master, 0, 0x6060, 0x00, (uint8_t *)value, 1, &abort_code))
		{
			fprintf(stderr, "Failed to initialize slave 0 and position 0,\nError: %d\n", abort_code);
			return -1;
		}
	}
	{
		/* FIXME: Use following codes when multiple drives are required. */
		uint8_t value[1];
		EC_WRITE_S8((uint8_t *)value, 0x09);
		if(ecrt_master_sdo_download(master, 1, 0x6060, 0x00, (uint8_t *)value, 1, &abort_code))
		{
			fprintf(stderr, "Failed to initialize slave 1 and position 1,\nError: %d\n", abort_code);
			return -1;
		}
	}
	{
		/* FIXME: Use following codes when multiple drives are required. */
		uint8_t value[1];
		EC_WRITE_S8((uint8_t *)value, 0x09);
		if(ecrt_master_sdo_download(master, 2, 0x6060, 0x00, (uint8_t *)value, 1, &abort_code))
		{
			fprintf(stderr, "Failed to initialize slave 1 and position 1,\nError: %d\n", abort_code);
			return -1;
		}
	}


	/* Configuring DC signal */
	ecrt_slave_config_dc (slave0, 0x0300, SYNC0_DC_EVT_PERIOD_NS, SYNC0_SHIFT,
										  SYNC1_DC_EVT_PERIOD_NS, SYNC1_SHIFT);

	/* Select ref. clock */
	ecrt_master_select_reference_clock (master, slave0);

	dc_init ();

	/* Activating master stack */
	if (ecrt_master_activate(master))
		return -1;

	/* Debugging purpose, get the address of mapped domains. */
	if (!(domain1_pd = ecrt_domain_data(domain1))) return -1;
	fprintf(stdout, "Master 0 activated...\n\n");
	fprintf(stdout, "domain1_pd:  0x%.6lx\n", (unsigned long)domain1_pd);

	/* Creating cyclic xenomai task */
	ret = rt_task_create(&my_task,"my_task",0,50,T_JOINABLE);
	if (ret) {
		fprintf (stderr, "Task create failed!!!!\n");
	}

	/* Starting cyclic task */
	fprintf(stdout, "starting my_task\n");
	ret = rt_task_start(&my_task, &my_task_proc, NULL);
	if (ret) {
		fprintf (stderr, "Task start failed!!!!\n");
	}

	while (run)	{
		sched_yield();
	}

	/* Cleanup routines. */ 
	rt_task_join(&my_task);
	rt_task_delete(&my_task);
	fprintf(stdout, "End of Program\n");
	ecrt_release_master(master); /* Releases a requested EtherCAT master */

	return 0;
}
