*This project has been created as part of the 42 curriculum by hkanamit.*

# Codexion

## Description

Codexion is a concurrency simulation written in C with POSIX threads.

A number of coders sit around a circular co-working hub. Between every pair of
neighbours lies one USB dongle, so there are exactly as many dongles as coders.
A coder must hold **both** the dongle on their left and the one on their right to
compile. After compiling they release both dongles, debug, refactor, and
immediately try to compile again.

Two clocks run against them:

* **Burnout** — if a coder does not *start* a new compile within
  `time_to_burnout` milliseconds of the start of their previous compile (or of
  the start of the simulation), they burn out and the simulation stops.
* **Cooldown** — a released dongle stays unusable for `dongle_cooldown`
  milliseconds before anyone may take it again.

Each coder is a thread. Each dongle is a small state machine
(`D_FREE` / `D_TAKEN` / `D_COOLDOWN`) guarded by its own mutex and condition
variable, with a per-dongle priority queue that arbitrates between the two
coders that can possibly want it. A separate monitor thread watches every
coder's deadline and stops the simulation the moment one is missed.

The simulation ends when either

* a coder burns out (`<timestamp> <id> burned out` is printed), or
* every coder has compiled at least `number_of_compiles_required` times.

### Log format

```
<timestamp_in_ms> <coder_id> has taken a dongle
<timestamp_in_ms> <coder_id> is compiling
<timestamp_in_ms> <coder_id> is debugging
<timestamp_in_ms> <coder_id> is refactoring
<timestamp_in_ms> <coder_id> burned out
```

Timestamps are milliseconds elapsed since the start of the simulation.

## Instructions

### Build

```sh
make          # builds ./codexion
make clean    # removes obj/
make fclean   # removes obj/ and the binary
make re       # fclean + all
```

Compiled with `cc -Wall -Wextra -Werror -pthread`. Sources live in `src/`, the
single header in `incl/`, objects are produced into `obj/`.

### Run

```sh
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

| Argument | Meaning |
|---|---|
| `number_of_coders` | number of coders, and of dongles (≥ 1) |
| `time_to_burnout` | ms; deadline measured from the start of the last compile |
| `time_to_compile` | ms spent compiling, holding two dongles |
| `time_to_debug` | ms spent debugging |
| `time_to_refactor` | ms spent refactoring |
| `number_of_compiles_required` | stop once every coder reached this count (≥ 1) |
| `dongle_cooldown` | ms a dongle stays unavailable after release |
| `scheduler` | exactly `fifo` or `edf` |

All eight arguments are mandatory. Each numeric argument must be a string of
decimal digits only, so negative numbers and non-integers are rejected before
parsing; the conversion also refuses values that would overflow `int`. Anything
invalid prints `Error` on stderr and exits with status 1.

Examples:

```sh
# every coder reaches 5 compiles and the program ends on its own
./codexion 5 800 100 100 100 5 0 edf
./codexion 4 600 200 100 100 5 0 fifo

# same parameters, different arbitration: FIFO lets a coder burn out
# almost immediately, EDF keeps everyone alive far longer
./codexion 4 410 200 100 100 5 0 fifo
./codexion 4 410 200 100 100 5 0 edf

