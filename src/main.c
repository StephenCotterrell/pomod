#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "timer.h"

#define NOTE_MAX 80

static struct notcurses *nc;

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

static const char *state_str(timer_state_t s) {
  switch (s) {
  case TIMER_STATE_RUNNING:
    return "RUNNING";
  case TIMER_STATE_PAUSED:
    return "PAUSED";
  case TIMER_STATE_STOPPED:
    return "STOPPED";
  default:
    return "UNKNOWN";
  }
}

static int get_key_nonblocking(void) {
  struct ncinput ni;
  uint32_t key = notcurses_get_nblock(nc, &ni);
  if (key == 0)
    return -1;
  return key;
}

typedef struct {
  bool input_mode;
  char note[NOTE_MAX];
  char edit_buf[NOTE_MAX];
  int edit_len;
} note_state_t;

static void note_begin(note_state_t *ns) {
  ns->input_mode = true;
  ns->edit_len = (int)strnlen(ns->note, NOTE_MAX - 1);
  memcpy(ns->edit_buf, ns->note, (size_t)ns->edit_len);
  ns->edit_buf[ns->edit_len] = '\0';
}

static void note_cancel(note_state_t *ns) {
  ns->input_mode = false;
  ns->edit_len = 0;
  ns->edit_buf[0] = '\0';
}

static void note_commit(note_state_t *ns) {
  strncpy(ns->note, ns->edit_buf, NOTE_MAX - 1);
  ns->note[NOTE_MAX - 1] = '\0';
  ns->input_mode = false;
}

static void note_handle_key(note_state_t *ns, int key) {
  if (key == 27) { // ESC
    note_cancel(ns);
    return;
  }
  if (key == '\n' || key == '\r') { // ENTER
    note_commit(ns);
    return;
  }
  if (key == 127 || key == 8) {
    if (ns->edit_len > 0) {
      ns->edit_len--;
      ns->edit_buf[ns->edit_len] = '\0';
    }
  }
  if (key >= 32 && key <= 126) {
    if (ns->edit_len < NOTE_MAX - 1) {
      ns->edit_buf[ns->edit_len++] = (char)key;
      ns->edit_buf[ns->edit_len] = '\0';
    }
  }
}

typedef enum {
  PHASE_ADVANCE_TIMEOUT = 0,
  PHASE_ADVANCE_SKIP = 1
} phase_advance_reason_t;

static void reset_elapsed(pomod_timer_t *t, int64_t now_ms) {
  t->accumulated_ms = 0;
  t->started_at_ms = now_ms;
  t->state = TIMER_STATE_RUNNING;
}

static void log_phase_end(const pomod_timer_t *t, int64_t elapsed_ms,
                          const char *note, phase_advance_reason_t reason) {
  int sec = (int)(elapsed_ms / 1000);
  int min = sec / 60;
  sec = sec % 60;
  printf("\nphase_end=%s duration=%02d:%02d note=\"%s\" reason=%s\n",
         phase_str(t->phase), min, sec, note && note[0] ? note : "",
         reason == PHASE_ADVANCE_SKIP ? "skip" : "timeout");
}

static void advance_phase(pomod_timer_t *t, int64_t now_ms, note_state_t *ns,
                          phase_advance_reason_t reason) {
  int64_t elapsed = timer_elapsed_ms(t, now_ms);
  log_phase_end(t, elapsed, ns->note, reason);
  ns->note[0] = '\0';

  if (t->phase == TIMER_PHASE_FOCUS) {
    t->completed_focus_sessions++;
    if (t->completed_focus_sessions % t->cycles == 0) {
      t->phase = TIMER_PHASE_LONG_BREAK;
    } else {
      t->phase = TIMER_PHASE_BREAK;
    }
  } else {
    t->phase = TIMER_PHASE_FOCUS;
  }
  reset_elapsed(t, now_ms);
}

static int64_t monotonic_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    return 0;
  }
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main(int argc, char **argv) {
  int verbose = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0) {
      verbose = 1;
    }
  }

  struct notcurses_options opts = {0};
  nc = notcurses_init(&opts, NULL);
  if (!nc) {
    fprintf(stderr, "failed to init notcurses\n");
    return 1;
  }

  pomod_timer_t t;
  timer_init(&t);

  int64_t start = monotonic_ms();
  timer_start(&t, start);

  note_state_t note_state = {0};

  while (1) {
    int64_t now = monotonic_ms();

    if (t.state == TIMER_STATE_RUNNING && !note_state.input_mode) {
      pomod_timer_tick(&t, now);
      if (t.mode == TIMER_MODE_POMODORO && timer_remaining_ms(&t, now) == 0) {
        advance_phase(&t, now, &note_state, PHASE_ADVANCE_TIMEOUT);
      }
    }

    int key = get_key_nonblocking();

    if (note_state.input_mode) {
      note_handle_key(&note_state, key);
      if (!note_state.input_mode) {
        t.state = TIMER_STATE_RUNNING;
      }
    } else {
      if (key == ' ') {
        if (t.state == TIMER_STATE_RUNNING)
          timer_pause(&t, now);
        else if (t.state == TIMER_STATE_PAUSED)
          timer_resume(&t, now);
      } else if (key == 'n' || key == 'N') {
        advance_phase(&t, now, &note_state, PHASE_ADVANCE_SKIP);
      } else if (key == 'i' || key == 'I') {
        note_begin(&note_state);
        t.state = TIMER_STATE_PAUSED;
      }
    }

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

    struct timespec one_sec = {.tv_sec = 1, .tv_nsec = 0};
    nanosleep(&one_sec, NULL);
  }

  printf("\nDone. Press any key to exit.\n");
  notcurses_stop(nc);
  getchar();
}
