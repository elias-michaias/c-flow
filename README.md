# Functional Utilities and Iterators for C

**c-flow** is a single-header C library that brings functional-style utilities and iterator abstractions to C, inspired by languages like Rust, Scala, and Elixir. It provides a set of macros for mapping, filtering, folding, zipping, and composing operations on iterators, as well as utility macros like `chain()` and `pipe()` for composing functions in sequence on arbitrary types. It also includes a `curry()` macro for Clang users, as it relies on [blocks](https://en.wikipedia.org/wiki/Blocks_(C_language_extension)).

## Features
- **Iterator abstraction**: The `Iterator` struct wraps a pointer, length, and element size, allowing generic iteration over arrays and sequences.
- **Functional macros**: Map, filter, fold, zip, flatten, partition, scan, and more.
- **Pipe and chain composition**: Macros for chaining and piping operations, supporting both unary and multi-argument functions.
- **Single-header, zero dependencies**: Just include `flow.h` in your project.

## Core Concepts

### Iterator Structure
```c
typedef struct {
    void *data;
    size_t len;
    size_t elem_size;
} Iterator;
```
- Wraps a pointer to data, a length, and the size of each element.
- Created from static arrays using `to_iter(arr)`.

### Functional Macros
- **Mapping**: `iter_map(iter, in_type, in_var, out_type, out_expr)`
- **Filtering**: `iter_filter(iter, type, var, predicate)`
- **Folding**: `iter_foldl`, `iter_foldr`
- **Zipping**: `iter_zip`
- **Flattening**: `iter_flatten`
- **Partitioning**: `iter_partition`
- **Scan (prefix sum)**: `iter_scan`
- **Range, slice, pad, repeat, unique, concat, sum, for-each**: see `flow.h` for the full list

### Composition Macros
- **pipe(...)**: Compose a sequence of operations, using `_` as a placeholder for the previous result. `_` must always be the same type throughout the entire `pipe()` expression.
- **chain(...)**: Compose unary functions in a nested fashion. 

### Example Usage
```c
#include "flow.h"
#include <stdio.h>

int arr[] = {1, 2, 3, 4, 5};
Iterator it = to_iter(arr);

// Map: multiply each element by 2
Iterator mapped = iter_map(it, int, x, int, x * 2);

// Filter: keep even numbers
Iterator filtered = iter_filter(mapped, int, x, x % 2 == 0);

// Fold: sum all elements
int sum = iter_sum(filtered, int);
printf("Sum of evens: %d\n", sum);

// Pipe: chain operations
Iterator result = pipe(
    to_iter(arr),
    iter_map(_, int, x, int, x + 1),
    iter_filter(_, int, x, x % 2 == 0)
);

// For: effectful procs with no transformation on iter
iter_for(result, int, x, printf("%d ", x));
printf("\n");
```
See `example.c` for many more advanced and combined examples, including zip, flatten, partition, scan, and more.

## Technical Notes & Tradeoffs
- **Heap allocation**: Most macros that produce new iterators allocate new arrays on the heap. You are responsible for freeing memory if you need to avoid leaks.
- **Type safety**: Macros require you to specify types explicitly. There is no runtime type checking.
- **Macro limitations**: Debugging macro expansions can be tricky. IDEs with macro expansion support are recommended.
- **Not MSVC compatible**: Uses GCC expressions `({...})` which are supported in GCC and Clang.

## Getting Started
1. Copy `flow.h` into your project.
2. `#include "flow.h"` in your C file.
3. See `example.c` for usage patterns and inspiration.

## License
MIT License. See the file for details.
