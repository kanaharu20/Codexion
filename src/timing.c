/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   timing.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/09 15:20:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:40:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include <unistd.h>

/*
** usleep(ms * 1000) の一発待ちはスケジューラ次第で数ms単位に伸びる。
** 締切が厳しいパラメータではその誤差がそのまま burnout につながるので、
** 短い usleep を重ねながら経過時間を見て、期限を跨いだ時点で戻る。
*/
void	precise_sleep(long ms)
{
	struct timeval	start;
	struct timeval	now;
	long			target;
	long			elapsed;

	if (ms <= 0)
		return ;
	target = ms * 1000L;
	gettimeofday(&start, NULL);
	while (1)
	{
		gettimeofday(&now, NULL);
		elapsed = (now.tv_sec - start.tv_sec) * 1000000L
			+ (now.tv_usec - start.tv_usec);
		if (elapsed >= target)
			return ;
		if (target - elapsed > 1000)
			usleep(200);
		else
			usleep(50);
	}
}
