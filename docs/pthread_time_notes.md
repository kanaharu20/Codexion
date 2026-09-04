# pthread / 時間関数 学習ノート（codexion）

このプロジェクトで使う POSIX スレッド関数と時間関数のまとめ。

---

## 目次

- [Part 1: スレッド管理](#part-1-スレッド管理)
- [Part 2: Mutex（排他制御）](#part-2-mutex排他制御)
- [Part 3: 条件変数](#part-3-条件変数)
- [Part 4: 時間関連](#part-4-時間関連)
- [Part 5: codexion での使い分け早見表](#part-5-codexion-での使い分け早見表)

共通ルール: **pthread系の関数は失敗時に `-1` ではなく、エラー番号そのものを返す**（`errno` を見るのではなく戻り値を見る）。時間系（`gettimeofday`/`clock_gettime`/`usleep`）は逆に `-1` を返して `errno` をセットする。

---

## Part 1: スレッド管理

### `pthread_create`

```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg);
```

| 引数 | 意味 |
|---|---|
| `thread` | 生成したスレッドIDの書き込み先 |
| `attr` | 属性（スタックサイズ等）。通常 `NULL` |
| `start_routine` | 実行する関数。**シグネチャは `void *f(void *)` 固定** |
| `arg` | 渡す引数（1個のみ）。複数渡したいときは構造体のポインタ |

- 呼んだ瞬間に新スレッドが**非同期に**動き出す。`pthread_create` 自体は完了を待たずすぐ戻る
- スレッドが終わるのは、`start_routine` が `return` する / `pthread_exit()` / プロセス終了のいずれか

**ハマりどころ**: ループで `&tmp` のように**単一変数を使い回して渡すと全スレッドが同じアドレスを見る**。配列にして `&args[i]` を渡す。

```c
// codexion: src/codexion.c
pthread_create(&shared->coders[i].thread, NULL, coder_thread, &shared->coders[i]);
```

### `pthread_join`

```c
int pthread_join(pthread_t thread, void **retval);
```

- 指定スレッドが終了するまで**ブロック**し、リソースを回収する
- `retval` に `void**` を渡すとスレッドの戻り値を受け取れる。不要なら `NULL`
- `pthread_create` と**必ずペア**（`malloc`/`free` と同じ感覚）

**呼ばないとどうなるか**: ①終了済みスレッドの管理情報が残り続ける（ゾンビスレッド）、②`main` が先に終わるとプロセスごと消え、動作中のスレッドが強制終了する。

**注意**: スタック上のローカル変数へのポインタを `return` してはいけない（スレッド終了でスタックが消える）。

---

## Part 2: Mutex（排他制御）

### `pthread_mutex_init`

```c
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
```

- **メモリ確保はしない**。すでに確保済みの領域の中身をセットアップするだけ
- `attr` は通常 `NULL`（デフォルト属性）
- 静的に1個だけなら `pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;` でも代用可

**ルール**: `init` を呼ばずに `lock` してはいけない。二重 `init` もNG。`malloc` した領域なら **`free` の前に必ず `destroy`**。

### `pthread_mutex_lock` / `pthread_mutex_unlock`

```c
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

- `lock`: 空いていれば即座に取得、取られていれば**ブロック**して待つ
- `unlock`: 解放。**lock したのと同じスレッドが呼ぶ必要がある**（他スレッドが unlock すると `EPERM`）
- `lock`〜`unlock` の区間がクリティカルセクション（一度に1スレッドだけ）

**ハマりどころ**:
1. **lock区間はできるだけ短く**。`printf` や `usleep` を握ったまま入れない
2. **unlockし忘れ**（特にエラー時の早期 `return`）＝そのmutexが永久ロック
3. デフォルト属性では**同じスレッドが二重にlockするとデッドロック**

```c
// NG: エラー時にunlockし忘れる
pthread_mutex_lock(&d->lock);
if (err)
    return (-1);          // ← unlockされない
pthread_mutex_unlock(&d->lock);
```

### `pthread_mutex_destroy`

```c
int pthread_mutex_destroy(pthread_mutex_t *mutex);
```

- `init` と1対1で対応する後始末。**メモリ自体は解放しない**
- **unlocked状態でなければならない**（ロック中に呼ぶと `EBUSY`）
- 順序が重要: **全スレッドを `pthread_join` で回収してから** `destroy` する

---

## Part 3: 条件変数

### 【重要】「条件」はどこにあるのか

`pthread_cond_t` は**条件式を一切持っていない**。持っているのは「この条件変数で寝ているスレッドの待ち行列」だけ。呼び鈴のようなもので、「荷物が届いたか」は知らない。

| 要素 | 正体 | 誰が用意するか |
|---|---|---|
| `d->state`（フラグ） | **条件そのもの（predicate）** | 自分で定義した普通の変数 |
| `while (...)` | 条件を**判定する文** | 自分で書く |
| `d->cond` | 条件が変化したことを**通知する仕組み** | pthreadライブラリ |
| `d->lock` | 上記フラグを保護するmutex | 自分で用意し、`cond_wait` にも渡す |

`pthread_cond_wait` は「lockを外して寝て、起きたらlockを取り直して**判定を呼び出し側に投げ返す**」だけ。`pthread_cond_signal` も共有データを読み書きしない。**条件を実際に変化させる代入は、signalを呼ぶ側が自分で書く**。

### `pthread_cond_init` / `pthread_cond_destroy`

```c
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
```

- mutex版とまったく同じ考え方（メモリ確保しない、`init`/`destroy` は1対1、`free` 前に `destroy`）
- `destroy` は**誰も待機中でない状態**で呼ぶ（待機中だと `EBUSY`）
- `broadcast` を送っただけでは「もう誰も待っていない」保証にならない。**必ず `join` で完全終了を確認してから** `destroy`

### `pthread_cond_wait`

```c
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
```

内部で以下を**アトミックに**行う:

1. `mutex` を**アンロック**する
2. `signal`/`broadcast` が来るまで**スリープ**
3. 起きたら**同じmutexを再ロックしてから**戻る

**必ず `while` で囲む（`if` は誤り）**:

```c
pthread_mutex_lock(&d->lock);
while (!d->available)                  // ← if ではなく while
    pthread_cond_wait(&d->cond, &d->lock);
d->available = 0;
pthread_mutex_unlock(&d->lock);
```

理由は2つ:
1. **spurious wakeup**: signalが来ていなくても稀に戻ってくることが仕様上許容されている
2. **横取り**: `broadcast` で全員起きても、実際に条件を満たせるのは1人だけ

### `pthread_cond_timedwait`

```c
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                            const struct timespec *abstime);
```

- `pthread_cond_wait` と同じ動作＋**期限付き**
- 期限までに起こされなければ **`ETIMEDOUT`** を返す（これはエラーではなく想定内の戻り方）
- **`abstime` は相対時間ではなく「絶対時刻」**。「今」を取得してから加算する

```c
struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);
ts.tv_sec  += ms / 1000;
ts.tv_nsec += (ms % 1000) * 1000000L;
if (ts.tv_nsec >= 1000000000L)        // ← 繰り上がり処理必須
{
    ts.tv_sec  += 1;
    ts.tv_nsec -= 1000000000L;
}
```

**注意**: デフォルトのクロックは `CLOCK_REALTIME`。`abstime` を作るときも同じ `CLOCK_REALTIME` を使って揃える（`CLOCK_MONOTONIC` を使うなら `pthread_condattr_setclock` で条件変数側の設定も変える必要がある）。

### `pthread_cond_signal` / `pthread_cond_broadcast`

```c
int pthread_cond_signal(pthread_cond_t *cond);     // 1人だけ起こす
int pthread_cond_broadcast(pthread_cond_t *cond);  // 全員起こす
```

**使い分けの基準**: 条件の変化によって**複数のスレッドが同時に先に進める可能性があるか**。

| 状況 | 使う関数 |
|---|---|
| dongleが1本返却された | `signal`（空いた枠は1つ） |
| 終了フラグを立てた | `broadcast`（全員が反応すべき） |
| 誰が実際に進めるかコードで決められない | `broadcast`（各自の `while` 再判定に任せる） |

**注意点**:
- **signalは記憶されない**。誰も待っていないときに送っても何も起きず、後から `wait` に入った人は取りこぼす。だから「共有フラグ＋mutex＋条件変数」の三点セットが必須
- mutexを**lockした状態で呼ぶ**のが安全（条件の変更とsignalの間に隙間を作らない）
- どのスレッドが起きるかは**実装依存**。FIFO/EDFのような公平性を実現するには、自前の優先度キューで「自分の番か」を `while` 条件に含める必要がある

---

## Part 4: 時間関連

### `gettimeofday`

```c
int gettimeofday(struct timeval *tv, struct timezone *tz);   // tz は常に NULL
```

```c
struct timeval { time_t tv_sec; suseconds_t tv_usec; };   // 秒 + マイクロ秒
```

**経過ミリ秒の計算**:

```c
gettimeofday(&now, NULL);
sec_diff  = now.tv_sec  - start.tv_sec;
usec_diff = now.tv_usec - start.tv_usec;
return (sec_diff * 1000 + usec_diff / 1000);
```

**`usec_diff` がマイナスになるのは正常**。例: `start=100.900000` / `now=101.100000`（実経過200ms）なら
`sec_diff = 1`、`usec_diff = 100000 - 900000 = -800000` → `1*1000 + (-800) = 200ms` で正しく出る。
秒差を先に1000倍することで繰り下がりが自動的に吸収される。

### `clock_gettime`

```c
int clock_gettime(clockid_t clk_id, struct timespec *tp);
```

```c
struct timespec { time_t tv_sec; long tv_nsec; };   // 秒 + ナノ秒
```

| `clk_id` | 意味 |
|---|---|
| `CLOCK_REALTIME` | システム実時計（`gettimeofday` と同性質） |
| `CLOCK_MONOTONIC` | 起動からの経過。後戻りしない |

`gettimeofday` との違いは、①ナノ秒精度、②時計の種類を選べる、の2点。
**`pthread_cond_timedwait` に渡す `struct timespec` をそのまま作れる**ので、この用途ではこちらが便利。

### `usleep`

```c
int usleep(useconds_t usec);   // ★ マイクロ秒
```

- 指定時間スレッドを止める。その間CPUは使わない
- `pthread_cond_wait` との違い: **誰かに起こしてもらうのではなく、時間が経てば自動的に起きる**

**ハマりどころ**: subjectの引数は**ミリ秒**、`usleep` は**マイクロ秒**。

```c
usleep(shared->t_to_compile * 1000);   // ms → us の変換を忘れない
```

POSIX.1-2008以降は非推奨（本来は `nanosleep` 推奨）だが、**この課題では許可関数リストにあるので `usleep` を使う**（`nanosleep` は許可されていない）。

### 単位まとめ

| 単位 | 1秒あたり | 使う構造体/関数 |
|---|---|---|
| ミリ秒 (ms) | 1,000 | subjectの引数、ログのタイムスタンプ |
| マイクロ秒 (us) | 1,000,000 | `struct timeval.tv_usec`、`usleep` |
| ナノ秒 (ns) | 1,000,000,000 | `struct timespec.tv_nsec` |

---

## Part 5: codexion での使い分け早見表

| 用途 | 使うもの | 理由 |
|---|---|---|
| ログのタイムスタンプ | `gettimeofday` | `start_time` も `timeval` で統一 |
| `pthread_cond_timedwait` の `abstime` | `clock_gettime(CLOCK_REALTIME, ...)` | 型が `timespec` で一致 |
| compile/debug/refactor の待機 | `usleep` | 単純な時間経過待ち |
| dongleが空くのを待つ | `pthread_cond_wait` | 他スレッドの解放イベント待ち |
| cooldown明けを待つ | `pthread_cond_timedwait` | 誰もsignalしてくれない（時間経過のみ）ため期限が必要 |
| dongle解放の通知 | `pthread_cond_broadcast` | 誰が進めるかは各自の再判定に任せる |
| ログ出力の直列化 | `pthread_mutex_t log_lock` | 行が混ざらないようにする |

### 関数の対応関係

| ペア | 関係 | 誰と誰 |
|---|---|---|
| `pthread_create` ↔ `pthread_join` | スレッド1つにつき1回ずつ | main ↔ 各worker |
| `pthread_mutex_init` ↔ `pthread_mutex_destroy` | 変数1つにつき1回ずつ | main（両方） |
| `pthread_cond_init` ↔ `pthread_cond_destroy` | 変数1つにつき1回ずつ | main（両方） |
| `pthread_mutex_lock` ↔ `pthread_mutex_unlock` | 同じスレッドが対で（何度も） | 呼んだスレッド自身 |
| `cond_wait`/`timedwait` ↔ `signal`/`broadcast` | **別スレッド間の非同期な関係** | 待つ側 ↔ 起こす側 |

最後の行だけが「異なるスレッド同士の関係」。ここが条件変数の難しさの本質で、
「先にsignalが来てしまう」「誰も待っていない」ケースがあるため、`while` 再判定と共有フラグが必須になる。

### 起動から終了までの流れ

```
[初期化]  main単独            mutex_init / cond_init（部分失敗時は巻き戻し）
    ↓
[起動]    pthread_create      coder × N（+ monitor）
    ↓
[稼働]    各スレッド並行       lock → cond_wait/timedwait → unlock → usleep のループ
    ↓
[終了]    pthread_join        全スレッド回収を確認してから
    ↓
[片付け]  mutex_destroy / cond_destroy / free
```
