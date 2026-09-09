/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:28:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:40:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include <unistd.h>

/*
** 1人分の判定。0 = 生存中 / 1 = burnout / 2 = 規定回数を達成済み。
** 判定はマイクロ秒で行う。ミリ秒に丸めると切り捨て分だけ期限が最大1ms
** 早まり、burnout を過剰に検知してしまうため。
** 達成済みの coder は以後 compile を始めないので判定対象から外す。
*/
static int	check_one_coder(t_coder *coder, long now_us)
{
	long	deadline;
	int		done;

	pthread_mutex_lock(&coder->state_lock);
	done = (coder->compile_count >= coder->shared->num_compile_req);
	deadline = coder->last_compile_start_us
		+ coder->shared->t_to_burnout * 1000L;
	pthread_mutex_unlock(&coder->state_lock);
	if (done)
		return (2);
	if (now_us >= deadline)
		return (1);
	return (0);
}

/*
** 全員を1周見る。0 = 続行 / 1 = 監視終了（burnout または全員達成）。
*/
static int	scan_coders(t_shared *shared)
{
	int		i;
	int		res;
	int		finished;
	long	now;

	i = 0;
	finished = 0;
	now = elapsed_us(shared);
	while (i < shared->num_coders)
	{
		res = check_one_coder(&shared->coders[i], now);
		if (res == 1)
		{
			set_stopped(shared);
			log_burnout(shared, shared->coders[i].id);
			wake_all_dongles(shared);
			return (1);
		}
		if (res == 2)
			finished++;
		i++;
	}
	return (finished == shared->num_coders);
}

void	*monitor_thread(void *arg)
{
	t_shared	*shared;

	shared = (t_shared *)arg;
	while (scan_coders(shared) == 0)
		usleep(1000);
	return (NULL);
}
