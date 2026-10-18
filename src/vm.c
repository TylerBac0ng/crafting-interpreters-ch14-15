#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "memory.h"
#include "vm.h"

// 15.4: 1 = negate and binary operators work on the stack in place,
// 0 = the book's pop-then-push version. Only the benchmark builds with 0.
#ifndef IN_PLACE_OPS
#define IN_PLACE_OPS 1
#endif

VM vm;

static void resetStack() {
  vm.stackTop = vm.stack;
}

void initVM() {
  vm.stack = NULL;
  vm.stackCapacity = 0;
  resetStack();
}

void freeVM() {
  FREE_ARRAY(Value, vm.stack, vm.stackCapacity);
  initVM();
}

void push(Value value) {
  // 15.3: grow the stack when it is full. reallocate() may move it, so
  // stackTop has to be rebuilt from its index afterwards.
  int count = (int)(vm.stackTop - vm.stack);
  if (count == vm.stackCapacity) {
    int oldCapacity = vm.stackCapacity;
    vm.stackCapacity = GROW_CAPACITY(oldCapacity);
    vm.stack = GROW_ARRAY(Value, vm.stack, oldCapacity, vm.stackCapacity);
    vm.stackTop = vm.stack + count;
  }

  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() \
    (vm.ip += 3, vm.chunk->constants.values[ \
        vm.ip[-3] | (vm.ip[-2] << 8) | (vm.ip[-1] << 16)])
#if IN_PLACE_OPS
#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      vm.stackTop[-1] = vm.stackTop[-1] op b; \
    } while (false)
#else
#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      double a = pop(); \
      push(a op b); \
    } while (false)
#endif

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk,
                           (int)(vm.ip - vm.chunk->code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_CONSTANT_LONG: {
        Value constant = READ_CONSTANT_LONG();
        push(constant);
        break;
      }
      case OP_ADD:      BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE:   BINARY_OP(/); break;
#if IN_PLACE_OPS
      case OP_NEGATE:   vm.stackTop[-1] = -vm.stackTop[-1]; break;
#else
      case OP_NEGATE:   push(-pop()); break;
#endif
      case OP_RETURN: {
        printValue(pop());
        printf("\n");
        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef BINARY_OP
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}
