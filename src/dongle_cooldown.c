/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_cooldown.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:18:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:40:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

struct timespec	cooldown_deadline(t_dongle *d, long cooldown_ms)
{
	struct timespec	ts;
	long			total_nsec;

	ts.tv_sec = d->release_time.tv_sec + cooldown_ms / 1000;
	total_nsec = d->release_time.tv_usec * 1000L + (cooldown_ms % 1000)
		* 1000000L;
	ts.tv_nsec = total_nsec % 1000000000L;
	ts.tv_sec += total_nsec / 1000000000L;
	return (ts);
}

void	refresh_dongle_state(t_dongle *d, long cooldown_ms)
{
	struct timespec	now;
	struct timespec	deadline;

	if (d->state != D_COOLDOWN)
		return ;
	clock_gettime(CLOCK_REALTIME, &now);
	deadline = cooldown_deadline(d, cooldown_ms);
	if (now.tv_sec > deadline.tv_sec || (now.tv_sec == deadline.tv_sec
			&& now.tv_nsec >= deadline.tv_nsec))
		d->state = D_FREE;
}

/*
** 取れなかった dongle の cond で待つ。呼び出し時も戻り時も d->lock は保持。
** cooldown 中はその期限まで、それ以外は release / 行列の変化まで眠る。
*/
void	wait_on_blocker(t_dongle *d, t_coder *coder)
{
	struct timespec	deadline;

	if (d->state == D_COOLDOWN)
	{
		deadline = cooldown_deadline(d, coder->shared->dongle_cooldown);
		pthread_cond_timedwait(&d->cond, &d->lock, &deadline);
	}
	else
		pthread_cond_wait(&d->cond, &d->lock);
	refresh_dongle_state(d, coder->shared->dongle_cooldown);
}
