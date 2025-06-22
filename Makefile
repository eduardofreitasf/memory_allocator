# Compilation Flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 # -fsanitize=address
LDFLAGS = 

# Files
CFILES = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(CFILES))
BIN = mem_alloc

# Rules

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN)