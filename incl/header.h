/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:48:41 by hkanamit          #+#    #+#             */
/*   Updated: 2026/09/09 15:30:00 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HEADER_H
# define HEADER_H

# include <pthread.h>
# include <sys/time.h>

/* ---- CLIから読み取った生の引数（parse段階でのみ使用） ---- */
typedef struct s_args
{
	int					num_coders;
	int					t_to_burnout;
	int					t_to_compile;
	int					t_to_debug;
	int					t_to_refactor;
	int					num_compile_req;
	int					dongle_cooldown;
	char				*scheduler;
}						t_args;

/* ---- スケジューリング方式 ---- */
typedef enum e_scheduler
{
	FIFO,
	EDF
}						t_scheduler;

/*
** dongleを要求している1件分の情報（FIFO/EDF用ヒープの要素）。
** priority_key はどちらの scheduler でもシミュレーション開始からの
** マイクロ秒で統一する。
** fifo: 要求が到着した時刻 / edf: last_compile_start_us + time_to_burnout
*/
typedef struct s_request
{
	int					coder_id;
	long				priority_key;
}						t_request;

/*
** 1本のdongleを要求しうるのは、隣り合う2人のcoderだけ（構造上それ以上は増えない）。
** そのため優先度キューは最大2要素の固定長配列で表現できる。
*/
typedef struct s_heap
{
	t_request			entries[2];
	int					size;
}						t_heap;

/* ---- dongleの状態 ---- */
typedef enum e_dongle_state
{
	D_FREE,
	D_TAKEN,
	D_COOLDOWN
}						t_dongle_state;

/* t_dongle / t_coder <-> t_shared が互いを参照するため前方宣言 */
typedef struct s_shared	t_shared;

/* release_time は cooldown 計算の起点（絶対時刻） */
typedef struct s_dongle
{
	int					id;
	pthread_mutex_t		lock;
	pthread_cond_t		cond;
	t_dongle_state		state;
	struct timeval		release_time;
	t_heap				waiters;
	t_shared			*shared;
}						t_dongle;

/*
** id は 1 〜 num_coders。
** state_lock は last_compile_start_us と compile_count を保護する。
** last_compile_start_us: 直近の compile 開始時刻（シミュレーション開始からのus）。
**                        burnout 判定と edf のキーの起点になる。
** blocked_on: いま待っている dongle（待っていなければ NULL）。state_lock で保護。
** compile_count: number_of_compiles_required の判定用。
** shared: 共有領域への逆参照。
*/
typedef struct s_coder
{
	int					id;
	pthread_t			thread;
	t_dongle			*left;
	t_dongle			*right;
	pthread_mutex_t		state_lock;
	t_dongle			*blocked_on;
	long				last_compile_start_us;
	int					compile_count;
	t_shared			*shared;
}						t_coder;

/*
** stopped は burnout 検知で 1 になる一度きりのフラグ。
** ロック順序の約束（どの経路もこれを破らないこと）:
**   1. dongle の lock は id の昇順で取る（f -> s）。
**   2. dongle の lock -> coder の state_lock。逆順で取らない。
**   3. stop_lock は常に最内側。握ったまま他のロックを取らない。
** log_lock と dongle の lock は同時に握らない（log_state は解放後に呼ぶ）。
** start_time はタイムスタンプ計算の基準（シミュレーション開始 = 0ms）。
*/
struct					s_shared
{
	int					num_coders;
	long				t_to_burnout;
	long				t_to_compile;
	long				t_to_debug;
	long				t_to_refactor;
	int					num_compile_req;
	long				dongle_cooldown;
	t_scheduler			scheduler;
	t_dongle			*dongles;
	t_coder				*coders;
	pthread_t			monitor;
	int					stopped;
	pthread_mutex_t		stop_lock;
	pthread_mutex_t		log_lock;
	struct timeval		start_time;
};

/* ---- dongle.c ---- */
int						init_dongles(t_shared *shared);
void					destroy_dongles(t_shared *shared);

/* ---- coder.c ---- */
int						init_coders(t_shared *shared);
void					destroy_coders(t_shared *shared);

/* ---- shared.c ---- */
int						init_shared(t_shared *shared, t_args *ins);
void					destroy_shared(t_shared *shared);

/* ---- codexion.c ---- */
int						run(t_args *ins);

/* ---- heap.c ---- */
void					heap_push(t_heap *h, int coder_id, long priority_key);
int						heap_top(t_heap *h);
void					heap_pop(t_heap *h);
void					heap_remove(t_heap *h, int coder_id);

/* ---- dongle_cooldown.c ---- */
struct timespec			cooldown_deadline(t_dongle *d, long cooldown_ms);
void					refresh_dongle_state(t_dongle *d, long cooldown_ms);
void					wait_on_blocker(t_dongle *d, t_coder *coder);

/* ---- timing.c ---- */
void					precise_sleep(long ms);

/* ---- coder_state.c ---- */
void					set_blocked_on(t_coder *coder, t_dongle *d);
int						can_be_passed_over(t_coder *coder, t_dongle *d);
void					release_reservations(t_dongle *d);

/* ---- dongle_pair.c ---- */
void					enqueue_both(t_coder *coder, t_dongle *f, t_dongle *s,
							long key);
void					dequeue_both(t_coder *coder, t_dongle *f, t_dongle *s);
t_dongle				*try_take_pair(t_coder *coder, t_dongle *f,
							t_dongle *s);

/* ---- dongle_acquire.c ---- */
int						acquire_two_dongles(t_coder *coder);

/* ---- dongle_release.c ---- */
void					release_one_dongle(t_dongle *d);
void					release_two_dongles(t_coder *coder);

/* ---- log.c ---- */
long					elapsed_us(t_shared *shared);
long					elapsed_ms(t_shared *shared);
void					log_state(t_shared *shared, int coder_id,
							const char *msg);
void					log_burnout(t_shared *shared, int coder_id);

/* ---- stop.c ---- */
int						is_stopped(t_shared *shared);
void					set_stopped(t_shared *shared);
void					wake_all_dongles(t_shared *shared);
void					wait_until_stopped(t_dongle *d, t_shared *shared);

/* ---- coder_routine.c ---- */
void					*coder_thread(void *arg);

/* ---- monitor.c ---- */
void					*monitor_thread(void *arg);

#endif
