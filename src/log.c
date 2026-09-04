#include "header.h"
#include <stdio.h>

long	elapsed_ms(t_shared *shared)
{
	struct timeval	now;
	long			sec_diff;
	long			usec_diff;

	gettimeofday(&now, NULL);
	sec_diff = now.tv_sec - shared->start_time.tv_sec;
	usec_diff = now.tv_usec - shared->start_time.tv_usec;
	return (sec_diff * 1000 + usec_diff / 1000);
}

void	log_state(t_shared *shared, int coder_id, const char *msg)
{
	long	ts;

	ts = elapsed_ms(shared);
	pthread_mutex_lock(&shared->log_lock);
	printf("%ld %d %s\n", ts, coder_id, msg);
	pthread_mutex_unlock(&shared->log_lock);
}
