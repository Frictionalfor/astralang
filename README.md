# AstraLang

A systems programming language designed for low-level execution with a long-term path toward WebAssembly and browser integration.

AstraLang is implemented entirely in C, C++, and Rust. It compiles source files to a custom bytecode format (.asc) which is executed by a Rust-based virtual machine.

---

## Project Status

Version: 1.2.0
Bytecode: v1.2 (stable)
Platform: Linux

---

## Architecture

```
source (.as)
    |
    v
Lexer          [C]       lexer/lexer.c
    |
    v
Parser         [C++]     compiler/parser/
    |
    v
Semantic       [C++]     compiler/sema/
    |
    v
Optimizer      [C++]     compiler/opt/
    |
    v
IR Codegen     [C++]     compiler/ir/
    |
    v
Bytecode (.asc)          build/*.asc
    |
    v
Runtime VM     [Rust]    runtime/src/vm.rs
    |
    v
Execution output
```

---

## Project Structure

```
astralang/
├── include/
│   ├── astra_token.h       Token definitions (shared C/C++)
│   ├── astra_ir.h          Bytecode IR format and opcode table
│   └── astra_error.h       Unified error and diagnostic system
├── lexer/
│   └── lexer.c             Tokenizer (C)
├── compiler/
│   ├── main.cpp            Unified CLI entry point (astrac)
│   ├── error.cpp           Diagnostic printer
│   ├── ast/ast.h           AST node definitions
│   ├── parser/             Pratt expression parser
│   ├── sema/               Semantic analysis and type checking
│   ├── opt/                Optimizer (constant folding, dead code)
│   └── ir/                 IR code generator
├── runtime/
│   └── src/
│       ├── main.rs         Runtime entry point
│       ├── vm.rs           Stack-based virtual machine
│       ├── loader.rs       Bytecode loader with version checking
│       └── debugger.rs     Interactive bytecode debugger
├── tools/
│   └── disasm.cpp          Bytecode disassembler (astradis)
├── stdlib/
│   ├── std_math.as         Math module
│   ├── std_io.as           I/O module
│   └── std_string.as       String module
├── examples/
│   ├── hello.as            Hello world and basic features
│   └── structs.as          Structs, modules, recursion
└── Makefile
```

---

## Building

Requirements: gcc, g++, cargo (rustup)

```bash
# Build everything
make all

# Build compiler only
make compiler

# Build runtime only
make runtime

# Build disassembler only
make disasm
```

---

## Usage

### Compiler CLI

```bash
# Build a source file to bytecode
build/astrac build examples/hello.as -o build/hello.asc

# Build with debug info
build/astrac build examples/hello.as --debug

# Build without optimizer
build/astrac build examples/hello.as --no-opt

# Type-check only (no output)
build/astrac check examples/hello.as

# Compile and run immediately
build/astrac run examples/hello.as

# Disassemble bytecode
build/astrac disasm build/hello.asc

# Print version
build/astrac version
```

### Runtime

```bash
# Execute bytecode
runtime/target/release/astra build/hello.asc

# Execute with instruction trace
runtime/target/release/astra build/hello.asc --trace

# Execute with custom stack limit
runtime/target/release/astra build/hello.asc --stack-limit 4096

# Execute with interactive debugger
runtime/target/release/astra build/hello.asc --debug
```

### Disassembler

```bash
build/astradis build/hello.asc
```

---

## Language Syntax

### Variables

```astra
let x: i64 = 10;
let mut counter: i64 = 0;
let name: str = "astra";
```

### Functions

```astra
fn add(a: i64, b: i64) -> i64 {
    return a + b;
}

fn main() -> void {
    let result: i64 = add(10, 32);
    println(result);
}
```

### Control Flow

```astra
if x > 0 {
    println("positive");
} else {
    println("non-positive");
}

while counter < 10 {
    counter = counter + 1;
}
```

### Structs

```astra
struct Point {
    x: i64,
    y: i64,
}
```

### Modules

```astra
mod math {
    pub fn factorial(n: i64) -> i64 {
        if n <= 1 { return 1; }
        return n * factorial(n - 1);
    }
}

fn main() -> void {
    let result: i64 = math::factorial(10);
    println(result);
}
```

### Types

| Type   | Description              |
|--------|--------------------------|
| i8     | 8-bit signed integer     |
| i16    | 16-bit signed integer    |
| i32    | 32-bit signed integer    |
| i64    | 64-bit signed integer    |
| u8     | 8-bit unsigned integer   |
| u16    | 16-bit unsigned integer  |
| u32    | 32-bit unsigned integer  |
| u64    | 64-bit unsigned integer  |
| f32    | 32-bit float             |
| f64    | 64-bit float             |
| bool   | boolean                  |
| str    | string                   |
| void   | no return value          |
| *T     | pointer to T             |
| [T; N] | fixed array of T         |

