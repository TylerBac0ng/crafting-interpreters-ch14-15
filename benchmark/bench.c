// 15.4: times OP_NEGATE and the binary operators. Build it with
// -DIN_PLACE_OPS=0 for the book's pop-then-push version and -DIN_PLACE_OPS=1
// for the in-place version (see run.sh).
#include <stdio.h>
#include <time.h>

#include "../src/chunk.h"
#include "../src/vm.h"

#define OPS 1000000
#define RUNS 200

static double timeChunk(Chunk* chunk) {
  clock_t start = clock();
  for (int i = 0; i < RUNS; i++) interpret(chunk);
  return (double)(clock() - start) / CLOCKS_PER_SEC;
}

int main() {
  initVM();

  // 1.5 negated a million times (an even number, so the result is 1.5).
  Chunk negate;
  initChunk(&negate);
  writeConstant(&negate, 1.5, 1);
  for (int i = 0; i < OPS; i++) writeChunk(&negate, OP_NEGATE, 1);
  writeChunk(&negate, OP_RETURN, 1);

  // 1 + 1 + 1 + ... : half a million OP_CONSTANT / OP_ADD pairs.
  Chunk add;
  initChunk(&add);
  writeConstant(&add, 1, 1);
  for (int i = 0; i < OPS / 2; i++) {
    writeChunk(&add, OP_CONSTANT, 1);
    writeChunk(&add, 0, 1);
    writeChunk(&add, OP_ADD, 1);
  }
  writeChunk(&add, OP_RETURN, 1);

  // OP_RETURN prints the result, so run.sh sends stdout to /dev/null and
  // the timings go to stderr.
  double negateTime = timeChunk(&negate);
  double addTime = timeChunk(&add);
  fprintf(stderr, "%.3f %.3f\n", negateTime, addTime);

  freeChunk(&negate);
  freeChunk(&add);
  freeVM();
  return 0;
}
