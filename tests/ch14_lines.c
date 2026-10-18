// 14.1: run-length encoded line numbers.
#include <stdio.h>

#include "../src/chunk.h"
#include "../src/debug.h"

int main() {
  // Bytes on these lines, including a jump back from line 5 to line 3.
  int lines[] = {1, 1, 1, 2, 2, 5, 5, 5, 5, 3, 3, 7};
  int count = sizeof(lines) / sizeof(lines[0]);

  Chunk chunk;
  initChunk(&chunk);
  for (int i = 0; i < count; i++) writeChunk(&chunk, OP_RETURN, lines[i]);

  printf("%d bytes stored in %d runs:\n", chunk.count, chunk.lineCount);
  for (int i = 0; i < chunk.lineCount; i++) {
    printf("  offset %2d -> line %d\n",
           chunk.lines[i].offset, chunk.lines[i].line);
  }

  int mismatches = 0;
  for (int i = 0; i < count; i++) {
    if (getLine(&chunk, i) != lines[i]) {
      printf("offset %d: expected line %d, got %d\n",
             i, lines[i], getLine(&chunk, i));
      mismatches++;
    }
  }
  printf("getLine() matches all %d offsets: %s\n",
         count, mismatches == 0 ? "yes" : "no");
  freeChunk(&chunk);

  // A bigger chunk: 100 lines with 100 bytes each.
  initChunk(&chunk);
  for (int line = 1; line <= 100; line++) {
    for (int i = 0; i < 100; i++) writeChunk(&chunk, OP_RETURN, line);
  }
  mismatches = 0;
  for (int i = 0; i < chunk.count; i++) {
    if (getLine(&chunk, i) != i / 100 + 1) mismatches++;
  }
  printf("\n%d bytes, %d runs, mismatches: %d\n",
         chunk.count, chunk.lineCount, mismatches);
  printf("line info before: %zu bytes (one int per byte of capacity)\n",
         sizeof(int) * chunk.capacity);
  printf("line info now:    %zu bytes (one LineStart per run of capacity)\n",
         sizeof(LineStart) * chunk.lineCapacity);
  freeChunk(&chunk);

  // The disassembler only prints a line number when it changes.
  initChunk(&chunk);
  writeConstant(&chunk, 1, 10);
  writeConstant(&chunk, 2, 10);
  writeChunk(&chunk, OP_ADD, 10);
  writeChunk(&chunk, OP_NEGATE, 11);
  writeChunk(&chunk, OP_RETURN, 12);
  printf("\n");
  disassembleChunk(&chunk, "lines");
  freeChunk(&chunk);
  return 0;
}
