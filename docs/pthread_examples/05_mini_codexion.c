/*
** 05: 本番の縮小版（coder 3人 / dongle 3本 / FIFO のみ / cooldown なし）
**
** codexion の骨格をこの1ファイルに圧縮したもの。ここまでの4本で出た
** 関数が全部登場する。実装の3つの層が独立していることを見てほしい:
**
**   1. デッドロック回避 : 必ず id の小さい dongle から取る（資源順序付け）
**   2. スケジューリング : waiters(最大2要素) の最小キーが取得権を持つ
**   3. 停止             : stopped を立てて全 cond に broadcast
*/

#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#define N			3
#define T_COMPILE	60
#define T_DEBUG		60
#define T_BURNOUT	600
#define N_REQUIRED	3

/* ---- 待ち行列: 1本の dongle を狙うのは隣り合う2人だけなので固定長2 ---- */
typedef struct s_heap
{
	int		id[2];
	long	key[2];
	int		size;
}	t_heap;

typedef struct s_dongle
{
	int				id;
	pthread_mutex_t	lock;	/* 以下すべてを保護 */
	pthread_cond_t	cond;
	int				taken;
	t_heap			waiters;
}	t_dongle;

typedef struct s_coder
{
	int				id;
	pthread_t		th;
	t_dongle		*left;
	t_dongle		*right;
	pthread_mutex_t	state_lock;	/* 以下2つを保護 */
	struct timeval	last_compile;
	int				count;
}	t_coder;

static t_dongle			g_d[N];
static t_coder			g_c[N];
static struct timeval	g_start;
static int				g_stopped;
static pthread_mutex_t	g_stop_lock;
static pthread_mutex_t	g_log_lock;

/* ---------------- 時刻 ---------------- */

static long	now_us(void)
{
	struct timeval	now;

	gettimeofday(&now, NULL);
	return (now.tv_sec * 1000000L + now.tv_usec);
}

static long	elapsed_ms(void)
{
	return ((now_us() - (g_start.tv_sec * 1000000L + g_start.tv_usec)) / 1000);
}

/* ---------------- 停止フラグ ---------------- */
/* ロック順序: d->lock / log_lock -> stop_lock。stop_lock は常に最内側 */

static int	is_stopped(void)
{
	int	res;

	pthread_mutex_lock(&g_stop_lock);
	res = g_stopped;
	pthread_mutex_unlock(&g_stop_lock);
	return (res);
}

static void	set_stopped(void)
{
	pthread_mutex_lock(&g_stop_lock);
	g_stopped = 1;
	pthread_mutex_unlock(&g_stop_lock);
}

/* cond で寝ている全員を起こす。これが無いと停止しても誰も目を覚まさない */
static void	wake_all(void)
{
	int	i;

	i = 0;
	while (i < N)
	{
		pthread_mutex_lock(&g_d[i].lock);
		pthread_cond_broadcast(&g_d[i].cond);
		pthread_mutex_unlock(&g_d[i].lock);
		i++;
	}
}

static void	log_state(int id, const char *msg)
{
	long	ts;

	ts = elapsed_ms();
	pthread_mutex_lock(&g_log_lock);
	if (!is_stopped())
		printf("%ld %d %s\n", ts, id, msg);
	pthread_mutex_unlock(&g_log_lock);
}

/* ---------------- 待ち行列（最小キーが先頭） ---------------- */

static void	heap_push(t_heap *h, int id, long key)
{
	long	tk;
	int		ti;

	if (h->size >= 2)
		return ;
	h->id[h->size] = id;
	h->key[h->size] = key;
	h->size++;
	if (h->size == 2 && h->key[1] < h->key[0])
	{
		tk = h->key[0];
		ti = h->id[0];
		h->key[0] = h->key[1];
		h->id[0] = h->id[1];
		h->key[1] = tk;
		h->id[1] = ti;
	}
}

static int	heap_top(t_heap *h)
{
	if (h->size == 0)
		return (-1);
	return (h->id[0]);
}

static void	heap_pop(t_heap *h)
{
	if (h->size == 0)
		return ;
	if (h->size == 2)
	{
		h->id[0] = h->id[1];
		h->key[0] = h->key[1];
	}
	h->size--;
}

static void	heap_remove(t_heap *h, int id)
{
	if (h->size > 0 && h->id[0] == id)
		heap_pop(h);
	else if (h->size == 2 && h->id[1] == id)
		h->size--;
}

/* ---------------- 取得と解放 ---------------- */

/* 0 = 取得できた / 1 = 停止したので諦めた */
static int	acquire_one(t_dongle *d, t_coder *c, long key)
{
	pthread_mutex_lock(&d->lock);
	heap_push(&d->waiters, c->id, key);
	/* 起床条件は「空いている」かつ「行列の先頭が自分」の2つ。
	** 2つの別データを同時に見るので、同じ lock の下で判定する必要がある */
	while (d->taken || heap_top(&d->waiters) != c->id)
	{
		if (is_stopped())
		{
			heap_remove(&d->waiters, c->id);
			pthread_mutex_unlock(&d->lock);
			return (1);
		}
		pthread_cond_wait(&d->cond, &d->lock);
	}
	d->taken = 1;
	heap_pop(&d->waiters);
	pthread_mutex_unlock(&d->lock);
	log_state(c->id, "has taken a dongle");
	return (0);
}

