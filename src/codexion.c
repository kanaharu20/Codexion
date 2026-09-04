/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:12:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/04 14:30:31 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include <stdio.h>

static int	spawn_coders(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->num_coders)
	{
		if (pthread_create(&shared->coders[i].thread, NULL, coder_thread,
				&shared->coders[i]) != 0)
		{
			set_stopped(shared);
			wake_all_dongles(shared);
			while (--i >= 0)
				pthread_join(shared->coders[i].thread, NULL);
			return (1);
		}
		i++;
	}
	return (0);
}

static void	join_coders(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->num_coders)
	{
		pthread_join(shared->coders[i].thread, NULL);
		i++;
	}
}

static int	error_exit(t_shared *shared, int inited)
{
	if (inited)
		destroy_shared(shared);
	fprintf(stderr, "Error\n");
	return (1);
}

int	run(args *ins)
{
	t_shared	shared;

	if (init_shared(&shared, ins) != 0)
		return (error_exit(&shared, 0));
	if (spawn_coders(&shared) != 0)
		return (error_exit(&shared, 1));
	if (pthread_create(&shared.monitor, NULL, monitor_thread, &shared) != 0)
	{
		set_stopped(&shared);
		wake_all_dongles(&shared);
		join_coders(&shared);
		return (error_exit(&shared, 1));
	}
	pthread_join(shared.monitor, NULL);
	join_coders(&shared);
	destroy_shared(&shared);
	return (0);
}
