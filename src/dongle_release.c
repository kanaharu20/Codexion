#include "header.h"

static void	release_one(t_dongle *d)
{
	pthread_mutex_lock(&d->lock);
	d->state = D_COOLDOWN;
	gettimeofday(&d->release_time, NULL);
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->lock);
}

void	release_two_dongles(t_coder *coder)
{
	release_one(coder->left);
	if (coder->right != coder->left)
		release_one(coder->right);
}
