#include "timer.h"
#include <stdint.h>

void timer_init(pomod_timer_t *t) {
  *t = (pomod_timer_t){
      .mode = TIMER_MODE_POMODORO,
      .phase = TIMER_PHASE_FOCUS,
      .state = TIMER_STATE_STOPPED,
      .focus_seconds = 25 * 60,
      .break_seconds = 5 * 60,
      .long_break_seconds = 15 * 60,
      .cycles = 4,
  };
}

void timer_set_mode(pomod_timer_t *t, timer_mode_t mode) {
  t->mode = mode;
  t->phase = (mode == TIMER_MODE_STOPWATCH) ? TIMER_PHASE_STOPWATCH
                                            : TIMER_PHASE_FOCUS;
  t->state = TIMER_STATE_STOPPED;
  t->accumulated_ms = 0;
  t->started_at_ms = 0;
}

void timer_set_phase(pomod_timer_t *t, timer_phase_t phase) {
  t->phase = phase;
  t->state = TIMER_STATE_STOPPED;
  t->accumulated_ms = 0;
  t->started_at_ms = 0;
}

void timer_start(pomod_timer_t *t, int64_t now_ms) {
  t->state = TIMER_STATE_RUNNING;
  t->started_at_ms = now_ms;
}

void timer_pause(pomod_timer_t *t, int64_t now_ms) {
  if (t->state != TIMER_STATE_RUNNING)
    return;
  t->accumulated_ms += (now_ms - t->started_at_ms);
  t->state = TIMER_STATE_PAUSED;
}

void timer_resume(pomod_timer_t *t, int64_t now_ms) {
  if (t->state != TIMER_STATE_PAUSED)
    return;
  t->state = TIMER_STATE_RUNNING;
  t->started_at_ms = now_ms;
}

void timer_stop(pomod_timer_t *t) {
  t->state = TIMER_STATE_STOPPED;
  t->accumulated_ms = 0;
  t->started_at_ms = 0;
}

int64_t timer_elapsed_ms(const pomod_timer_t *t, int64_t now_ms) {
  if (t->state == TIMER_STATE_RUNNING) {
    return t->accumulated_ms + (now_ms - t->started_at_ms);
  }
  return t->started_at_ms;
}

static int phase_duration_seconds(const pomod_timer_t *t) {
  if (t->mode == TIMER_MODE_STOPWATCH)
    return 0;
  switch (t->phase) {
  case TIMER_PHASE_FOCUS:
    return t->focus_seconds;
  case TIMER_PHASE_BREAK:
    return t->break_seconds;
  case TIMER_PHASE_LONG_BREAK:
    return t->long_break_seconds;
  default:
    return t->focus_seconds;
  }
}

int64_t timer_remaining_ms(const pomod_timer_t *t, int64_t now_ms) {
  if (t->mode == TIMER_MODE_STOPWATCH)
    return 0;
  int64_t duration_ms = (int64_t)phase_duration_seconds(t) * 1000;
  int64_t remaining = duration_ms - timer_elapsed_ms(t, now_ms);
  return remaining < 0 ? 0 : remaining;
}

bool timer_is_complete(const pomod_timer_t *t, int64_t now_ms) {
  return (t->mode == TIMER_MODE_POMODORO) &&
         (timer_remaining_ms(t, now_ms) == 0);
}

void pomod_timer_tick(pomod_timer_t *t, int64_t now_ms) {
  (void)now_ms;
  (void)t;
}
