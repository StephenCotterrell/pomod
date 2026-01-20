#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  TIMER_MODE_POMODORO = 0,
  TIMER_MODE_STOPWATCH = 1,
} timer_mode_t;

typedef enum {
  TIMER_PHASE_FOCUS = 0,
  TIMER_PHASE_BREAK = 1,
  TIMER_PHASE_LONG_BREAK = 2,
  TIMER_PHASE_STOPWATCH = 3,
} timer_phase_t;

typedef enum {
  TIMER_STATE_STOPPED = 0,
  TIMER_STATE_RUNNING = 1,
  TIMER_STATE_PAUSED = 2,
} timer_state_t;

typedef struct {
  timer_mode_t mode;
  timer_phase_t phase;
  timer_state_t state;

  int focus_seconds;
  int break_seconds;
  int long_break_seconds;
  int cycles; // for long breaks every N focus cycles

  int completed_focus_sessions;

  int64_t started_at_ms;  // monotonic ms, valid when running
  int64_t accumulated_ms; // accumulated elapsed when not running
} pomod_timer_t;

void timer_init(pomod_timer_t *t);
void timer_set_mode(pomod_timer_t *t, timer_mode_t mode);
void timer_set_phase(pomod_timer_t *t, timer_phase_t);

void timer_start(pomod_timer_t *t, int64_t now_ms);
void timer_pause(pomod_timer_t *t, int64_t now_ms);
void timer_resume(pomod_timer_t *t, int64_t now_ms);
void timer_stop(pomod_timer_t *t);

void pomod_timer_tick(pomod_timer_t *t, int64_t now_ms);

int64_t timer_elapsed_ms(const pomod_timer_t *t, int64_t now_ms);
int64_t timer_remaining_ms(const pomod_timer_t *t, int64_t now_ms);
bool timer_is_complete(const pomod_timer_t *t, int64_t now_ms);
