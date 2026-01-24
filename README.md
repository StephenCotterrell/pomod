# Pomodoro CLI tool

A terminal-based Pomodoro timer with a notcurses UI, keyboard controls, and
optional session summaries.

## Build

```sh
make
```

## Run

```sh
./pomod
```

### Flags

- `--basic`: basic single-line output
- `--verbose`: verbose per-tick output

## Controls (notcurses mode)

- `space`: pause/resume
- `n`: skip to next phase
- `i`: enter note input mode
- `enter`: save note
- `esc`: cancel note input
- `q`: quit and print summary

## Notes

When a phase ends (timeout, skip, or session end), it is recorded in the
session summary printed on exit.
