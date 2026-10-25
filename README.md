# Zynk
*this is my interpreted programming language!*

## Docs

Networking lib: [Net](libs/net.md)


Syntax: [Syntax Documentation (Spanish)](docs/syntax.md)


Stdlib : [Built-ins Documentation (Spanish)](docs/globals.md) 



## High-Level Architecture Overview

```
Source Code (.z)
      │
      ▼
┌───────────┐
│   Lexer   │  (lexer.c / mbtype.c)
└─────┬─────┘  Converts raw source code into structured tokens.
      │
      ▼
┌───────────┐
│  Parser   │  (parser.c)
└─────┬─────┘  Builds an Abstract Syntax Tree (AST) using recursive descent.
      │
      ▼
┌───────────┐
│ Evaluator │  (eval.c)
└─────┬─────┘  Executes AST nodes with dynamic typing & GC tracking.
      │
      ▼
┌───────────┐
│ Built-ins │  (globals.c)
└───────────┘  Provides native standard functions, dynamic libraries (.so), & I/O.

```

---

### Core Components Summary

#### 1. Entry Point (`zynk.c`)

* Handles CLI execution, loading the source file into memory.
* Initializes the global environment context (`EvalCtx`) and registers standard native functions.
* Sequences execution: **Lexing $\rightarrow$ Parsing $\rightarrow$ Evaluating $\rightarrow$ Cleanup**.

#### 2. Lexical Analyzer / Lexer (`lexer.c` & `mbtype.c`)

* Performs tokenization on the source string.
* Handles identifiers, numerical literals, escaped string literals (`"..."`, `'...'`), comments (`#`), operators (`<=`, `>=`, `!=`, `==`), and control-flow keywords (`let`, `fn`, `if`, `while`, `for`, `class`, `return`, `do`, `done`, etc.).
* Custom character utilities in `mbtype.c` support standalone/kernel-space execution constraints (like `k_isspace`, `k_isalpha`).

#### 3. Recursive Descent Parser (`parser.c`)

* Constructs the Abstract Syntax Tree (**AST**) out of token arrays.
* Implements operator precedence parsing for arithmetic, logical, and binary comparisons (`+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>`, `&&`, `||`).
* Parses key language constructs:
* **Variable Declarations & Assignments:** `let x = ...`
* **Functions & Classes:** `fn name { ... }`, `class Name { ... }`
* **Control Flow:** `if condition then ... else ... fi`, `do { ... } while condition`, `for (...) do ... done`
* **Member & Method Invocation:** `obj:method()` / bracket access.



#### 4. Tree-Walking Interpreter & Runtime (`eval.c`)

* Evaluates AST nodes recursively based on the execution context (`EvalCtx`).
* **Dynamic Type System (`ShellValue`):** Supports numbers, strings, arrays, dictionaries/classes, function objects, and native C function pointers (`CFunc`).
* **Memory Management & Garbage Collector:** Uses reference counting (`refc`) combined with a cleanup routine (`zynk_gc()`) that periodically runs every $N$ evaluation cycles (`CYCLES_PER_SWEEP`).
* **Scope & Variables:** Variable lookup uses hash-table indexing (`zhash`) across parent and global scope hierarchies.

#### 5. Native Standard Library & Extensions (`globals.c`)

* **Memory Tracker:** Tracks allocated AST nodes and tokens to prevent memory leaks during script execution.
* **Dynamic Plugin Support:** `include("plugin.so")` loads C shared objects at runtime using `dlopen()` and registers library functions via `dlsym()`.
* **Built-in Standard Library:**
* File I/O: `read()`, `write()`
* Terminal I/O: `input()`, `echo`
* Built-in Data Structures: Dynamic array object builder (`Array`) with native `push`, `pop`, and `len` methods.
* Script imports: `import("other_script.z")` parses and executes another file as an isolated instance.
