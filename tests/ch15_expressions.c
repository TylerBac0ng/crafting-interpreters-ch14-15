// 15.1 and 15.2: the bytecode from the written answers, run on the VM.
#include <stdio.h>
#include <stdarg.h>

#include "../src/chunk.h"
#include "../src/debug.h"
#include "../src/vm.h"

// Builds a chunk from a list of instructions. A number means "load this
// constant", anything else is an opcode. -1 ends the list.
#define C(n) 1000 + (n)
static void run(const char* source, ...) {
  Chunk chunk;
  initChunk(&chunk);

  va_list args;
  va_start(args, source);
  for (int op = va_arg(args, int); op != -1; op = va_arg(args, int)) {
    if (op >= 1000) {
      writeConstant(&chunk, op - 1000, 1);
    } else {
      writeChunk(&chunk, (uint8_t)op, 1);
    }
  }
  va_end(args);
  writeChunk(&chunk, OP_RETURN, 1);

  disassembleChunk(&chunk, source);
  printf("result: ");
  interpret(&chunk);
  printf("\n");
  freeChunk(&chunk);
}

int main() {
  initVM();

  printf("--- 15.1 ---\n");
  run("1 * 2 + 3", C(1), C(2), OP_MULTIPLY, C(3), OP_ADD, -1);
  run("1 + 2 * 3", C(1), C(2), C(3), OP_MULTIPLY, OP_ADD, -1);
  run("3 - 2 - 1", C(3), C(2), OP_SUBTRACT, C(1), OP_SUBTRACT, -1);
  run("1 + 2 * 3 - 4 / -5",
      C(1), C(2), C(3), OP_MULTIPLY, OP_ADD,
      C(4), C(5), OP_NEGATE, OP_DIVIDE, OP_SUBTRACT, -1);

  printf("--- 15.2 ---\n");
  run("4 - 3 * -2 (both)",
      C(4), C(3), C(2), OP_NEGATE, OP_MULTIPLY, OP_SUBTRACT, -1);
  run("4 - 3 * -2 (no OP_NEGATE)",
      C(4), C(3), C(0), C(2), OP_SUBTRACT, OP_MULTIPLY, OP_SUBTRACT, -1);
  run("4 - 3 * -2 (no OP_SUBTRACT)",
      C(4), C(3), C(2), OP_NEGATE, OP_MULTIPLY, OP_NEGATE, OP_ADD, -1);

  // Replacing OP_NEGATE with 0 - x is not exact for floating point:
  // -(0) is -0, but 0 - 0 is +0.
  run("-0 with OP_NEGATE", C(0), OP_NEGATE, -1);
  run("-0 as 0 - 0", C(0), C(0), OP_SUBTRACT, -1);

  freeVM();
  return 0;
}
