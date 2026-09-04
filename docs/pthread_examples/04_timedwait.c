/*
** 04: pthread_cond_timedwait + gettimeofday / clock_gettime
**
** cooldown（返却後しばらく再取得できない）の実装。
** 「起こされる」か「期限が来る」かの早い方まで待つ。
** dongle_cooldown.c と同じ構造。
**
** ★ timedwait に渡すのは「あと何ms」ではなく「いつまで」の絶対時刻。
**   しかも clock は CLOCK_REALTIME（epoch 起点）で、struct timespec（ns精度）。
**   gettimeofday は struct timeval（µs精度）なので、変換が要る。
*/

#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define COOLDOWN_MS	300

typedef enum e_state
{
	S_FREE,
	S_TAKEN,
	S_COOLDOWN
}	t_state;

typedef struct s_res
{
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
	t_state			state;
	struct timeval	release_time;
}	t_res;

static t_res			g_res;
static struct timeval	g_start;

static long	elapsed_ms(void)
{
	struct timeval	now;

	gettimeofday(&now, NULL);
	return ((now.tv_sec - g_start.tv_sec) * 1000
		+ (now.tv_usec - g_start.tv_usec) / 1000);
}

/* release_time + COOLDOWN_MS を timespec に組み立てる */
static struct timespec	cooldown_deadline(void)
{
	struct timespec	ts;
	long			total_nsec;

	ts.tv_sec = g_res.release_time.tv_sec + COOLDOWN_MS / 1000;
	total_nsec = g_res.release_time.tv_usec * 1000L
		+ (COOLDOWN_MS % 1000) * 1000000L;
	/* ★ tv_nsec は 0〜999999999 に収める。溢れた分は tv_sec へ繰り上げ */
	ts.tv_nsec = total_nsec % 1000000000L;
	ts.tv_sec += total_nsec / 1000000000L;
	return (ts);
}

/*
** cooldown は「時間が経つ」だけで誰も書き換えない状態遷移なので、
** 参照するタイミングで期限を過ぎていないか確認して FREE に落とす。
*/
static void	refresh_state(void)
{
	struct timespec	now;
	struct timespec	deadline;

	if (g_res.state != S_COOLDOWN)
		return ;
	clock_gettime(CLOCK_REALTIME, &now);
	deadline = cooldown_deadline();
	if (now.tv_sec > deadline.tv_sec
		|| (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec))
		g_res.state = S_FREE;
}

static void	*worker(void *arg)
{
	struct timespec	deadline;
	int				id;
	int				n;

	id = *(int *)arg;
	n = 0;
	while (n < 2)
	{
		pthread_mutex_lock(&g_res.lock);
		refresh_state();
		while (g_res.state != S_FREE)
		{
			if (g_res.state == S_COOLDOWN)
			{
				deadline = cooldown_deadline();
				/* 期限が来たら ETIMEDOUT で自力で起きる。
				** 誰も broadcast してくれないので timedwait が必須 */
				pthread_cond_timedwait(&g_res.cond, &g_res.lock, &deadline);
			}
			else
				pthread_cond_wait(&g_res.cond, &g_res.lock);
			refresh_state();
		}
		g_res.state = S_TAKEN;
		printf("%4ld ms  worker %d: took\n", elapsed_ms(), id);
		pthread_mutex_unlock(&g_res.lock);
		usleep(50 * 1000);
		pthread_mutex_lock(&g_res.lock);
		g_res.state = S_COOLDOWN;
		gettimeofday(&g_res.release_time, NULL);
		printf("%4ld ms  worker %d: released -> cooldown %d ms\n",
			elapsed_ms(), id, COOLDOWN_MS);
		pthread_cond_broadcast(&g_res.cond);
		pthread_mutex_unlock(&g_res.lock);
		n++;
	}
	return (NULL);
}

int	main(void)
{
	pthread_t	th[2];
	int			ids[2];
	int			i;

	gettimeofday(&g_start, NULL);
	pthread_mutex_init(&g_res.lock, NULL);
	pthread_cond_init(&g_res.cond, NULL);
	g_res.state = S_FREE;
	i = 0;
	while (i < 2)
	{
		ids[i] = i + 1;
		pthread_create(&th[i], NULL, worker, &ids[i]);
		i++;
	}
	i = 0;
	while (i < 2)
	{
		pthread_join(th[i], NULL);
		i++;
	}
	pthread_cond_destroy(&g_res.cond);
	pthread_mutex_destroy(&g_res.lock);
	return (0);
}
