#include "header.h"
#include <unistd.h>

static void	do_compile_phase(t_coder *coder)
{
	pthread_mutex_lock(&coder->state_lock);
	gettimeofday(&coder->last_compile_start, NULL);
	coder->compiling = 1;
	pthread_mutex_unlock(&coder->state_lock);
	log_state(coder->shared, coder->id, "is compiling");
	usleep(coder->shared->t_to_compile * 1000);
	pthread_mutex_lock(&coder->state_lock);
	coder->compiling = 0;
	coder->compile_count++;
	pthread_mutex_unlock(&coder->state_lock);
}

static void	do_debug_phase(t_coder *coder)
{
	log_state(coder->shared, coder->id, "is debugging");
	usleep(coder->shared->t_to_debug * 1000);
}

static void	do_refactor_phase(t_coder *coder)
{
	log_state(coder->shared, coder->id, "is refactoring");
	usleep(coder->shared->t_to_refactor * 1000);
}

void	*coder_thread(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	while (coder->compile_count < coder->shared->num_compile_req)
	{
		acquire_two_dongles(coder);
		do_compile_phase(coder);
		release_two_dongles(coder);
		do_debug_phase(coder);
		do_refactor_phase(coder);
	}
	return (NULL);
}
