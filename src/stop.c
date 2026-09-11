/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   stop.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:26:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:40:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

int	is_stopped(t_shared *shared)
{
	int	res;

	pthread_mutex_lock(&shared->stop_lock);
	res = shared->stopped;
	pthread_mutex_unlock(&shared->stop_lock);
	return (res);
}

void	set_stopped(t_shared *shared)
{
	pthread_mutex_lock(&shared->stop_lock);
	shared->stopped = 1;
	pthread_mutex_unlock(&shared->stop_lock);
}

void	wake_all_dongles(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->num_coders)
	{
		pthread_mutex_lock(&shared->dongles[i].lock);
		pthread_cond_broadcast(&shared->dongles[i].cond);
		pthread_mutex_unlock(&shared->dongles[i].lock);
		i++;
	}
}

void	wait_until_stopped(t_dongle *d, t_shared *shared)
{
	pthread_mutex_lock(&d->lock);
	while (!is_stopped(shared))
		pthread_cond_wait(&d->cond, &d->lock);
	pthread_mutex_unlock(&d->lock);
}
