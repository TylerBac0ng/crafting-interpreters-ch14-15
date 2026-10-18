// 15.3 and 15.4: a stack that grows, and in-place negate.
#include <stdio.h>

#include "../src/chunk.h"
#include "../src/vm.h"

int main() {
  initVM();

  // Push 1000 values before adding any of them. The book's fixed stack
  // holds 256 values, so this would have written past the end of it.
  Chunk chunk;
  initChunk(&chunk);
  for (int i = 1; i <= 1000; i++) writeConstant(&chunk, i, 1);
  for (int i = 1; i < 1000; i++) writeChunk(&chunk, OP_ADD, 1);
  writeChunk(&chunk, OP_RETURN, 1);

  printf("capacity before: %d\n", vm.stackCapacity);
  printf("sum of 1..1000 (expect 500500): ");
  interpret(&chunk);
  printf("capacity after: %d (grew 8, 16, ..., 1024)\n", vm.stackCapacity);
  freeChunk(&chunk);

  // The stack stays allocated, so a second run doesn't grow it again.
  initChunk(&chunk);
  writeConstant(&chunk, 5, 1);
  writeChunk(&chunk, OP_NEGATE, 1);
  writeChunk(&chunk, OP_NEGATE, 1);
  writeChunk(&chunk, OP_NEGATE, 1);
  writeChunk(&chunk, OP_RETURN, 1);
  printf("\nnegate 5 three times in place (expect -5): ");
  interpret(&chunk);
  printf("capacity: %d, stack empty after return: %s\n", vm.stackCapacity,
         vm.stackTop == vm.stack ? "yes" : "no");
  freeChunk(&chunk);

  // In-place binary operators must keep the operand order: 2 - 3 and 3 / 4.
  initChunk(&chunk);
  writeConstant(&chunk, 2, 1);
  writeConstant(&chunk, 3, 1);
  writeChunk(&chunk, OP_SUBTRACT, 1);
  writeConstant(&chunk, 3, 1);
  writeConstant(&chunk, 4, 1);
  writeChunk(&chunk, OP_DIVIDE, 1);
  writeChunk(&chunk, OP_ADD, 1);
  writeChunk(&chunk, OP_RETURN, 1);
  printf("(2 - 3) + (3 / 4) (expect -0.25): ");
  interpret(&chunk);
  freeChunk(&chunk);

  freeVM();
  return 0;
}
