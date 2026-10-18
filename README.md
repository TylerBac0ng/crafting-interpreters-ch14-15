# Crafting Interpreters — Chapters 14 & 15 Challenges

Solutions to the end-of-chapter challenges from [_Crafting Interpreters_](https://craftinginterpreters.com/) by Robert Nystrom, chapters **14 (Chunks of Bytecode)** and **15 (A Virtual Machine)**. This is the start of clox, the bytecode virtual machine in C, and it holds the complete code through chapter 15. There is no scanner or compiler yet (those come in chapters 16 and 17), so like the book's `main.c`, the tests build chunks of bytecode by hand and run them.

## Build and run

```sh
make              # builds build/clox (runs the book's test chunk)
make trace        # same, printing the stack and each instruction as it runs
./run_tests.sh    # runs the tests
make bench        # the 15.4 benchmark
```

Build with `-DSYSTEM_ALLOCATOR` to use the C library's `realloc()` and `free()` instead of the custom allocator from 14.3.

## Chapter 14 — Chunks of Bytecode

1. **Run-length encoded line numbers.** A chunk stores an array of
   `LineStart { offset, line }` runs instead of one `int` per byte.
   `writeChunk()` only adds a run when the line changes, and `getLine()`
   binary searches the runs. Offsets always increase, so this works even if
   line numbers go backwards. For 10,000 bytes on 100 lines, the line info
   drops from 65,536 bytes to 1,024. See `tests/ch14_lines.c`.
2. **`OP_CONSTANT_LONG`.** `writeConstant()` uses `OP_CONSTANT` for the first
   256 constants and `OP_CONSTANT_LONG` with a 24-bit little-endian operand
   after that. The disassembler and the VM both support it. See
   `tests/ch14_constant_long.c`.
3. **How `malloc()` works**: written answer (submitted separately).
   **Hardcore mode is implemented:** `reallocate()` calls `malloc()` once to
   get a 64 MiB heap and manages it itself. Blocks have a header and footer
   (boundary tags), free blocks are kept on an explicit free list, and
   allocation is first fit with splitting. Freeing merges a block with its
   free neighbors, and `realloc` shrinks or grows in place when it can.
   `checkHeap()` checks the heap's invariants. See `tests/ch14_heap.c`, which
   ends with a 200,000-operation stress test.

## Chapter 15 — A Virtual Machine

1. **Bytecode for four expressions.** Written answer (submitted separately).
   `tests/ch15_expressions.c` runs each sequence on the VM.
2. **Dropping `OP_NEGATE` or `OP_SUBTRACT`.** Written answer (submitted
   separately), with the three sequences for `4 - 3 * -2` also run in
   `tests/ch15_expressions.c`. One test shows that `0 - x` can't replace `-x`
   exactly: `-(0)` is `-0`, but `0 - 0` is `0`.
3. **A stack that grows.** The VM's stack is a dynamic array that doubles when
   it is full. `push()` rebuilds `stackTop` after growing, since the array may
   move. `tests/ch15_stack.c` pushes 1,000 values, more than the book's fixed
   256-value stack can hold.
4. **Negating in place.** `OP_NEGATE` changes the top of the stack directly,
   and the binary operators pop one operand and overwrite the other. The
   benchmark builds both versions:

   | Build | Version | `OP_NEGATE` | `OP_CONSTANT` + `OP_ADD` |
   |---|---|---:|---:|
   | `-O0` | pop + push | 1.616 s | 1.340 s |
   | `-O0` | in place | 1.145 s | 0.959 s |
   | `-O2` | pop + push | 0.482 s | 0.438 s |
   | `-O2` | in place | 0.483 s | 0.392 s |

   200 runs of 1,000,000 instructions, best of 5, on an Intel Core i7-7700HQ
   with Apple clang 14. Results will be different on other machines.

## Files changed from the book's chapter 15 code

| File | Change |
|---|---|
| `chunk.h`, `chunk.c` | `LineStart` runs and `getLine()` (14.1); `OP_CONSTANT_LONG` and `writeConstant()` (14.2) |
| `debug.c` | Line numbers from `getLine()` (14.1); disassembles `OP_CONSTANT_LONG` (14.2) |
| `memory.h`, `memory.c` | Custom heap allocator and `checkHeap()` (14.3) |
| `vm.h`, `vm.c` | `OP_CONSTANT_LONG` (14.2); growable stack (15.3); in-place negate and binary operators (15.4) |
| `main.c` | The book's test chunk, written with `writeConstant()` |

## Tests

`./run_tests.sh` builds the interpreter and every program in `tests/`, and
compares each one's output with its `.expected` file. It runs everything
twice, once with the custom allocator and once with `-DSYSTEM_ALLOCATOR`. The
heap test only runs in the first mode. All 11 tests pass.
