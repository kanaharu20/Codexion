/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_queue.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/11 15:00:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/11 15:00:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

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
