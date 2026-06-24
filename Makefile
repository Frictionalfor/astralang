# AstraLang build system — v2.0
# Requires: gcc, g++, cargo (rustup)

CC       = gcc
CXX      = g++
CARGO    = $(HOME)/.cargo/bin/cargo
CFLAGS   = -Wall -Wextra -O2 -Iinclude
CXXFLAGS = -Wall -Wextra -O2 -std=c++17 -Iinclude

LEXER_OBJ    = build/lexer.o
OPCODE_OBJ   = build/opcode_table.o
ERROR_OBJ    = build/error.o
COMPILER_BIN = build/astrac
DISASM_BIN   = build/astradis
RUNTIME_BIN  = runtime/target/release/astra

COMPILER_SRC = compiler/main.cpp \
               compiler/parser/parser.cpp \
               compiler/sema/sema.cpp \
               compiler/opt/optimizer.cpp \
               compiler/ir/codegen.cpp \
               compiler/module/resolver.cpp

COMPILER_OBJS = $(LEXER_OBJ) $(OPCODE_OBJ) $(ERROR_OBJ)

.PHONY: all compiler runtime disasm clean run-hello run-structs test regression opcodes

all: compiler runtime disasm

# ---- C objects ----
$(LEXER_OBJ): lexer/lexer.c include/astra_token.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(OPCODE_OBJ): compiler/opcode_table.c include/astra_opcode_table.h include/astra_ir.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(ERROR_OBJ): compiler/error.cpp include/astra_error.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---- Compiler ----
$(COMPILER_BIN): $(COMPILER_OBJS) $(COMPILER_SRC)
	$(CXX) $(CXXFLAGS) $(COMPILER_OBJS) $(COMPILER_SRC) -o $@

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

# ---- Opcode table dump ----
opcodes: compiler
	@echo "--- AstraLang Opcode Table ---"
	@build/astrac opcodes

# ---- Tests ----
run-hello: compiler runtime
	build/astrac build examples/hello.as -o build/hello.asc
	$(RUNTIME_BIN) build/hello.asc

run-structs: compiler runtime
	build/astrac build examples/structs.as -o build/structs.asc
	$(RUNTIME_BIN) build/structs.asc

test: compiler runtime disasm
	@echo "=== hello ==="
	build/astrac build examples/hello.as -o build/hello.asc
	$(RUNTIME_BIN) build/hello.asc
	@echo ""
	@echo "=== structs ==="
	build/astrac build examples/structs.as -o build/structs.asc
	$(RUNTIME_BIN) build/structs.asc
	@echo ""
	@echo "=== check ==="
	build/astrac check examples/hello.as
	build/astrac check examples/structs.as
	@echo ""
	@echo "=== version ==="
	build/astrac version
	$(RUNTIME_BIN) --version
	@echo ""
	@echo "=== regression ==="
	bash tests/regression.sh

regression: compiler runtime disasm
	bash tests/regression.sh

# ---- Clean ----
clean:
	rm -rf build
	$(CARGO) clean --manifest-path runtime/Cargo.toml
