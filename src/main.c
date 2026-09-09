/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:51:07 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/04 14:30:51 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool	is_valid_num(const char *s)
{
	int	i;

	if (!s[0])
		return (false);
	i = 0;
	while (s[i])
	{
		if (s[i] < '0' || '9' < s[i])
			return (false);
		i++;
	}
	return (true);
}

bool	ori_atoi(const char *str, int *arg)
{
	int	i;
	int	num;
	int	digit;

	if (!is_valid_num(str))
		return (false);
	i = 0;
	num = 0;
	while (str[i])
	{
		digit = str[i] - '0';
		if (num > (INT_MAX - digit) / 10)
			return (false);
		num = num * 10 + digit;
		i++;
	}
	*arg = num;
	return (true);
}

bool	is_valid_schedule(const char *s)
{
	return (strcmp(s, "fifo") == 0 || strcmp(s, "edf") == 0);
}

bool	parse_numbers(char **argv, t_args *ins)
{
	if (!(ori_atoi(argv[1], &ins->num_coders) && ori_atoi(argv[2],
				&ins->t_to_burnout) && ori_atoi(argv[3], &ins->t_to_compile)
			&& ori_atoi(argv[4], &ins->t_to_debug) && ori_atoi(argv[5],
				&ins->t_to_refactor) && ori_atoi(argv[6], &ins->num_compile_req)
			&& ori_atoi(argv[7], &ins->dongle_cooldown)))
		return (false);
	if (ins->num_coders < 1 || ins->num_compile_req < 1)
		return (false);
	return (true);
}

int	main(int argc, char **argv)
{
	t_args	ins;

	if (argc != 9)
	{
		fprintf(stderr, "Error\n");
		return (1);
	}
	if (!parse_numbers(argv, &ins) || !is_valid_schedule(argv[8]))
	{
		fprintf(stderr, "Error\n");
		return (1);
	}
	ins.scheduler = argv[8];
	return (run(&ins));
}
