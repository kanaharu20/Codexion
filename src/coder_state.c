/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_state.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/09 16:50:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 17:00:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

/*
** いま待っている dongle を記録する。待っていないときは NULL。
** 呼び出し側が d->lock を持ったまま呼ぶので、ロック順序は d->lock -> state_lock。
*/
void	set_blocked_on(t_coder *coder, t_dongle *d)
{
	pthread_mutex_lock(&coder->state_lock);
	coder->blocked_on = d;
	pthread_mutex_unlock(&coder->state_lock);
}

/*
** 待ち行列の先頭にいるこの coder を飛ばして d を取ってよいか。
** 条件は2つとも満たすときだけ。
**   1. 別の dongle を待って止まっている = いま d を渡しても使えない。
**      予約させたままにすると、その隣人まで連鎖して誰も動けなくなる。
**   2. 丸1周ぶんの余裕が残っている。burnout が近い coder は絶対に飛ばさない。
**      これが liveness を守る側の条件で、締切が迫った要求は必ず優先される。
*/
int	can_be_passed_over(t_coder *coder, t_dongle *d)
{
	long	slack;
	long	cycle;
	int		blocked;

	cycle = coder->shared->t_to_compile + coder->shared->t_to_debug
		+ coder->shared->t_to_refactor;
	pthread_mutex_lock(&coder->state_lock);
	blocked = (coder->blocked_on != NULL && coder->blocked_on != d);
	slack = coder->last_compile_start_us
		+ coder->shared->t_to_burnout * 1000L;
	pthread_mutex_unlock(&coder->state_lock);
	slack -= elapsed_us(coder->shared);
	return (blocked && slack > cycle * 1000L);
}

/*
** d が解放された瞬間に、d を待っていた coder を「待ち状態ではない」に戻す。
** これをやらないと、起きて取りに行くまでの隙に横取りされ続け、
** 待ち行列の先頭にいる coder がいつまでも compile できなくなる。
** 呼び出し側は d->lock を持っていること。
*/
void	release_reservations(t_dongle *d)
{
	int		i;
	t_coder	*waiter;

	i = 0;
	while (i < d->waiters.size)
	{
		waiter = &d->shared->coders[d->waiters.entries[i].coder_id - 1];
		pthread_mutex_lock(&waiter->state_lock);
		if (waiter->blocked_on == d)
			waiter->blocked_on = NULL;
		pthread_mutex_unlock(&waiter->state_lock);
		i++;
	}
}
