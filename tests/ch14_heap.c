// 14.3 hardcore mode: the custom allocator behind reallocate().
#include <stdio.h>
#include <string.h>

#include "../src/memory.h"

static void report(const char* label) {
  HeapStats stats;
  bool ok = checkHeap(&stats);
  printf("%-28s valid: %-3s used blocks: %d  free blocks: %d\n",
         label, ok ? "yes" : "NO", stats.usedBlocks, stats.freeBlocks);
}

#define SLOTS 500
static unsigned int seed = 12345;
static unsigned int randomInt() {
  seed = seed * 1103515245 + 12345;
  return (seed >> 16) & 0x7fff;
}

int main() {
  report("empty heap:");
  HeapStats stats;
  checkHeap(&stats);
  printf("free bytes: %zu (heap size %d minus 32 bytes of bookkeeping)\n\n",
         stats.freeBytes, HEAP_SIZE);

  // Freeing coalesces with both neighbors.
  char* a = reallocate(NULL, 0, 24);
  char* b = reallocate(NULL, 0, 100);
  char* c = reallocate(NULL, 0, 1000);
  report("after 3 allocations:");
  reallocate(b, 100, 0);
  report("free middle:");
  reallocate(a, 24, 0);
  report("free first (merges):");
  reallocate(c, 1000, 0);
  report("free last (merges all):");

  // Growing into the free block that follows, without moving.
  char* p = reallocate(NULL, 0, 16);
  char* q = reallocate(NULL, 0, 16);
  char* guard = reallocate(NULL, 0, 16);
  reallocate(q, 16, 0);
  strcpy(p, "lox");
  char* grown = reallocate(p, 16, 40);
  printf("\ngrow into free neighbor: same pointer: %s, data kept: %s\n",
         grown == p ? "yes" : "no", strcmp(grown, "lox") == 0 ? "yes" : "no");

  // Growing when the next block is in use moves the data.
  char* moved = reallocate(guard, 16, 5000);
  char* blocker = reallocate(NULL, 0, 16);
  strcpy(moved, "clox");
  char* moved2 = reallocate(moved, 5000, 100000);
  printf("grow with used neighbor: moved: %s, data kept: %s\n",
         moved2 != moved ? "yes" : "no", strcmp(moved2, "clox") == 0 ? "yes" : "no");

  char* shrunk = reallocate(moved2, 100000, 8);
  printf("shrink: same pointer: %s\n", shrunk == moved2 ? "yes" : "no");
  reallocate(shrunk, 8, 0);
  reallocate(grown, 40, 0);
  reallocate(blocker, 16, 0);
  report("everything freed:");

  // Stress test: random sizes, frees and reallocs. Each block is filled with
  // a byte pattern that is checked before the block is touched again.
  char* slots[SLOTS] = {0};
  size_t sizes[SLOTS] = {0};
  int corrupt = 0;
  int badHeaps = 0;
  for (int op = 0; op < 200000; op++) {
    int i = randomInt() % SLOTS;
    for (size_t j = 0; j < sizes[i]; j++) {
      if (slots[i][j] != (char)(i + j)) { corrupt++; break; }
    }

    size_t newSize = randomInt() % 4 == 0 ? 0 : randomInt() % 2000 + 1;
    if (randomInt() % 50 == 0) newSize = randomInt() * 16;
    slots[i] = reallocate(slots[i], sizes[i], newSize);
    sizes[i] = newSize;
    for (size_t j = 0; j < sizes[i]; j++) slots[i][j] = (char)(i + j);

    if (op % 1000 == 0 && !checkHeap(&stats)) badHeaps++;
  }
  for (int i = 0; i < SLOTS; i++) reallocate(slots[i], sizes[i], 0);
  checkHeap(&stats);
  printf("\nstress: 200000 random operations, corrupted blocks: %d, "
         "bad heap checks: %d\n", corrupt, badHeaps);
  report("after stress test:");
  printf("free bytes back to %zu: %s\n", stats.freeBytes,
         stats.freeBytes == (size_t)HEAP_SIZE - 32 ? "yes" : "no");
  return 0;
}
