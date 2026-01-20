#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "timer.h"

static int64_t monotonic_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    return 0;
  }
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static const char *state_str(timer_state_t s) {
  switch (s) {
  case TIMER_STATE_RUNNING:
    return "RUNNING";
  case TIMER_STATE_PAUSED:
    return "PAUSED";
  case TIMER_STATE_STOPPED:
    return "BREAK";
  default:
    return "UNKNOWN";
  }
}

static const char *phase_str(timer_phase_t p) {
  switch (p) {
  case TIMER_PHASE_FOCUS:
    return "FOCUS";
  case TIMER_PHASE_BREAK:
    return "BREAK";
  case TIMER_PHASE_LONG_BREAK:
    return "LONG_BREAK";
  case TIMER_PHASE_STOPWATCH:
    return "STOPWATCH";
  default:
    return "UNKNOWN";
  }
}

int main(void) {
  pomod_timer_t t;
  timer_init(&t);

  int64_t start = monotonic_ms();
  timer_start(&t, start);

  for (int i = 0; i < 10; i++) {
    int64_t now = monotonic_ms();
    pomod_timer_tick(&t, now);

    int64_t elapsed = timer_elapsed_ms(&t, now);
    int64_t remaining = timer_remaining_ms(&t, now);

    printf("[%s] phase=%s elapsed=%lldms remaining=%lldms focus_done=%d\n",
           state_str(t.state), phase_str(t.phase), (long long)elapsed,
           (long long)remaining, t.completed_focus_sessions);

    struct timespec one_sec = {.tv_sec = 1, .tv_nsec = 0};
    nanosleep(&one_sec, NULL);
  }

  timer_pause(&t, monotonic_ms());
  printf("Paused. \n");

  timer_resume(&t, monotonic_ms());
  printf("Resumed. \n");

  timer_stop(&t);
  printf("Timer stopped. \n");
}
