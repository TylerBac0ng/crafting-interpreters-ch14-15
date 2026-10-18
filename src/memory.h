#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

// 14.3 hardcore mode: reallocate() parcels memory out of one big block that
// is malloc()ed once. Build with -DSYSTEM_ALLOCATOR to use realloc()/free()
// like the book does.
#ifndef SYSTEM_ALLOCATOR

#ifndef HEAP_SIZE
#define HEAP_SIZE (64 * 1024 * 1024)
#endif

typedef struct {
  int usedBlocks;
  int freeBlocks;
  size_t usedBytes;    // Including block headers and footers.
  size_t freeBytes;
  size_t largestFree;
} HeapStats;

// Walks every block, checks the heap's invariants, and fills in [stats].
// Returns false if the heap is corrupt.
bool checkHeap(HeapStats* stats);

#endif

#endif
