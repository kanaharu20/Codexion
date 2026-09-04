/*
** 02: pthread_mutex_init / lock / unlock / destroy
**
** ロック無しだとなぜ壊れるかを実際に見る。
** c->value++ は「読む→+1する→書く」の3段階で、途中で他スレッドに
** 割り込まれると更新が消える（lost update）。
*/

#include <pthread.h>
#include <stdio.h>

#define N_THREADS	4
#define N_LOOP		200000

typedef struct s_counter
{
	long			value;
	pthread_mutex_t	lock;
	int				use_lock;
}	t_counter;

static void	*bump(void *arg)
{
	t_counter	*c;
	int			i;

	c = (t_counter *)arg;
	i = 0;
	while (i < N_LOOP)
	{
		if (c->use_lock)
		{
			pthread_mutex_lock(&c->lock);
			c->value++;
			pthread_mutex_unlock(&c->lock);
		}
		else
			c->value++;
		i++;
	}
	return (NULL);
}

static long	run_test(int use_lock)
{
	pthread_t	th[N_THREADS];
	t_counter	c;
	int			i;

	c.value = 0;
	c.use_lock = use_lock;
	/* mutex は使う前に必ず init、使い終わったら destroy */
	pthread_mutex_init(&c.lock, NULL);
	i = 0;
	while (i < N_THREADS)
	{
		pthread_create(&th[i], NULL, bump, &c);
		i++;
	}
	i = 0;
	while (i < N_THREADS)
	{
		pthread_join(th[i], NULL);
		i++;
	}
	/* ★ 誰かが握っている mutex を destroy するのは未定義動作。
	**   必ず全 join が終わってから destroy する。 */
	pthread_mutex_destroy(&c.lock);
	return (c.value);
}

int	main(void)
{
	printf("expected      : %d\n", N_THREADS * N_LOOP);
	printf("without mutex : %ld\t<- 壊れる（実行するたび違う値）\n", run_test(0));
	printf("with mutex    : %ld\t<- 常に正しい\n", run_test(1));
	return (0);
}