static void	release_one(t_dongle *d)
{
	pthread_mutex_lock(&d->lock);
	d->taken = 0;
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->lock);
}

static int	acquire_two(t_coder *c)
{
	t_dongle	*first;
	t_dongle	*second;
	long		key;

	/* ★ 全員が id の小さい方から取る。これで循環待ちが原理的に消える */
	first = c->left;
	second = c->right;
	if (second->id < first->id)
	{
		first = c->right;
		second = c->left;
	}
	key = now_us();	/* FIFO: 到着時刻。2本に同じキーを使い回す */
	if (acquire_one(first, c, key) != 0)
		return (1);
	if (second != first && acquire_one(second, c, key) != 0)
	{
		release_one(first);	/* ★ 持っている分は必ず返してから抜ける */
		return (1);
	}
	return (0);
}

/* ---------------- coder スレッド ---------------- */

static void	*coder_thread(void *arg)
{
	t_coder	*c;

	c = (t_coder *)arg;
	while (c->count < N_REQUIRED)
	{
		if (acquire_two(c) != 0)
			break ;
		pthread_mutex_lock(&c->state_lock);
		gettimeofday(&c->last_compile, NULL);
		pthread_mutex_unlock(&c->state_lock);
		log_state(c->id, "is compiling");
		usleep(T_COMPILE * 1000);
		pthread_mutex_lock(&c->state_lock);
		c->count++;
		pthread_mutex_unlock(&c->state_lock);
		release_one(c->left);
		if (c->right != c->left)
			release_one(c->right);
		if (is_stopped())
			break ;
		log_state(c->id, "is debugging");
		usleep(T_DEBUG * 1000);
	}
	return (NULL);
}

/* ---------------- monitor スレッド ---------------- */

/* 0 = 生存中 / 1 = burnout / 2 = 規定回数達成済み */
static int	check_one(t_coder *c)
{
	long	deadline;
	int		done;

	pthread_mutex_lock(&c->state_lock);
	done = (c->count >= N_REQUIRED);
	deadline = c->last_compile.tv_sec * 1000000L + c->last_compile.tv_usec
		+ T_BURNOUT * 1000L;
	pthread_mutex_unlock(&c->state_lock);
	if (done)
		return (2);
	if (now_us() >= deadline)
		return (1);
	return (0);
}

static void	*monitor_thread(void *arg)
{
	int	i;
	int	res;
	int	finished;

	(void)arg;
	while (1)
	{
		i = 0;
		finished = 0;
		while (i < N)
		{
			res = check_one(&g_c[i]);
			if (res == 1)
			{
				set_stopped();
				pthread_mutex_lock(&g_log_lock);
				printf("%ld %d burned out\n", elapsed_ms(), g_c[i].id);
				pthread_mutex_unlock(&g_log_lock);
				wake_all();
				return (NULL);
			}
			if (res == 2)
				finished++;
			i++;
		}
		if (finished == N)
			return (NULL);
		usleep(1000);
	}
}

/* ---------------- 組み立て ---------------- */

int	main(void)
{
	pthread_t	monitor;
	int			i;

	gettimeofday(&g_start, NULL);
	g_stopped = 0;
	pthread_mutex_init(&g_stop_lock, NULL);
	pthread_mutex_init(&g_log_lock, NULL);
	i = 0;
	while (i < N)
	{
		g_d[i].id = i + 1;
		g_d[i].taken = 0;
		g_d[i].waiters.size = 0;
		pthread_mutex_init(&g_d[i].lock, NULL);
		pthread_cond_init(&g_d[i].cond, NULL);
		i++;
	}
	i = 0;
	while (i < N)
	{
		g_c[i].id = i + 1;
		g_c[i].count = 0;
		g_c[i].last_compile = g_start;
		g_c[i].right = &g_d[i];
		g_c[i].left = &g_d[(i - 1 + N) % N];	/* リング配線 */
		pthread_mutex_init(&g_c[i].state_lock, NULL);
		i++;
	}
	i = 0;
	while (i < N)
	{
		pthread_create(&g_c[i].th, NULL, coder_thread, &g_c[i]);
		i++;
	}
	pthread_create(&monitor, NULL, monitor_thread, NULL);
	/* ★ 先に monitor を join。停止判断が出てから coder を回収する */
	pthread_join(monitor, NULL);
	i = 0;
	while (i < N)
	{
		pthread_join(g_c[i].th, NULL);
		i++;
	}
	i = 0;
	while (i < N)
	{
		pthread_mutex_destroy(&g_d[i].lock);
		pthread_cond_destroy(&g_d[i].cond);
		pthread_mutex_destroy(&g_c[i].state_lock);
		i++;
	}
	pthread_mutex_destroy(&g_log_lock);
	pthread_mutex_destroy(&g_stop_lock);
	return (0);
}
