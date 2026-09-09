/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_pair.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/09 16:00:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 17:00:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

/*
** d を今この coder が取れるか。呼び出し側で d->lock を持っていること。
** 待ち行列が空、または自分が先頭なら取れる。先頭が別の coder でも、
** その coder を飛ばしてよい条件（can_be_passed_over）を満たすなら取れる。
*/
static int	can_take(t_dongle *d, t_coder *coder, long cooldown_ms)
{
	int	top;

	refresh_dongle_state(d, cooldown_ms);
	if (d->state != D_FREE)
		return (0);
	top = heap_top(&d->waiters);
	if (top == -1 || top == coder->id)
		return (1);
	return (can_be_passed_over(&coder->shared->coders[top - 1], d));
}

static void	claim_both(t_dongle *f, t_dongle *s, t_coder *coder)
{
	f->state = D_TAKEN;
	s->state = D_TAKEN;
	heap_remove(&f->waiters, coder->id);
	heap_remove(&s->waiters, coder->id);
	set_blocked_on(coder, NULL);
}

void	enqueue_both(t_coder *coder, t_dongle *f, t_dongle *s, long key)
{
	pthread_mutex_lock(&f->lock);
	heap_push(&f->waiters, coder->id, key);
	pthread_mutex_unlock(&f->lock);
	pthread_mutex_lock(&s->lock);
	heap_push(&s->waiters, coder->id, key);
	pthread_mutex_unlock(&s->lock);
}

void	dequeue_both(t_coder *coder, t_dongle *f, t_dongle *s)
{
	pthread_mutex_lock(&f->lock);
	heap_remove(&f->waiters, coder->id);
	pthread_cond_broadcast(&f->cond);
	pthread_mutex_unlock(&f->lock);
	pthread_mutex_lock(&s->lock);
	heap_remove(&s->waiters, coder->id);
	pthread_cond_broadcast(&s->cond);
	pthread_mutex_unlock(&s->lock);
}

/*
** 2本を「両方取れるときだけ同時に取る」。片方だけ握って待つ（hold and wait）と
** 全員が1本ずつ持って止まり、同時に compile できる人数が半分に落ちる。
** 成功なら NULL（ロックは両方解放済み）。失敗なら取れなかった dongle を
** lock したまま返す。lock は必ず id の昇順（f -> s）で取る。
*/
t_dongle	*try_take_pair(t_coder *coder, t_dongle *f, t_dongle *s)
{
	long	cooldown;

	cooldown = coder->shared->dongle_cooldown;
	pthread_mutex_lock(&f->lock);
	if (!can_take(f, coder, cooldown))
	{
		set_blocked_on(coder, f);
		return (f);
	}
	pthread_mutex_lock(&s->lock);
	if (!can_take(s, coder, cooldown))
	{
		set_blocked_on(coder, s);
		pthread_mutex_unlock(&f->lock);
		return (s);
	}
	claim_both(f, s, coder);
	pthread_mutex_unlock(&s->lock);
	pthread_mutex_unlock(&f->lock);
	return (NULL);
}
