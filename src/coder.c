/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:00:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/11 16:30:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include <stdlib.h>

static void	init_one_coder(t_coder *c, int id, t_shared *shared)
{
	c->id = id;
	c->compile_count = 0;
	c->blocked_on = NULL;
	c->last_compile_start_us = 0;
	c->shared = shared;
}

static void	link_dongles(t_shared *shared)
{
	int	i;
	int	n;

	n = shared->num_coders;
	i = 0;
	while (i < n)
	{
		shared->coders[i].right = &shared->dongles[i];
		shared->coders[i].left = &shared->dongles[(i - 1 + n) % n];
		i++;
	}
}

int	init_coders(t_shared *shared)
{
	int	i;
	int	n;

	n = shared->num_coders;
	shared->coders = malloc(sizeof(t_coder) * n);
	if (!shared->coders)
		return (error_sys("malloc of the coder array"));
	i = 0;
	while (i < n)
	{
		init_one_coder(&shared->coders[i], i + 1, shared);
		if (pthread_mutex_init(&shared->coders[i].state_lock, NULL) != 0)
		{
			error_sys_id("pthread_mutex_init for coder", i + 1);
			while (--i >= 0)
				pthread_mutex_destroy(&shared->coders[i].state_lock);
			free(shared->coders);
			return (1);
		}
		i++;
	}
	link_dongles(shared);
	return (0);
}

void	destroy_coders(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->num_coders)
	{
		pthread_mutex_destroy(&shared->coders[i].state_lock);
		i++;
	}
	free(shared->coders);
}
