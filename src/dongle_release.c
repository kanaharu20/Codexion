/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_release.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 14:22:00 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:40:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

void	release_one_dongle(t_dongle *d)
{
	pthread_mutex_lock(&d->lock);
	d->state = D_COOLDOWN;
	gettimeofday(&d->release_time, NULL);
	release_reservations(d);
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->lock);
}

void	release_two_dongles(t_coder *coder)
{
	release_one_dongle(coder->left);
	if (coder->right != coder->left)
		release_one_dongle(coder->right);
}