---

## Bytecode Format (.asc)

```
[4 bytes]  magic: "ASTR"
[4 bytes]  version: u32 (major << 16 | minor)
[4 bytes]  flags: u32
[4 bytes]  string pool count
[...]      string pool entries: [u32 length][bytes]
[4 bytes]  function count
[...]      function entries
[4 bytes]  entry function index
```

Each instruction is 24 bytes:

```
[4 bytes]  opcode (u32)
[4 bytes]  reserved (must be 0)
[8 bytes]  operand_a (i64)
[8 bytes]  operand_b (i64)
```

### Bytecode Flags

| Flag        | Value | Meaning                        |
|-------------|-------|--------------------------------|
| NONE        | 0x00  | No flags                       |
| DEBUG       | 0x01  | Debug info present             |
| OPTIMIZED   | 0x02  | Compiled with optimizer        |
| STDLIB      | 0x04  | Standard library module        |

### Versioning

The bytecode version follows major.minor encoding. The runtime checks the major version on load. A mismatch causes a hard error. Minor version increments are backward compatible.

Current version: 1.2

---

## Debugger

Start the runtime with `--debug` to enter the interactive debugger.

```
[dbg] main:0000 > help

  s/step       - step one instruction
  c/continue   - run until next breakpoint
  b <fi> <ip>  - set breakpoint at function index and instruction
  del <fi> <ip>- remove breakpoint
  bp           - list all breakpoints
  stack        - print value stack
  locals       - print local variables
  bt           - print call stack
  q/quit       - exit
```

---

## Optimizer

The optimizer runs as an AST pass before IR generation. It performs:

- Constant folding: `2 + 3` becomes `5` at compile time
- Dead code elimination: `if false { ... }` is removed entirely
- Expression simplification: `x * 1`, `x + 0`, `x && true` are reduced
- Unary folding: `-5`, `!true` evaluated at compile time

Disable with `--no-opt` flag.

---

## Error System

Errors are reported in structured format:

```
ERROR [300] examples/hello.as:12:5 - undefined symbol 'foo'
WARN  [301] examples/hello.as:8:3  - type mismatch in assignment
```

Error code ranges:

| Range | Category         |
|-------|------------------|
| 1xx   | Lexer errors     |
| 2xx   | Parser errors    |
| 3xx   | Semantic errors  |
| 4xx   | Codegen errors   |
| 5xx   | Runtime errors   |
| 6xx   | Loader errors    |

---

## Standard Library

The standard library is written in AstraLang itself with extern bridges to the runtime.

| Module         | Contents                              |
|----------------|---------------------------------------|
| std::math      | abs, min, max, pow, clamp             |
| std::io        | println_str, println_i64, println_f64 |
| std::string    | len, concat, slice, eq, contains      |

---

## Running the Examples

```bash
# Hello world
build/astrac build examples/hello.as -o build/hello.asc
runtime/target/release/astra build/hello.asc

# Structs and recursion
build/astrac build examples/structs.as -o build/structs.asc
runtime/target/release/astra build/structs.asc

# Or use make
make run-hello
make run-structs
```

---

## Implementation Stack

| Component         | Language | Location              |
|-------------------|----------|-----------------------|
| Lexer             | C        | lexer/lexer.c         |
| Parser            | C++      | compiler/parser/      |
| Semantic analysis | C++      | compiler/sema/        |
| Optimizer         | C++      | compiler/opt/         |
| IR code generator | C++      | compiler/ir/          |
| Compiler CLI      | C++      | compiler/main.cpp     |
| Disassembler      | C++      | tools/disasm.cpp      |
| Virtual machine   | Rust     | runtime/src/vm.rs     |
| Bytecode loader   | Rust     | runtime/src/loader.rs |
| Debugger          | Rust     | runtime/src/debugger.rs |

---

## Roadmap

Phase 1 (complete)
- Working compiler and runtime
- Custom bytecode format
- Optimizer
- Disassembler
- Trace mode
- Debugger

Phase 2 (planned)
- Full struct field access in codegen
- Array type support
- Standard library compilation pipeline
- Error recovery in parser
- Source maps for runtime errors

Phase 3 (future)
- LLVM backend option
- WebAssembly compilation target
- Async/await execution model
- Package manager
- Language server protocol support

---

## License

AstraLang is a personal systems project. All rights reserved.
