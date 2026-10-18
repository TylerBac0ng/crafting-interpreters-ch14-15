#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG, // 14.2: 24-bit constant index
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  OP_RETURN,
} OpCode;

// 14.1: run-length encoded line information. Each LineStart marks the first
// byte of a run of bytecode that all comes from the same source line.
typedef struct {
  int offset;
  int line;
} LineStart;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  int lineCount;
  int lineCapacity;
  LineStart* lines;
  ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
void writeConstant(Chunk* chunk, Value value, int line);
int getLine(Chunk* chunk, int instruction);

#endif
