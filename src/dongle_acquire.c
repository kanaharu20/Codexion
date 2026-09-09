/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_acquire.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:20:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 16:30:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

/*
** キーの単位は fifo / edf のどちらもシミュレーション開始からのマイクロ秒。
** fifo は要求が到着した時刻、edf は burnout 期限をそのままキーにする。
** 1回の要求につき1度だけ採り、2本の dongle で同じ値を使う。こうすると
** どの dongle でも待ち行列の順序が一致する。
*/
static long	snapshot_priority_key(t_coder *coder)
{
	long	key;

	if (coder->shared->scheduler == FIFO)
		return (elapsed_us(coder->shared));
	pthread_mutex_lock(&coder->state_lock);
	key = coder->last_compile_start_us + coder->shared->t_to_burnout * 1000L;
	pthread_mutex_unlock(&coder->state_lock);
	return (key);
}

/*
** coder が1人のときは left == right で、机の上の dongle は1本しかない。
** 2本目は永久に揃わないので、1本を取ったまま停止を待って burnout する。
*/
static int	acquire_single(t_coder *coder, t_dongle *d, long key)
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
		wait_on_blocker(d, coder);
	}
	d->state = D_TAKEN;
	heap_pop(&d->waiters);
	pthread_mutex_unlock(&d->lock);
	log_state(coder->shared, coder->id, "has taken a dongle");
	wait_until_stopped(d, coder->shared);
	release_one_dongle(d);
	return (1);
}

static int	give_up(t_coder *coder, t_dongle *f, t_dongle *s, t_dongle *held)
{
	pthread_mutex_unlock(&held->lock);
	set_blocked_on(coder, NULL);
	dequeue_both(coder, f, s);
	return (1);
}

int	acquire_two_dongles(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;
	t_dongle	*blocker;
	long		key;

	order_by_id(coder, &first, &second);
	key = snapshot_priority_key(coder);
	if (second == first)
		return (acquire_single(coder, first, key));
	enqueue_both(coder, first, second, key);
	blocker = try_take_pair(coder, first, second);
	while (blocker != NULL)
	{
		if (is_stopped(coder->shared))
			return (give_up(coder, first, second, blocker));
		wait_on_blocker(blocker, coder);
		pthread_mutex_unlock(&blocker->lock);
		blocker = try_take_pair(coder, first, second);
	}
	log_state(coder->shared, coder->id, "has taken a dongle");
	log_state(coder->shared, coder->id, "has taken a dongle");
	return (0);
}
