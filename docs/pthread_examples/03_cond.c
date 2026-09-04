/*
** 03: pthread_cond_init / wait / broadcast / destroy
**
** 「ある条件が成立するまで寝て待つ」。busy-wait（whileで回し続ける）と違い
** CPU を消費しない。dongle_acquire.c の待機がまさにこれ。
**
** cond_wait の3つの動作（これがアトミックに起きるのが肝）:
**   1. 渡された mutex を手放す
**   2. 眠る
**   3. 起こされたら mutex を取り直してから返る
** 1と2の間に隙間が無いので、条件確認の直後に broadcast が飛んでも
** 取りこぼさない（lost wakeup が起きない）。
*/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

typedef struct s_res
{
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
	int				taken;	/* この変数を lock が保護する */
}	t_res;

static t_res	g_res;

static void	*worker(void *arg)
{
	int	id;
	int	n;

	id = *(int *)arg;
	n = 0;
	while (n < 2)
	{
		pthread_mutex_lock(&g_res.lock);
		/*
		** ★ if ではなく while で囲うこと。理由は2つ:
		**   1) broadcast は待機者を「全員」起こす。起きても他人に先を
		**      越されていて、条件がまだ偽のことがある
		**   2) spurious wakeup: 何も起きていないのに起床する場合が
		**      仕様上ありうる
		** どちらも「起きたらもう一度条件を確認する」で正しく処理できる。
		*/
		while (g_res.taken)
			pthread_cond_wait(&g_res.cond, &g_res.lock);
		g_res.taken = 1;
		printf("worker %d: took   (%d回目)\n", id, n + 1);
		pthread_mutex_unlock(&g_res.lock);
		usleep(100 * 1000);
		pthread_mutex_lock(&g_res.lock);
		g_res.taken = 0;
		printf("worker %d: released\n", id);
		/*
		** ★ 状態を変えた「後」に通知する。順序が逆だと、起きた側が
		**   まだ古い状態を見て寝直すだけになる。
		** ★ signal は1人だけ起こす。誰を起こすかを選べない以上、
		**   「先頭が自分か」のような条件付き待機では broadcast が安全。
		*/
		pthread_cond_broadcast(&g_res.cond);
		pthread_mutex_unlock(&g_res.lock);
		usleep(10 * 1000);
		n++;
	}
	return (NULL);
}

int	main(void)
{
	pthread_t	th[3];
	int			ids[3];
	int			i;

	pthread_mutex_init(&g_res.lock, NULL);
	pthread_cond_init(&g_res.cond, NULL);
	g_res.taken = 0;
	i = 0;
	while (i < 3)
	{
		ids[i] = i + 1;
		pthread_create(&th[i], NULL, worker, &ids[i]);
		i++;
	}
	i = 0;
	while (i < 3)
	{
		pthread_join(th[i], NULL);
		i++;
	}
	pthread_cond_destroy(&g_res.cond);
	pthread_mutex_destroy(&g_res.lock);
	return (0);
}
