# AstraLang Design Specification

Version: 1.0

Status: Living Design Document

Author: Frictionalfor

---

# 1. Introduction

## 1.1 Purpose

This document defines the architectural, semantic, and implementation principles of AstraLang.

Unlike the README, which explains how to build and use the language, this document explains:

* Why AstraLang exists
* How the language should evolve
* Which design principles are considered fundamental
* Which implementation decisions are intentional
* Which tradeoffs have been accepted

This document serves as the primary reference for future language development.

---

## 1.2 Vision

AstraLang is a systems programming language designed around a portable bytecode execution model.

The language aims to provide:

* Predictable execution
* Strong compile-time guarantees
* Portable deployment
* Future WebAssembly compatibility
* Low implementation complexity

AstraLang is not intended to compete directly with existing mature ecosystems.

Instead, it serves as a research and production-oriented language platform exploring modern compiler design through a custom virtual machine architecture.

---

# 2. Design Philosophy

## 2.1 Simplicity Over Cleverness

Every language feature introduces:

* parser complexity
* semantic complexity
* optimizer complexity
* runtime complexity

New features must justify their existence.

If a feature does not significantly improve:

* safety
* performance
* readability
* developer productivity

it should not be added.

---

## 2.2 Explicitness

AstraLang prefers explicit behavior.

Example:

```astra
let value: i64 = 42;
```

is preferred over:

```astra
let value = some_runtime_expression();
```

when type inference would reduce clarity.

The compiler should never surprise the programmer.

---

## 2.3 Compile-Time Validation

The compiler should reject invalid programs whenever possible.

Examples:

* type mismatches
* invalid assignments
* unreachable symbols
* invalid module imports
* incorrect function signatures

Errors discovered during compilation are cheaper than errors discovered during execution.

---

## 2.4 Deterministic Execution

Identical bytecode executed on identical VM versions should produce identical observable behavior.

This property simplifies:

* testing
* debugging
* deployment
* future browser execution

---

# 3. Language Model

## 3.1 Compilation Model

AstraLang follows:

```text
Source
 ↓
Lexing
 ↓
Parsing
 ↓
AST
 ↓
Semantic Analysis
 ↓
Optimization
 ↓
IR Generation
 ↓
Bytecode
 ↓
Virtual Machine
```

Each stage has clearly defined responsibilities.

No stage should perform work belonging to another stage.

---

## 3.2 Static Typing

AstraLang is statically typed.

Types are resolved during semantic analysis.

Type information must remain available until code generation.

Future type inference may reduce annotation requirements but must not weaken type safety.

---

## 3.3 Mutability

Variables are immutable by default.

Example:

```astra
let x: i64 = 5;
```

Mutation requires explicit declaration:

```astra
let mut x: i64 = 5;
```

The semantic analyzer must reject mutations of immutable variables.

---

## 3.4 Ownership and Memory

Current versions rely on VM-managed storage.

Future versions may introduce:

* ownership analysis
* borrow checking
* escape analysis

without breaking existing language semantics.

---

# 4. Compiler Architecture

## 4.1 Lexer

Responsibilities:

* tokenization
* keyword recognition
* literal decoding
* source location tracking

The lexer should remain stateless beyond token generation.

---

## 4.2 Parser

Parser implementation:

Pratt Parser

Reasons:

* compact implementation
* extensible precedence system
* good expression handling

Parser responsibilities:

* syntax validation
* AST generation

Parser must not:

* perform type checking
* resolve symbols
* generate bytecode

---

## 4.3 AST Design

The AST represents semantic intent.

Example:

```astra
let x: i64 = 5 + 3;
```

Should become:

```text
VariableDecl
 ├─ Name: x
 ├─ Type: i64
 └─ BinaryExpr(+)
     ├─ Integer(5)
     └─ Integer(3)
```

The AST should remain independent of VM implementation details.

---

## 4.4 Semantic Analysis

Responsibilities:

### Symbol Resolution

Resolve:

* variables
* functions
* modules
* types

### Type Checking

Validate:

* assignments
* expressions
* returns
* calls

### Scope Validation

Enforce:

* lexical scope
* shadowing rules
* visibility rules

### Mutability Enforcement

Reject:

```astra
let x = 5;
x = 10;
```

unless declared mutable.

---

## 4.5 Optimizer

Optimization must never alter program behavior.

Current passes:

* constant folding
* dead code elimination
* expression simplification

Future passes:

* inlining
* loop simplification
* strength reduction
* escape analysis

---

## 4.6 Code Generation

Responsibilities:

* bytecode emission
* function table construction
* constant pool generation

Compiler invariants:

* every call target is valid
* every function index exists
* entry point must exist
* bytecode must pass validation

Violation of any invariant is a compiler bug.

---

# 5. Virtual Machine Design

## 5.1 Goals

The VM acts as:

* execution backend
* debugging backend
* reference implementation

The VM prioritizes:

* correctness
* portability
* debuggability

over raw speed.

---

## 5.2 Execution Model

Stack-based architecture.

Advantages:

* compact bytecode
* simple interpreter
* easy debugging

Tradeoffs:

* slower than register VMs
* more instructions required

The simplicity tradeoff is intentional.

---

# 6. Bytecode Design

## 6.1 Design Goals

Bytecode must be:

* deterministic
* portable
* versioned
* forward evolvable

---

## 6.2 Compatibility Rules

Major version changes:

* may break compatibility

Minor version changes:

* must remain backward compatible

Runtime loaders must reject unsupported major versions.

---

## 6.3 Validation

Before execution:

* opcode validity checked
* function tables checked
* entrypoint checked
* constant pools checked

Invalid bytecode must never execute.

---

# 7. Error System

Compiler diagnostics are part of the language.

Every error must include:

* file
* line
* column
* category
* human-readable explanation

Preferred:

```text
error[E3004]

cannot assign value of type str
to variable of type i64

help: use a conversion function
```

---

# 8. Security Principles

The compiler and runtime must treat source code as untrusted input.

Requirements:

* no shell injection
* no unchecked file operations
* validated bytecode loading
* bounds-checked VM operations

Security bugs take priority over feature development.

---

# 9. Future Evolution

Potential future additions:

* generics
* type inference
* closures
* package manager
* language server
* LLVM backend
* WebAssembly backend

Features should be introduced through RFCs.

No feature enters the language without a documented rationale.

---

# 10. Non-Goals

AstraLang intentionally avoids:

* implicit coercion
* dynamic typing
* hidden allocations
* inheritance-heavy OOP
* runtime type guessing
* undefined behavior as a language feature

These exclusions are deliberate and define the identity of AstraLang.

---

# 11. Guiding Principle

If a future design decision conflicts with:

* simplicity
* explicitness
* compile-time safety
* deterministic execution

then the design decision is wrong.

The language exists to provide a predictable and understandable execution model rather than maximizing feature count.
