#include "header.h"

/*
** ロック順序の約束: d->lock / log_lock → stop_lock。
** stop_lock を握ったまま d->lock や log_lock を取ってはいけない。
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
