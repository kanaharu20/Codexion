/*
** 01: pthread_create / pthread_join
**
** ・スレッドを作ると、その瞬間から並行に走り出す（main は待たない）
** ・arg は void* が1個だけ。複数渡したいなら構造体にまとめる
** ・戻り値は join の第2引数で受け取る
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct s_task
{
	int	id;
	int	sleep_ms;
}	t_task;

/* スレッドの入口は必ずこの型: void *(*)(void *) */
static void	*worker(void *arg)
{
	t_task	*task;
	int		*result;

	task = (t_task *)arg;
	printf("  worker %d: start (%d ms)\n", task->id, task->sleep_ms);
	usleep(task->sleep_ms * 1000);
	printf("  worker %d: done\n", task->id);
	/*
	** 戻り値はスレッド終了後も生きている領域を指す必要がある。
	** ローカル変数のアドレスを返すのは未定義動作。
	*/
	result = malloc(sizeof(int));
	if (!result)
		return (NULL);
	*result = task->id * 100;
	return (result);
}

int	main(void)
{
	pthread_t	th[3];
	t_task		tasks[3];
	void		*ret;
	int			i;

	i = 0;
	while (i < 3)
	{
		tasks[i].id = i + 1;
		tasks[i].sleep_ms = (3 - i) * 200;
		/*
		** ★ &tasks[i] のように「要素ごとに別アドレス」を渡す。
		**    &i を渡すと全スレッドが同じ変数を見てしまい破綻する。
		** ★ 失敗時は -1 ではなく errno そのものを返す。errno はセットされない。
		*/
		if (pthread_create(&th[i], NULL, worker, &tasks[i]) != 0)
		{
			fprintf(stderr, "pthread_create failed\n");
			return (1);
		}
		printf("main: created worker %d\n", tasks[i].id);
		i++;
	}
	printf("main: 3本とも既に走っている。ここから join する\n");
	i = 0;
	while (i < 3)
	{
		/* join は「そのスレッド」が終わるまでブロックする。順番は指定した通り */
		pthread_join(th[i], &ret);
		printf("main: joined worker %d -> result=%d\n", i + 1, *(int *)ret);
		free(ret);
		i++;
	}
	return (0);
}
