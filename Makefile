# AstraLang build system
# Requires: gcc, g++, cargo (rustup)

CC       = gcc
CXX      = g++
CARGO    = $(HOME)/.cargo/bin/cargo
CFLAGS   = -Wall -Wextra -O2 -Iinclude
CXXFLAGS = -Wall -Wextra -O2 -std=c++17 -Iinclude

LEXER_OBJ    = build/lexer.o
COMPILER_BIN = build/astrac
DISASM_BIN   = build/astradis
RUNTIME_BIN  = runtime/target/release/astra

COMPILER_SRC = compiler/main.cpp \
               compiler/parser/parser.cpp \
               compiler/sema/sema.cpp \
               compiler/opt/optimizer.cpp \
               compiler/ir/codegen.cpp

.PHONY: all compiler runtime disasm clean run-hello run-structs test

all: compiler runtime disasm

# ---- Lexer object ----
$(LEXER_OBJ): lexer/lexer.c include/astra_token.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Compiler ----
$(COMPILER_BIN): $(LEXER_OBJ) $(COMPILER_SRC)
	$(CXX) $(CXXFLAGS) $(LEXER_OBJ) $(COMPILER_SRC) -o $@

compiler: $(COMPILER_BIN)

# ---- Disassembler ----
$(DISASM_BIN): tools/disasm.cpp include/astra_ir.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

disasm: $(DISASM_BIN)

# ---- Runtime ----
$(RUNTIME_BIN):
	$(CARGO) build --release --manifest-path runtime/Cargo.toml

runtime: $(RUNTIME_BIN)

# ---- Tests ----
run-hello: compiler runtime
	$(COMPILER_BIN) examples/hello.as -o build/hello.asc
	$(RUNTIME_BIN) build/hello.asc

run-structs: compiler runtime
	$(COMPILER_BIN) examples/structs.as -o build/structs.asc
	$(RUNTIME_BIN) build/structs.asc

test: compiler runtime disasm run-hello run-structs
	@echo "--- disassembly of hello.asc ---"
	$(DISASM_BIN) build/hello.asc
	@echo ""
	@echo "--- trace mode ---"
	$(RUNTIME_BIN) build/hello.asc --trace 2>&1 | head -30

# ---- Clean ----
clean:
	rm -rf build
	$(CARGO) clean --manifest-path runtime/Cargo.toml
