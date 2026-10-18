#!/bin/bash
# Builds the benchmark with and without in-place operators, at -O0 and -O2,
# runs each build 5 times, and prints the fastest time for each.
cd "$(dirname "$0")/.."
CC=${CC:-cc}
SOURCES=$(ls src/*.c | grep -v main.c)
mkdir -p build

echo "200 runs of 1,000,000 instructions each, best of 5 (seconds)"
printf "%-6s %-14s %10s %18s\n" "flags" "version" "OP_NEGATE" "OP_CONSTANT+OP_ADD"
for opt in -O0 -O2; do
  for inplace in 0 1; do
    exe=build/bench$opt-$inplace
    $CC -std=c99 $opt -DIN_PLACE_OPS=$inplace $SOURCES benchmark/bench.c -o $exe || exit 1
    best_negate=999; best_add=999
    for i in 1 2 3 4 5; do
      read negate add < <($exe 2>&1 >/dev/null)
      best_negate=$(echo "$negate $best_negate" | awk '{print ($1 < $2) ? $1 : $2}')
      best_add=$(echo "$add $best_add" | awk '{print ($1 < $2) ? $1 : $2}')
    done
    version=$([ $inplace = 1 ] && echo "in place" || echo "pop + push")
    printf "%-6s %-14s %10s %18s\n" $opt "$version" $best_negate $best_add
  done
done
