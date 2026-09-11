/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:55:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/11 16:30:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include <stdlib.h>

static int	init_one_dongle(t_shared *shared, t_dongle *d, int id)
{
	d->shared = shared;
	d->id = id;
	d->state = D_FREE;
	d->release_time.tv_sec = 0;
	d->release_time.tv_usec = 0;
	d->waiters.size = 0;
	if (pthread_mutex_init(&d->lock, NULL) != 0)
		return (error_sys_id("pthread_mutex_init for dongle", id));
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&d->lock);
		return (error_sys_id("pthread_cond_init for dongle", id));
	}
	return (0);
}

static void	destroy_one_dongle(t_dongle *d)
{
	pthread_mutex_destroy(&d->lock);
	pthread_cond_destroy(&d->cond);
}

int	init_dongles(t_shared *shared)
{
	int	i;

	shared->dongles = malloc(sizeof(t_dongle) * shared->num_coders);
	if (!shared->dongles)
		return (error_sys("malloc of the dongle array"));
	i = 0;
	while (i < shared->num_coders)
	{
		if (init_one_dongle(shared, &shared->dongles[i], i + 1) != 0)
		{
			while (--i >= 0)
				destroy_one_dongle(&shared->dongles[i]);
			free(shared->dongles);
			return (1);
		}
		i++;
	}
	return (0);
}

void	destroy_dongles(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->num_coders)
	{
		destroy_one_dongle(&shared->dongles[i]);
		i++;
	}
	free(shared->dongles);
}
