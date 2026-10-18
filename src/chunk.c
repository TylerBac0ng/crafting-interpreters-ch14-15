#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lineCount = 0;
  chunk->lineCapacity = 0;
  chunk->lines = NULL;
  initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code,
        oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->count++;

  // 14.1: still on the same line as the last run, so nothing to record.
  if (chunk->lineCount > 0 &&
      chunk->lines[chunk->lineCount - 1].line == line) {
    return;
  }

  if (chunk->lineCapacity < chunk->lineCount + 1) {
    int oldCapacity = chunk->lineCapacity;
    chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
    chunk->lines = GROW_ARRAY(LineStart, chunk->lines,
        oldCapacity, chunk->lineCapacity);
  }

  LineStart* start = &chunk->lines[chunk->lineCount++];
  start->offset = chunk->count - 1;
  start->line = line;
}

int addConstant(Chunk* chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}

// 14.2: use the short form when the index fits in a byte, and a 24-bit
// little-endian operand otherwise.
void writeConstant(Chunk* chunk, Value value, int line) {
  int index = addConstant(chunk, value);
  if (index < 256) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, (uint8_t)index, line);
  } else if (index < (1 << 24)) {
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    writeChunk(chunk, (uint8_t)(index & 0xff), line);
    writeChunk(chunk, (uint8_t)((index >> 8) & 0xff), line);
    writeChunk(chunk, (uint8_t)((index >> 16) & 0xff), line);
  } else {
    fprintf(stderr, "Too many constants in one chunk.\n");
    exit(1);
  }
}

// 14.1: binary search for the last run that starts at or before
// the instruction. Run offsets always increase, so this works even if the
// line numbers themselves jump around.
int getLine(Chunk* chunk, int instruction) {
  int low = 0;
  int high = chunk->lineCount - 1;
  int line = -1;

  while (low <= high) {
    int mid = low + (high - low) / 2;
    if (chunk->lines[mid].offset <= instruction) {
      line = chunk->lines[mid].line;
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return line;
}
