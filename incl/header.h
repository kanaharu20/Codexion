/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hkanamit <hkanamit@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 13:48:41 by hkanamit       #+#    #+#             */
/*   Updated: 2026/08/28 13:50:51 by hkanamit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HEADER_H
#define HEADER_H

#include <pthread.h>
#include <sys/time.h>

/* ---- CLIから読み取った生の引数（parse段階でのみ使用） ---- */
typedef struct {
    int num_coders;
    int t_to_burnout;
    int t_to_compile;
    int t_to_debug;
    int t_to_refactor;
    int num_compile_req;
    int dongle_cooldown;
    char *scheduler;
} args;

/* ---- スケジューリング方式 ---- */
typedef enum e_scheduler
{
    FIFO,
    EDF
}   t_scheduler;

/* ---- dongleを要求している1件分の情報（FIFO/EDF用ヒープの要素） ---- */
typedef struct s_request
{
    int     coder_id;
    long    priority_key;   /* fifo: 到着時刻(us) / edf: last_compile_start + t_to_burnout */
}   t_request;

/*
** 1本のdongleを要求しうるのは、隣り合う2人のcoderだけ（構造上それ以上は増えない）。
** そのため優先度キューは最大2要素の固定長配列で表現できる。
*/
typedef struct s_heap
{
    t_request   entries[2];
    int         size;
}   t_heap;

/* ---- dongleの状態 ---- */
typedef enum e_dongle_state
{
    D_FREE,
    D_TAKEN,
    D_COOLDOWN
}   t_dongle_state;

typedef struct s_dongle
{
    int             id;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    t_dongle_state  state;
    struct timeval  release_time;  /* cooldown計算の起点 */
    t_heap          waiters;
}   t_dongle;

/* t_coder <-> t_shared が互いを参照するため前方宣言 */
typedef struct s_shared t_shared;

typedef struct s_coder
{
    int             id;                  /* 1 〜 num_coders */
    pthread_t       thread;
    t_dongle        *left;
    t_dongle        *right;

    pthread_mutex_t state_lock;          /* 以下2つのフィールドを保護 */
    struct timeval  last_compile_start;  /* burnout計算の起点 */
    int             compiling;           /* 今compile中か（monitorのburnout判定除外用） */

    int             compile_count;       /* number_of_compiles_required 判定用 */
    t_shared        *shared;             /* 共有領域への逆参照 */
}   t_coder;

struct s_shared
{
    int             num_coders;
    long            t_to_burnout;
    long            t_to_compile;
    long            t_to_debug;
    long            t_to_refactor;
    int             num_compile_req;
    long            dongle_cooldown;
    t_scheduler     scheduler;

    t_dongle        *dongles;            /* malloc(num_coders * sizeof(t_dongle)) */
    t_coder         *coders;             /* malloc(num_coders * sizeof(t_coder)) */

    int             stopped;             /* burnout または全員達成で1になる */
    pthread_mutex_t stop_lock;
    pthread_cond_t  stop_cond;

    pthread_mutex_t log_lock;

    struct timeval  start_time;          /* タイムスタンプ計算の基準（シミュレーション開始=0ms） */
};

/* ---- dongle.c ---- */
int     init_dongles(t_shared *shared);
void    destroy_dongles(t_shared *shared);

/* ---- coder.c ---- */
int     init_coders(t_shared *shared);
void    destroy_coders(t_shared *shared);

/* ---- shared.c ---- */
int     init_shared(t_shared *shared, args *ins);
void    destroy_shared(t_shared *shared);

/* ---- codexion.c ---- */
int     run(args *ins);

/* ---- heap.c ---- */
void    heap_push(t_heap *h, int coder_id, long priority_key);
int     heap_top(t_heap *h);
void    heap_pop(t_heap *h);
void    heap_remove(t_heap *h, int coder_id);

/* ---- dongle_cooldown.c ---- */
long            timeval_to_ms(struct timeval *tv);
struct timespec cooldown_deadline(t_dongle *d, long cooldown_ms);
void            refresh_dongle_state(t_dongle *d, long cooldown_ms);

/* ---- dongle_acquire.c ---- */
void    acquire_two_dongles(t_coder *coder);

/* ---- dongle_release.c ---- */
void    release_two_dongles(t_coder *coder);

/* ---- log.c ---- */
long    elapsed_ms(t_shared *shared);
void    log_state(t_shared *shared, int coder_id, const char *msg);

/* ---- coder_routine.c ---- */
void    *coder_thread(void *arg);

#endif