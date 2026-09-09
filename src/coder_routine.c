/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:05:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:40:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

/*
** 規定回数に達したかを state_lock の下で読む。compile_count は monitor も
** 読むので、ロック無しで触る場所を作らない。
*/
static int	is_done(t_coder *coder)
{
	int	done;

	pthread_mutex_lock(&coder->state_lock);
	done = (coder->compile_count >= coder->shared->num_compile_req);
	pthread_mutex_unlock(&coder->state_lock);
	return (done);
}

static void	do_compile_phase(t_coder *coder)
{
	long	now;

	now = elapsed_us(coder->shared);
	pthread_mutex_lock(&coder->state_lock);
	coder->last_compile_start_us = now;
	pthread_mutex_unlock(&coder->state_lock);
	log_state(coder->shared, coder->id, "is compiling");
	precise_sleep(coder->shared->t_to_compile);
	pthread_mutex_lock(&coder->state_lock);
	coder->compile_count++;
	pthread_mutex_unlock(&coder->state_lock);
}

static void	do_debug_phase(t_coder *coder)
{
	log_state(coder->shared, coder->id, "is debugging");
	precise_sleep(coder->shared->t_to_debug);
}

static void	do_refactor_phase(t_coder *coder)
{
	log_state(coder->shared, coder->id, "is refactoring");
	precise_sleep(coder->shared->t_to_refactor);
}

/*
** 規定回数に達した時点でシミュレーションは終わりなので、最後の compile の
** あとに debug / refactor を続けない。続けると停止条件を満たしたあとにも
** ログが出て、終了も time_to_debug + time_to_refactor だけ遅れる。
*/
void	*coder_thread(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	while (!is_done(coder))
	{
		if (acquire_two_dongles(coder) != 0)
			break ;
		do_compile_phase(coder);
		release_two_dongles(coder);
		if (is_done(coder) || is_stopped(coder->shared))
			break ;
		do_debug_phase(coder);
		do_refactor_phase(coder);
	}
	return (NULL);
}
