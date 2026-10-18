// 14.2: OP_CONSTANT_LONG and writeConstant().
#include <stdio.h>

#include "../src/chunk.h"
#include "../src/debug.h"
#include "../src/vm.h"

int main() {
  initVM();

  // 300 constants: 0 to 255 use OP_CONSTANT, 256 and up use OP_CONSTANT_LONG.
  // Adding them all up checks that the VM reads both forms correctly.
  Chunk chunk;
  initChunk(&chunk);
  for (int i = 0; i < 300; i++) writeConstant(&chunk, i, 1);
  for (int i = 0; i < 299; i++) writeChunk(&chunk, OP_ADD, 1);
  writeChunk(&chunk, OP_RETURN, 1);

  printf("constants 254 to 257:\n");
  int offset = 0;
  for (int i = 0; i < 258; i++) {
    if (i >= 254) {
      offset = disassembleInstruction(&chunk, offset);
    } else {
      offset += chunk.code[offset] == OP_CONSTANT ? 2 : 4;
    }
  }
  printf("bytes: %d (256 * 2 + 44 * 4 for the loads, 300 for the rest)\n",
         chunk.count);
  printf("sum of 0..299 (expect 44850): ");
  interpret(&chunk);
  freeChunk(&chunk);

  // An index that needs all three bytes of the operand.
  initChunk(&chunk);
  for (int i = 0; i < 70000; i++) addConstant(&chunk, 0);
  writeConstant(&chunk, 7.5, 2);
  writeChunk(&chunk, OP_RETURN, 2);
  printf("\n");
  disassembleChunk(&chunk, "index 70000");
  printf("operand bytes: %02x %02x %02x\n",
         chunk.code[1], chunk.code[2], chunk.code[3]);
  printf("result: ");
  interpret(&chunk);
  freeChunk(&chunk);

  freeVM();
  return 0;
}
