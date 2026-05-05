CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
BUILD := build
SRC := src/main.c src/lexer.c src/parser.c src/ast.c src/codegen.c src/optimizer.c
BIN := $(BUILD)/z80cc

.PHONY: all clean test grammar
all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BIN): $(SRC) src/compiler.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRC)

grammar:
	bison -Wall -d -o src/parser_bison.c src/z80cc.y

test: $(BIN)
	@echo "No tests defined without examples."

clean:
	rm -rf $(BUILD)