# one coder, one dongle: the second dongle can never be taken,
# so this always ends in a burnout
./codexion 1 800 200 200 100 5 0 fifo
```

## Resources

* `man` pages: `pthread_create`, `pthread_join`, `pthread_mutex_lock`,
  `pthread_cond_wait`, `pthread_cond_timedwait`, `pthread_cond_broadcast`,
  `gettimeofday`, `clock_gettime`, `usleep`
* *The Little Book of Semaphores*, Allen B. Downey — the dining philosophers
  chapter, which this project is a variation of
* Coffman et al., *System Deadlocks* (1971) — the four conditions for deadlock,
  used here as the checklist for the "Blocking cases handled" section
* Michael Kerrisk, *The Linux Programming Interface*, chapters 29–30 (threads,
  mutexes, condition variables)
* Butenhof, *Programming with POSIX Threads* — condition-variable idioms, in
  particular why a wait must always sit inside a `while` loop
* Liu & Layland (1973) on Earliest Deadline First scheduling

### How AI was used

AI (Claude) was used as a study aid and a reviewer, not as a code generator:

* **Learning the primitives.** The standalone programs in `docs/pthread_examples/`
  (`01_create_join`, `02_mutex`, `03_cond`, `04_timedwait`, `05_mini_codexion`)
  were built up with AI assistance to understand `pthread_create`/`join`, mutexes,
  condition variables and `pthread_cond_timedwait` in isolation before any of
  them were used in the real project. `05_mini_codexion.c` is a deliberately
  simplified sketch of the final design.
* **Timing semantics.** `docs/pthread_time_notes.md` collects notes on
  `gettimeofday` vs `clock_gettime`, `struct timeval` vs `struct timespec`, and
  the unit conversions needed for `pthread_cond_timedwait`, written while
  clarifying those points with AI.
* **Reviewing invariants.** AI was used to question the lock ordering, the
  placement of the stop check inside `log_lock`, and the microsecond-vs-millisecond
  rounding in the monitor. The conclusions were re-derived and verified against
  the code before being applied.

All source files under `src/` and `incl/` were written and are fully understood
by the author.

## Blocking cases handled

### Deadlock (Coffman's four conditions)

A dongle is a genuinely exclusive resource that must be held while compiling, so
*mutual exclusion* and *hold and wait* cannot be removed. *No preemption* is also
kept: a dongle is never taken back from a coder that holds it. The design
therefore attacks the fourth condition, **circular wait**.

`order_by_id` in `src/dongle_acquire.c` sorts the two dongles a coder needs by
their `id` and always acquires the lower id first. Because every thread requests
resources in one global order, no cycle of "coder A holds 1 waits for 2, coder B
holds 2 waits for 1" can form, and the classic dining-philosophers deadlock is
impossible by construction rather than by timing luck.

### Starvation

Each dongle owns a priority queue (`t_heap`) of the coders currently waiting for
it. A dongle sits between exactly two neighbouring coders, and no one else can
ever request it, so the waiter set is provably bounded at two entries. The
priority queue is therefore a fixed-size two-element array rather than a
dynamically sized binary heap: `heap_push` restores the ordering invariant with a
single comparison and swap in constant time, and `heap_top`, `heap_pop` and
`heap_remove` preserve it. It is a hand-written priority queue with the same
contract as a heap, sized to the only input it can ever receive.

A coder that wants a dongle pushes itself into that queue and then waits until it
is *both* the top of the queue and the dongle is free:

```c
while (!(d->state == D_FREE && heap_top(&d->waiters) == coder->id))
```

Arbitration follows the `scheduler` argument:

* **`fifo`** — the key is the arrival timestamp in microseconds, so requests are
  served in arrival order.
* **`edf`** — the key is `last_compile_start + time_to_burnout`, i.e. the coder's
  burnout deadline, so the coder closest to burning out is served first. This is
  what gives liveness under feasible parameters: a coder that has been waiting
  long has an early deadline and is therefore promoted.

The key is sampled **once** per compile attempt in `snapshot_priority_key` and
reused for both dongles, so a coder cannot lose its place between the first and
the second acquisition.

**Tie-breaker.** `heap_push` swaps the two entries only on a strict `<`
comparison, so when two keys are exactly equal the request that was already in
the queue stays on top. Ties are therefore broken by insertion order,
deterministically.

### Cooldown

`release_one_dongle` sets the state to `D_COOLDOWN`, records `release_time`, and
broadcasts. A waiter that sees `D_COOLDOWN` does not spin: it computes the exact
expiry with `cooldown_deadline` and sleeps on `pthread_cond_timedwait` until that
absolute time, then calls `refresh_dongle_state`, which flips `D_COOLDOWN` back
to `D_FREE` once the deadline has passed. The state is refreshed lazily, always
under `d->lock`, so no separate timer thread is needed.

### Precise burnout detection

`monitor_thread` polls every coder every 1 ms. The comparison is done in
**microseconds**, not milliseconds: rounding the deadline down to milliseconds
would move it up to 1 ms earlier and cause a false burnout. Reading
`last_compile_start` and `compile_count` is done under the coder's `state_lock`,
so the monitor never observes a half-updated coder. With a 1 ms poll interval the
`burned out` line is printed well inside the 10 ms tolerance required by the
subject.

A coder that has already reached `number_of_compiles_required` is excluded from
the deadline check, because it will never start another compile and would
otherwise be reported as burnt out.

### Log serialization and the last line

Every state line is printed while holding `log_lock`, so two messages can never
interleave. More importantly, `log_state` re-checks the stop flag **inside**
`log_lock`:

```c
pthread_mutex_lock(&shared->log_lock);
if (!is_stopped(shared))
    printf("%ld %d %s\n", ts, coder_id, msg);
