#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

#ifdef SYSTEM_ALLOCATOR

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}

#else

// 14.3 hardcore mode. The heap is a sequence of blocks. Every block has a
// one-word header and a one-word footer that both hold the block's size, with
// the low bit set if the block is in use:
//
//   | header | payload ...                       | footer |
//            ^ pointer returned to the caller (16-byte aligned)
//
// The footer is a "boundary tag": it lets free() find the size of the block
// just before it, so neighboring free blocks can be merged in constant time.
// Free blocks also store next/prev pointers in their payload, forming an
// explicit doubly linked free list. Allocation is first fit over that list.
//
// The heap starts with a "prologue" block and ends with an "epilogue" header,
// both marked in use, so coalescing never has to check the heap's edges.

#define WORD sizeof(size_t)
#define ALIGNMENT 16
#define MIN_BLOCK 32 // header + next + prev + footer

typedef struct FreeBlock {
  struct FreeBlock* next;
  struct FreeBlock* prev;
} FreeBlock;

static uint8_t* heap = NULL;
static FreeBlock* freeList = NULL;

// All of these take a payload pointer.
#define HEADER(p)     ((size_t*)((uint8_t*)(p) - WORD))
#define SIZE(p)       (*HEADER(p) & ~(size_t)1)
#define IS_USED(p)    (*HEADER(p) & 1)
#define FOOTER(p)     ((size_t*)((uint8_t*)(p) + SIZE(p) - 2 * WORD))
#define NEXT_BLOCK(p) ((uint8_t*)(p) + SIZE(p))
#define PREV_FOOTER(p) (*(size_t*)((uint8_t*)(p) - 2 * WORD))
#define PREV_BLOCK(p) ((uint8_t*)(p) - (PREV_FOOTER(p) & ~(size_t)1))

static void setBlock(void* p, size_t size, bool used) {
  *HEADER(p) = size | (used ? 1 : 0);
  *FOOTER(p) = size | (used ? 1 : 0);
}

static void insertFree(void* p) {
  FreeBlock* block = (FreeBlock*)p;
  block->prev = NULL;
  block->next = freeList;
  if (freeList != NULL) freeList->prev = block;
  freeList = block;
}

static void removeFree(void* p) {
  FreeBlock* block = (FreeBlock*)p;
  if (block->prev != NULL) {
    block->prev->next = block->next;
  } else {
    freeList = block->next;
  }
  if (block->next != NULL) block->next->prev = block->prev;
}

static void initHeap() {
  // The one and only call to malloc().
  heap = malloc(HEAP_SIZE);
  if (heap == NULL) {
    fprintf(stderr, "Could not allocate the %d byte heap.\n", HEAP_SIZE);
    exit(1);
  }

  // malloc() returns 16-byte aligned memory, so payloads start at offsets
  // that are multiples of 16 and headers sit one word before them.
  uint8_t* prologue = heap + 2 * WORD;
  setBlock(prologue, 2 * WORD, true);

  uint8_t* first = NEXT_BLOCK(prologue);
  setBlock(first, HEAP_SIZE - 4 * WORD, false);
  insertFree(first);

  *HEADER(NEXT_BLOCK(first)) = 0 | 1; // Epilogue: size 0, in use.
}

static size_t blockSizeFor(size_t payload) {
  size_t size = (payload + 2 * WORD + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1);
  return size < MIN_BLOCK ? MIN_BLOCK : size;
}

// Merges [p] with any free neighbors and puts the result on the free list.
static void freeBlock(void* p) {
  size_t size = SIZE(p);

  uint8_t* next = NEXT_BLOCK(p);
  if (!IS_USED(next)) {
    removeFree(next);
    size += SIZE(next);
  }

  if (!(PREV_FOOTER(p) & 1)) {
    uint8_t* prev = PREV_BLOCK(p);
    removeFree(prev);
    size += SIZE(prev);
    p = prev;
  }

  setBlock(p, size, false);
  insertFree(p);
}

// Shrinks the in-use block [p] to [size] bytes and frees the rest, if the rest
// is big enough to be a block of its own.
static void split(void* p, size_t size) {
  size_t total = SIZE(p);
  if (total - size < MIN_BLOCK) return;

  setBlock(p, size, true);
  uint8_t* rest = NEXT_BLOCK(p);
  setBlock(rest, total - size, true);
  freeBlock(rest);
}

static void* allocate(size_t payload) {
  size_t size = blockSizeFor(payload);

  for (FreeBlock* block = freeList; block != NULL; block = block->next) {
    if (SIZE(block) >= size) {
      removeFree(block);
      setBlock(block, SIZE(block), true);
      split(block, size);
      return block;
    }
  }

  fprintf(stderr, "Out of memory: no free block for %zu bytes.\n", payload);
  exit(1);
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (heap == NULL) initHeap();

  if (newSize == 0) {
    if (pointer != NULL) freeBlock(pointer);
    return NULL;
  }

  if (pointer == NULL) return allocate(newSize);

  size_t size = blockSizeFor(newSize);

  // Shrinking, or growing within the slack the block already has.
  if (size <= SIZE(pointer)) {
    split(pointer, size);
    return pointer;
  }

  // Growing into a free block right after this one, without copying.
  uint8_t* next = NEXT_BLOCK(pointer);
  if (!IS_USED(next) && SIZE(pointer) + SIZE(next) >= size) {
    removeFree(next);
    setBlock(pointer, SIZE(pointer) + SIZE(next), true);
    split(pointer, size);
    return pointer;
  }

  void* result = allocate(newSize);
  memcpy(result, pointer, oldSize < newSize ? oldSize : newSize);
  freeBlock(pointer);
  return result;
}

bool checkHeap(HeapStats* stats) {
  if (heap == NULL) initHeap();
  memset(stats, 0, sizeof(HeapStats));

  int freeInList = 0;
  for (FreeBlock* block = freeList; block != NULL; block = block->next) {
    if (IS_USED(block)) return false;
    freeInList++;
  }

  bool previousFree = false;
  uint8_t* p = NEXT_BLOCK(heap + 2 * WORD);
  while (SIZE(p) != 0) {
    if (*HEADER(p) != *FOOTER(p)) return false;
    if ((uintptr_t)p % ALIGNMENT != 0) return false;

    if (IS_USED(p)) {
      stats->usedBlocks++;
      stats->usedBytes += SIZE(p);
      previousFree = false;
    } else {
      // Two free blocks next to each other should have been merged.
      if (previousFree) return false;
      stats->freeBlocks++;
      stats->freeBytes += SIZE(p);
      if (SIZE(p) > stats->largestFree) stats->largestFree = SIZE(p);
      previousFree = true;
    }
    p = NEXT_BLOCK(p);
  }

  if (p != heap + HEAP_SIZE) return false;
  return freeInList == stats->freeBlocks;
}

#endif
