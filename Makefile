
# directories
SRC_DIR := src
BLD_DIR := build
BIN_DIR := bin
TST_DIR := tests
DOC_DIR := docs


# compilation Flags
CC := gcc
OPTS := -O0
CFLAGS := -Wall -Wextra -Wpedantic $(OPTS) -std=c99 -I$(SRC_DIR) -g -fsanitize=address -fsanitize=undefined -Wstrict-aliasing -Wcast-align -Wconversion -Wshadow
LDFLAGS := 

# release flags
# gcc -O2 -DNDEBUG -Wall -Wextra -Wpedantic


# files
CFILES := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BLD_DIR)/%.o, $(CFILES))

# test files
TEST_CFILES := $(wildcard $(TST_DIR)/*.c)
TEST_BINS := $(patsubst $(TST_DIR)/%.c, $(BIN_DIR)/%, $(TEST_CFILES))

# rules

.DEFAULT_GOAL := all

all: help


.PHONY: help
help:
	@echo "Rules:"
	@echo "  compile       -> build the memory allocator library"
	@echo "  test          -> generate executables to test the library"
	@echo "  documentation -> generate documentation, with Doxygen"
	@echo "  clean         -> clean unnecessary files"


.PHONY: setup
setup:
	@mkdir -p $(BLD_DIR) $(BIN_DIR)


# compile the memory allocator library
.PHONY: compile
compile: setup $(OBJS)

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $^ -o $@ $(LDFLAGS)


# test binaries
.PHONY: test
test: setup $(TEST_BINS)

$(BIN_DIR)/%: $(TST_DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)


# generate documentation with doxygen
# firefox docs/html/index.html
.PHONY: documentation
documentation:
	@doxygen -q $(DOC_DIR)/Doxyfile

# formates the code
.PHONY: fmt
fmt:
	@-clang-format -verbose -i $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(TST_DIR)/*.c
	@shfmt -w -i 2 -l -ci .


.PHONY: clean
clean:
	rm -rf $(BLD_DIR)
	rm -rf $(BIN_DIR)
