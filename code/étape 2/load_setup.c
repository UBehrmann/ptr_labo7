
#include <unistd.h>
#include <evl/evl.h>
#include <evl/clock.h>
#include <evl/timer.h>

#include "load_setup.h"
#include "de1soc_utils/de1soc_io.h"
#include "commun.h"


void ts_diff(struct timespec *start, struct timespec *end, struct timespec *diff)
{
	diff->tv_sec = end->tv_sec - start->tv_sec;
	if(end->tv_nsec < start->tv_nsec) diff->tv_nsec = end->tv_nsec + 1000000000 - start->tv_nsec;
	else diff->tv_nsec = end->tv_nsec - start->tv_nsec;
}

void *load_task(void* cookie)
{
	Priv_load_args_t *priv = (Priv_load_args_t *)cookie;
    uint64_t ticks;
    struct sched_param param;
	uint32_t sw_val, prev_sw_val = 0;
	struct timespec ts_from, ts_now, ts_int;

    /* Create timer */
    struct itimerspec value;
	int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
	evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
	//Make timer start 1 sec from now (for some headroom)
    value.it_value.tv_sec += 1;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = LOAD_PERIOD_NS;

	evl_set_timer(tmfd, &value, NULL);

    // Make thread RT
    param.sched_priority = LOAD_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if(evl_attach_self("EVL load thread") < 0) return NULL;

    // Loop that reads a file with raw data
    while (*priv->running)
    {
        // Saturate CPU according to switch value
		sw_val = read_switch();
		if(sw_val > 100) sw_val = 100;
		if(prev_sw_val != sw_val)
		{
			evl_printf("CPU artificial load: %u\n", sw_val);
			prev_sw_val = sw_val;
		}
		evl_read_clock(EVL_CLOCK_MONOTONIC, &ts_from);
		do
		{
			evl_read_clock(EVL_CLOCK_MONOTONIC, &ts_now);
			ts_diff(&ts_from, &ts_now, &ts_int);
		} while (ts_int.tv_sec == 0 && ts_int.tv_nsec < (time_t)(sw_val * ONE_MS_IN_NS));
		

		oob_read(tmfd, &ticks, sizeof(ticks));
    }
    
    evl_printf("Terminating load task.\n");

    return NULL;
}