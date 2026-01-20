# Compiler
CC := cc

# Compiler flags
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g

# Preprocessor flags (header search paths)
CPPFLAGS := -Iinclude -I../include -D_POSIX_C_SOURCE=200809L

# Linker flags (unused for now)
LDFLAGS :=

# Libraries to link (add notcurses later)
LDLIBS :=

# Output binary name
BIN := pomod

# Dev Compile - suppress warnings 
RELAXED_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 -g

.PHONY: dev
dev: CFLAGS := $(RELAXED_CFLAGS)
dev: $(BIN)


# Source files
SRC := $(wildcard src/*.c)

# Object files (build/foo.o from src/foo.c)
OBJ := $(SRC:src/%.c=build/%.o)

# Default target
.PHONY: all
all: $(BIN)

# Link the final binary
$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Compile .c to .o
build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Ensure build directory exists
build:
	mkdir -p build

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf build $(BIN)
