#include "header.h"
#include <string.h>

static void	fill_config(t_shared *shared, args *ins)
{
	shared->num_coders = ins->num_coders;
	shared->t_to_burnout = ins->t_to_burnout;
	shared->t_to_compile = ins->t_to_compile;
	shared->t_to_debug = ins->t_to_debug;
	shared->t_to_refactor = ins->t_to_refactor;
	shared->num_compile_req = ins->num_compile_req;
	shared->dongle_cooldown = ins->dongle_cooldown;
	if (strcmp(ins->scheduler, "fifo") == 0)
		shared->scheduler = FIFO;
	else
		shared->scheduler = EDF;
}

static int	init_shared_sync(t_shared *shared)
{
	if (pthread_mutex_init(&shared->stop_lock, NULL) != 0)
		return (1);
	if (pthread_cond_init(&shared->stop_cond, NULL) != 0)
	{
		pthread_mutex_destroy(&shared->stop_lock);
		return (1);
	}
	if (pthread_mutex_init(&shared->log_lock, NULL) != 0)
	{
		pthread_cond_destroy(&shared->stop_cond);
		pthread_mutex_destroy(&shared->stop_lock);
		return (1);
	}
	return (0);
}

static void	destroy_shared_sync(t_shared *shared)
{
	pthread_mutex_destroy(&shared->log_lock);
	pthread_cond_destroy(&shared->stop_cond);
	pthread_mutex_destroy(&shared->stop_lock);
}

int	init_shared(t_shared *shared, args *ins)
{
	fill_config(shared, ins);
	shared->stopped = 0;
	gettimeofday(&shared->start_time, NULL);
	if (init_dongles(shared) != 0)
		return (1);
	if (init_coders(shared) != 0)
	{
		destroy_dongles(shared);
		return (1);
	}
	if (init_shared_sync(shared) != 0)
	{
		destroy_coders(shared);
		destroy_dongles(shared);
		return (1);
	}
	return (0);
}

void	destroy_shared(t_shared *shared)
{
	destroy_shared_sync(shared);
	destroy_coders(shared);
	destroy_dongles(shared);
}