pthread_mutex_unlock(&shared->log_lock);
```

Checking before taking the lock would leave a window in which a coder passes the
check, the monitor prints `burned out`, and the coder then prints a state line
after it. Doing the check inside the same critical section makes `burned out` the
last line of the output.

### Single coder

With one coder, `left == right`: only one dongle exists on the table and the
second acquisition can never succeed. This is handled explicitly rather than
deadlocking — `acquire_two_dongles` detects `second == first`, and the coder
parks in `wait_until_stopped` holding its single dongle until the monitor detects
the inevitable burnout and wakes it. The program then terminates normally.

### Shutdown

`set_stopped` alone is not enough: coders blocked in `pthread_cond_wait` would
never re-evaluate it. `wake_all_dongles` broadcasts on every dongle's condition
variable after the flag is set, so every sleeping coder wakes, sees
`is_stopped`, removes itself from the waiter queue, releases whatever it holds,
and returns. `main` can then join every thread and free all memory.

The same pair (`set_stopped` + `wake_all_dongles`) is used on the error paths in
`run` and `spawn_coders`, so a failed `pthread_create` also unwinds cleanly
instead of leaving threads blocked forever.

## Thread synchronization mechanisms

### Threads

* `number_of_coders` coder threads (`coder_thread`), one per coder.
* One monitor thread (`monitor_thread`).
* The main thread creates them, joins the monitor first, then joins the coders,
  and finally destroys every mutex/condition variable and frees the two arrays.

There are no global variables. Everything shared lives in a single `t_shared`
allocated on `main`'s stack and reached through `coder->shared`.

### Primitives

| Primitive | Where | Protects |
|---|---|---|
| `pthread_mutex_t lock` | one per `t_dongle` | dongle state, `release_time`, waiter queue |
| `pthread_cond_t cond` | one per `t_dongle` | waiting for the dongle to become free or its cooldown to expire |
| `pthread_mutex_t state_lock` | one per `t_coder` | `last_compile_start`, `compile_count` |
| `pthread_mutex_t log_lock` | one, in `t_shared` | stdout, and the stop check that guards it |
| `pthread_mutex_t stop_lock` | one, in `t_shared` | the `stopped` flag |

### The custom event: the stop flag

The project needs a one-shot "the simulation is over" event that any thread can
observe. It is built from a plain `int stopped` plus `stop_lock`, exposed only
through `is_stopped` / `set_stopped`, so the flag is never read or written
outside its mutex — there is no unsynchronized shared variable anywhere in the
program.

Because a flag on its own cannot wake a blocked thread, it is paired with
`wake_all_dongles`, which broadcasts on every dongle condition variable. Setting
the flag and broadcasting is the "signal" half of the event; the `while` loops in
`acquire_one` and `wait_until_stopped` are the "wait" half.

### Lock ordering

The single rule, documented in `incl/header.h` and `src/stop.c`, is:

```
d->lock / log_lock  →  stop_lock
```

`stop_lock` is always the innermost lock and is never held while acquiring
anything else. `is_stopped` is therefore safe to call from inside a dongle
critical section (`acquire_one`, `wait_until_stopped`) and from inside the log
critical section (`log_state`), and no path can produce an inverted order.
Dongle locks are never nested: `acquire_one` fully releases one dongle's lock
before taking the next one's, so only the id ordering — not lock nesting — is
what prevents deadlock.

### Preventing race conditions

* **Duplicated dongles.** The transition to `D_TAKEN` and the `heap_pop` happen
  while `d->lock` is held, in the same critical section as the test that allowed
  them. Two coders can never both conclude the dongle is free.
* **Spurious wakeups.** Every wait sits inside a `while` loop that re-tests the
  full predicate, never an `if`, so a spurious or broadcast-induced wakeup simply
  re-checks and sleeps again.
* **Torn coder state.** `last_compile_start` is written by the coder and read by
  the monitor; `compile_count` likewise. Both are always accessed under
  `state_lock`, and the monitor copies what it needs and releases the lock before
  doing any comparison, so it never holds `state_lock` while blocking.
* **Interleaved or late output.** See "Log serialization and the last line"
  above.

### Coder ↔ monitor communication

The two directions are deliberately asymmetric and both are one-way:

* **Coder → monitor:** the coder publishes `last_compile_start` and
  `compile_count` under `state_lock`. It never notifies the monitor; the monitor
  polls at 1 ms, which is what keeps burnout detection within the required 10 ms.
* **Monitor → coders:** the monitor sets the stop flag, prints the burnout line,
  and broadcasts on every dongle. Coders observe the flag either at the top of a
  wait loop or right after releasing their dongles, and exit.

Neither side ever blocks waiting for the other, so the monitor can always meet
its deadline, and a coder can always terminate.
