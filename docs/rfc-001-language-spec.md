# AstraLang RFC-001 — Language Specification v1

Status: Stable
Version: 1.0
Applies to: AstraLang compiler v2.0+, bytecode v1.2+

---

## 1. Lexical Rules

### 1.1 Source encoding

Source files use UTF-8 encoding. Only ASCII identifiers are supported in v1.

### 1.2 Whitespace

Spaces, tabs, carriage returns, and newlines are whitespace. Whitespace is not significant except as a token separator.

### 1.3 Comments

```
// single line comment
/* multi-line comment */
```

Nested block comments are not supported.

### 1.4 Identifiers

```
identifier = [a-zA-Z_][a-zA-Z0-9_]*
```

Identifiers are case-sensitive. Keywords are reserved and cannot be used as identifiers.

### 1.5 Keywords

```
let  mut  fn  return  if  else  while  for  in
struct  mod  use  pub  extern  true  false  null
as  async  await
```

### 1.6 Primitive type keywords

```
i8  i16  i32  i64  u8  u16  u32  u64  f32  f64  bool  str  void  ptr
```

### 1.7 Integer literals

Decimal only in v1. No underscores, no hex, no binary.

```
integer_literal = [0-9]+
```

### 1.8 Float literals

```
float_literal = [0-9]+ '.' [0-9]+ ([eE][+-]?[0-9]+)?
```

### 1.9 String literals

Enclosed in double quotes. Escape sequences: `\n \t \r \\ \"`.

---

## 2. Type System

### 2.1 Primitive types

| Type  | Width  | Range                                      |
|-------|--------|--------------------------------------------|
| i8    | 8-bit  | -128 to 127                                |
| i16   | 16-bit | -32768 to 32767                            |
| i32   | 32-bit | -2147483648 to 2147483647                  |
| i64   | 64-bit | -9223372036854775808 to 9223372036854775807|
| u8    | 8-bit  | 0 to 255                                   |
| u16   | 16-bit | 0 to 65535                                 |
| u32   | 32-bit | 0 to 4294967295                            |
| u64   | 64-bit | 0 to 18446744073709551615                  |
| f32   | 32-bit | IEEE 754 single precision                  |
| f64   | 64-bit | IEEE 754 double precision                  |
| bool  | 1-bit  | true or false                              |
| str   | ref    | immutable UTF-8 string reference           |
| void  | 0-bit  | no value (function return only)            |

### 2.2 Pointer types

```
*T   — pointer to T
```

Pointer arithmetic is not defined in v1. Pointers are used only with alloc/free.

### 2.3 Array types

```
[T; N]   — fixed-size array of N elements of type T
[T]      — dynamic array (heap allocated)
```

### 2.4 Type inference

Variable declarations without explicit type annotation infer the type from the initializer expression. If no initializer is present, an explicit type annotation is required.

### 2.5 Type coercion

No implicit coercion. All type conversions require an explicit `as` cast.

```astra
let x: i64 = 10;
let f: f64 = x as f64;
```

### 2.6 Null

`null` is a valid value for pointer types only. Dereferencing null is a runtime error (ASTRA_ERR_NULL_DEREF).

---

## 3. Expressions

### 3.1 Operator precedence (high to low)

| Level | Operators                        | Associativity |
|-------|----------------------------------|---------------|
| 14    | () [] .                          | left          |
| 12    | as                               | left          |
| 11    | * / %                            | left          |
| 10    | + -                              | left          |
| 9     | << >>                            | left          |
| 8     | < > <= >=                        | left          |
| 7     | == !=                            | left          |
| 6     | &                                | left          |
| 5     | ^                                | left          |
| 4     | |                                | left          |
| 3     | &&                               | left          |
| 2     | ||                               | left          |
| 1     | = += -= *= /=                    | right         |

### 3.2 Short-circuit evaluation

`&&` and `||` use short-circuit evaluation. The right operand is not evaluated if the result is determined by the left operand.

### 3.3 Assignment

Assignment is an expression that returns the assigned value. The left-hand side must be a mutable variable.

---

## 4. Statements

### 4.1 Variable declaration

```astra
let name: Type = expr;
let mut name: Type = expr;
let name = expr;          // type inferred
```

### 4.2 Function declaration

```astra
fn name(param: Type, ...) -> ReturnType {
    body
}

pub fn name(...) -> ReturnType { ... }
extern fn name(...) -> ReturnType;
async fn name(...) -> ReturnType { ... }
```

### 4.3 Control flow

```astra
if expr { block } else { block }
if expr { block } else if expr { block }

while expr { block }

for var in expr { block }
```

### 4.4 Return

```astra
return;
return expr;
```

A function with return type `void` may use bare `return` or fall off the end of the block.

---

## 5. Module System

### 5.1 Module declaration

```astra
mod name {
    // items
}
```

### 5.2 Import

```astra
use std::math;
use std::io;
use mymodule;
use pkg::submodule;
```

### 5.3 Visibility

Items are private by default. The `pub` keyword makes an item visible outside its module.

### 5.4 Qualified names

Items in a module are accessed with `::` syntax:

```astra
math::factorial(10)
std::io::println_str("hello")
```

### 5.5 Circular imports

Circular module dependencies are a compile-time error.

---

## 6. Memory Model

### 6.1 Stack allocation

All local variables and function parameters are stack-allocated. Stack frames are created on function entry and destroyed on function return.

### 6.2 Heap allocation

Heap allocation is explicit via the `alloc` built-in (future: via stdlib). The programmer is responsible for calling `free`. In v1, there is no garbage collector.

### 6.3 Ownership

AstraLang v1 does not enforce ownership or borrowing at the language level. Memory safety is the programmer's responsibility for heap-allocated values.

### 6.4 String semantics

String values are immutable references to constant data in the string pool. String concatenation produces a new heap-allocated string.

---

## 7. Execution Semantics

### 7.1 Entry point

Every program must define a `fn main() -> void` function. Execution begins there.

### 7.2 Evaluation order

Expressions are evaluated left-to-right. Function arguments are evaluated left-to-right before the call.

### 7.3 Integer overflow

Integer overflow is undefined behavior in v1. The runtime does not check for overflow.

### 7.4 Division by zero

Integer division by zero is a runtime error (ASTRA_ERR_DIV_BY_ZERO). Float division by zero produces infinity per IEEE 754.

### 7.5 Stack overflow

Exceeding the stack depth limit is a runtime error (ASTRA_ERR_STACK_OVERFLOW). Default limit: 8192 frames.

---

## 8. Built-in Functions

The following functions are available without import:

| Function    | Signature              | Description              |
|-------------|------------------------|--------------------------|
| println     | (value: any) -> void   | print value + newline    |
| print       | (value: any) -> void   | print value              |

---

## 9. Async (Reserved)

The `async` and `await` keywords are reserved for future use. They are parsed but not executed in v1. Emitting async code is a compile-time warning.

---

## 10. Undefined Behavior

The following are undefined behavior in AstraLang v1:

- Integer overflow
- Use of uninitialized memory
- Dereferencing a freed pointer
- Calling a function with wrong argument count (caught at compile time)
- Accessing array out of bounds (runtime error in v2)
