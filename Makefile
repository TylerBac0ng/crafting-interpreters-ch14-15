CC ?= cc
CFLAGS ?= -std=c99 -Wall -O2

build/clox: src/*.c src/*.h
	mkdir -p build
	$(CC) $(CFLAGS) src/*.c -o $@

# Prints the stack and each instruction as it runs.
trace: src/*.c src/*.h
	mkdir -p build
	$(CC) $(CFLAGS) -DDEBUG_TRACE_EXECUTION src/*.c -o build/clox-trace
	build/clox-trace

test:
	./run_tests.sh

bench:
	./benchmark/run.sh

clean:
	rm -rf build

.PHONY: trace test bench clean
