#include "header.h"

long	timeval_to_ms(struct timeval *tv)
{
	return (tv->tv_sec * 1000L + tv->tv_usec / 1000);
}

struct timespec	cooldown_deadline(t_dongle *d, long cooldown_ms)
{
	struct timespec	ts;
	long			total_nsec;

	ts.tv_sec = d->release_time.tv_sec + cooldown_ms / 1000;
	total_nsec = d->release_time.tv_usec * 1000L + (cooldown_ms % 1000)
		* 1000000L;
	ts.tv_nsec = total_nsec % 1000000000L;
	ts.tv_sec += total_nsec / 1000000000L;
	return (ts);
}

void	refresh_dongle_state(t_dongle *d, long cooldown_ms)
{
	struct timespec	now;
	struct timespec	deadline;

	if (d->state != D_COOLDOWN)
		return ;
	clock_gettime(CLOCK_REALTIME, &now);
	deadline = cooldown_deadline(d, cooldown_ms);
	if (now.tv_sec > deadline.tv_sec || (now.tv_sec == deadline.tv_sec
			&& now.tv_nsec >= deadline.tv_nsec))
		d->state = D_FREE;
}
