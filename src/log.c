/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:25:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:40:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include <stdio.h>

long	elapsed_us(t_shared *shared)
{
	struct timeval	now;

	gettimeofday(&now, NULL);
	return ((now.tv_sec - shared->start_time.tv_sec) * 1000000L
		+ (now.tv_usec - shared->start_time.tv_usec));
}

long	elapsed_ms(t_shared *shared)
{
	return (elapsed_us(shared) / 1000);
}

void	log_state(t_shared *shared, int coder_id, const char *msg)
{
	long	ts;

	pthread_mutex_lock(&shared->log_lock);
	if (!is_stopped(shared))
	{
		ts = elapsed_ms(shared);
		printf("%ld %d %s\n", ts, coder_id, msg);
	}
	pthread_mutex_unlock(&shared->log_lock);
}

void	log_burnout(t_shared *shared, int coder_id)
{
	long	ts;

	pthread_mutex_lock(&shared->log_lock);
	ts = elapsed_ms(shared);
	printf("%ld %d burned out\n", ts, coder_id);
	pthread_mutex_unlock(&shared->log_lock);
}
