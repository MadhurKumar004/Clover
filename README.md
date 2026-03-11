# Clover — LLVM-backed Compiler

Clover is a C-like language, performs semantic analysis, and emits LLVM IR.
(I am trying to keep it low level and thinking of making it performance oriented)

## Current functionality

- Lexer, parser, and AST construction
- Semantic analysis (type checking, basic symbol resolution)
- LLVM IR code generation (prints module IR to stdout)
- Supported features:
	- Built-in integer and floating types, casts
	- `const` bindings (immutability enforced by semantic pass)
	- `struct` declarations and field access
	- `extern` declarations to call C functions
	- Control flow: `if` / `elif` / `else`, `do` (do-while), `loop`, `break`, `continue`
	- Pointers, address-of (`&`) and deref (`*`) inside `unsafe` contexts
	- Arithmetic and bitwise operators
	- Basic testing via `lli` or by producing object files and linking with `clang`

## Repository layout

- `CMakeLists.txt` — build configuration (uses CMake + LLVM)
- `include/` — public headers (AST, lexer, parser, sem, codegen)
- `src/` — implementation (frontend + codegen)
- `test/` — example programs (e.g., `test/test.clv`)

## Build

Configure and build with CMake. Example:

```bash
# If `llvm-config --cmakedir` is non-empty, pass it as `LLVM_DIR`.
cmake -S . -B build -DLLVM_DIR=$(llvm-config --cmakedir) -DLLVM_LINK_LLVM_DYLIB=ON
cmake --build build -j
```

## Usage

### Using `extern` to call C functions

```clv
extern i32 puts(s : *char);

i32 main() {
	puts("Hello, Clover!");
	ret 0;
}
```

### `const` bindings

Example:

```clv
const PI : f64 = 3.14159;

i32 main() {
	x : f32 = PI as f32;
	ret 0;
}
```

### Struct definition

```clv
struct vec {
    a : f32,
    b : f32
}
```
---
