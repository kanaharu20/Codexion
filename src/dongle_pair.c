/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_pair.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/09 16:00:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/11 15:00:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

static int	can_take(t_dongle *d, t_coder *coder)
{
	int	top;

	refresh_dongle_state(d, coder->shared->dongle_cooldown);
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

t_dongle	*try_take_pair(t_coder *coder, t_dongle *f, t_dongle *s)
{
	pthread_mutex_lock(&f->lock);
	if (!can_take(f, coder))
	{
		set_blocked_on(coder, f);
		pthread_mutex_unlock(&f->lock);
		return (f);
	}
	pthread_mutex_lock(&s->lock);
	if (!can_take(s, coder))
	{
		set_blocked_on(coder, s);
		pthread_mutex_unlock(&s->lock);
		pthread_mutex_unlock(&f->lock);
		return (s);
	}
	claim_both(f, s, coder);
	pthread_mutex_unlock(&s->lock);
	pthread_mutex_unlock(&f->lock);
	return (NULL);
}

void	wait_for_dongle(t_dongle *d, t_coder *coder)
{
	pthread_mutex_lock(&d->lock);
	if (!can_take(d, coder))
		wait_on_cond(d, coder);
	pthread_mutex_unlock(&d->lock);
}
