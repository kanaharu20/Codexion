/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:12:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/11 16:30:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

static int	spawn_coders(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->num_coders)
	{
		if (pthread_create(&shared->coders[i].thread, NULL, coder_thread,
				&shared->coders[i]) != 0)
		{
			error_sys_id("pthread_create for coder", i + 1);
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

static int	destroy_and_fail(t_shared *shared)
{
	destroy_shared(shared);
	return (1);
}

int	run(t_args *ins)
{
	t_shared	shared;

	if (init_shared(&shared, ins) != 0)
		return (1);
	if (spawn_coders(&shared) != 0)
		return (destroy_and_fail(&shared));
	if (pthread_create(&shared.monitor, NULL, monitor_thread, &shared) != 0)
	{
		error_sys("pthread_create for the monitor thread");
		set_stopped(&shared);
		wake_all_dongles(&shared);
		join_coders(&shared);
		return (destroy_and_fail(&shared));
	}
	pthread_join(shared.monitor, NULL);
	join_coders(&shared);
	destroy_shared(&shared);
	return (0);
}
