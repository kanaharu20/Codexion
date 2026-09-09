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

/*
** stop_lock は最内側のロック。握ったまま d->lock や log_lock を取らない。
** 全体のロック順序は incl/header.h の t_shared のコメントを参照。
*/

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

/*
** dongle 待ちで寝ている coder を全員起こす。
** 待機側は各 dongle の cond で寝ているので、そちらへ broadcast する必要がある。
*/
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

/*
** coder が 1 人のときは left == right となり 2 本目を永久に取得できない。
** 停止が立つまで dongle の cond で寝る。監視スレッドが burnout を検知して
** set_stopped → wake_all_dongles する broadcast で起こされる。
** ロック順序 d->lock → stop_lock を守っている。
*/
void	wait_until_stopped(t_dongle *d, t_shared *shared)
{
	pthread_mutex_lock(&d->lock);
	while (!is_stopped(shared))
		pthread_cond_wait(&d->cond, &d->lock);
	pthread_mutex_unlock(&d->lock);
}
