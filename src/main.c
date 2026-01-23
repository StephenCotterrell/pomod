#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

int main(int argc, char **argv) {
  int verbose = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0) {
      verbose = 1;
    }
  }

  pomod_timer_t t;
  timer_init(&t);

  int64_t start = monotonic_ms();
  timer_start(&t, start);

  while (1) {
    int64_t now = monotonic_ms();
    pomod_timer_tick(&t, now);

    int64_t elapsed = timer_elapsed_ms(&t, now);
    int64_t remaining = timer_remaining_ms(&t, now);

    int rem_sec = (int)(remaining / 1000);
    int el_sec = (int)(elapsed / 1000);
    int rem_min = rem_sec / 60;
    int rem_s = rem_sec % 60;

    if (verbose) {
      printf("remaining=%02d:%02d elapsed=%ds phase=%s state=%s\n", rem_min,
             rem_s, el_sec, phase_str(t.phase), state_str(t.state));
    } else {
      printf("\rremaining=%02d:%02d elapsed=%ds phase=%s state=%s   ", rem_min,
             rem_s, el_sec, phase_str(t.phase), state_str(t.state));
      fflush(stdout);
    }

    if (timer_is_complete(&t, now)) {
      break;
    }

    struct timespec one_sec = {.tv_sec = 1, .tv_nsec = 0};
    nanosleep(&one_sec, NULL);
  }

  printf("\nDone. Press any key to exit.\n");
  getchar();

  timer_pause(&t, monotonic_ms());
  printf("Paused. \n");

  timer_resume(&t, monotonic_ms());
  printf("Resumed. \n");

  timer_stop(&t);
  printf("Timer stopped. \n");
}
