/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:12:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/08/28 14:12:27 by hkanamit         ###   ########.fr       */
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
		if (pthread_create(&shared->coders[i].thread, NULL,
				coder_thread, &shared->coders[i]) != 0)
		{
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

int	run(args *ins)
{
	t_shared	shared;

	if (init_shared(&shared, ins) != 0)
	{
		fprintf(stderr, "Error\n");
		return (1);
	}
	if (spawn_coders(&shared) != 0)
	{
		destroy_shared(&shared);
		fprintf(stderr, "Error\n");
		return (1);
	}
	join_coders(&shared);
	destroy_shared(&shared);
	return (0);
}
