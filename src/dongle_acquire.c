#include "header.h"

static void	order_by_id(t_coder *coder, t_dongle **first, t_dongle **second)
{
	if (coder->left->id < coder->right->id)
	{
		*first = coder->left;
		*second = coder->right;
	}
	else
	{
		*first = coder->right;
		*second = coder->left;
	}
}

static long	snapshot_priority_key(t_coder *coder)
{
	struct timeval	now;
	long			key;

	if (coder->shared->scheduler == FIFO)
	{
		gettimeofday(&now, NULL);
		return (now.tv_sec * 1000000L + now.tv_usec);
	}
	pthread_mutex_lock(&coder->state_lock);
	key = timeval_to_ms(&coder->last_compile_start) + coder->shared->t_to_burnout;
	pthread_mutex_unlock(&coder->state_lock);
	return (key);
}

static void	wait_once(t_dongle *d, t_coder *coder)
{
	struct timespec	deadline;

	if (d->state == D_COOLDOWN)
	{
		deadline = cooldown_deadline(d, coder->shared->dongle_cooldown);
		pthread_cond_timedwait(&d->cond, &d->lock, &deadline);
	}
	else
		pthread_cond_wait(&d->cond, &d->lock);
	refresh_dongle_state(d, coder->shared->dongle_cooldown);
}

/* 0 = 取得できた / 1 = 停止したので諦めた */
static int	acquire_one(t_dongle *d, t_coder *coder, long key)
{
	pthread_mutex_lock(&d->lock);
	heap_push(&d->waiters, coder->id, key);
	refresh_dongle_state(d, coder->shared->dongle_cooldown);
	while (!(d->state == D_FREE && heap_top(&d->waiters) == coder->id))
	{
		if (is_stopped(coder->shared))
		{
			heap_remove(&d->waiters, coder->id);
			pthread_mutex_unlock(&d->lock);
			return (1);
		}
		wait_once(d, coder);
	}
	d->state = D_TAKEN;
	heap_pop(&d->waiters);
	pthread_mutex_unlock(&d->lock);
	log_state(coder->shared, coder->id, "has taken a dongle");
	return (0);
}

int	acquire_two_dongles(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;
	long		key;

	order_by_id(coder, &first, &second);
	key = snapshot_priority_key(coder);
	if (acquire_one(first, coder, key) != 0)
		return (1);
	if (second != first && acquire_one(second, coder, key) != 0)
	{
		release_one_dongle(first);
		return (1);
	}
	return (0);
}
