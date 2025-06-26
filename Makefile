
# directories
SRC_DIR := src
BLD_DIR := build
BIN_DIR := bin
TST_DIR := tests
DOC_DIR := docs


# compilation Flags
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -I$(SRC_DIR)
LDFLAGS := 


# files
CFILES := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BLD_DIR)/%.o, $(CFILES))


# rules

.DEFAULT_GOAL := all

all: help


.PHONY: help
help:
	@echo "Rules:"
	@echo "      compile -> build the memory allocator library"
	@echo "      test -> generate executables to test the library"
	@echo "      documentation -> generate documentation, with Doxygen"
	@echo "      clean -> clean unnecessary files"


.PHONY: setup
setup:
	@mkdir -p $(BLD_DIR) $(BIN_DIR)

# compile the memory allocator library
.PHONY: compile
compile: setup $(OBJS)


$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $^ -o $@ $(LDFLAGS)


# generate tests for the library
.PHONY: test
test:
	@echo "Generating tests..."


# generate documentation with doxygen
# firefox docs/html/index.html
.PHONY: documentation
documentation:
	@doxygen -q $(DOC_DIR)/Doxyfile


.PHONY: clean
clean:
	rm -rf $(BLD_DIR)
	rm -rf $(BIN_DIR)
