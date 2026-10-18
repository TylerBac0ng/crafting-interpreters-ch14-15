#!/bin/bash
# Builds every test in tests/ against the clox sources, runs it, and compares
# its output with tests/<name>.expected. Everything runs twice: once with the
# custom heap allocator (14.3) and once with -DSYSTEM_ALLOCATOR.
cd "$(dirname "$0")"
CC=${CC:-cc}
CFLAGS="-std=c99 -Wall -Wextra -Wno-unused-parameter -O2"
SOURCES=$(ls src/*.c | grep -v main.c)
mkdir -p build
pass=0
fail=0

check() {
  local name=$1 expected=$2 actual=$3
  if diff -u "$expected" "$actual" > build/diff.txt; then
    echo "PASS $name"
    pass=$((pass + 1))
  else
    echo "FAIL $name"
    cat build/diff.txt
    fail=$((fail + 1))
  fi
}

for mode in heap system; do
  flags=$CFLAGS
  [ $mode = system ] && flags="$flags -DSYSTEM_ALLOCATOR"

  $CC $flags src/*.c -o build/clox-$mode || exit 1
  build/clox-$mode > build/main.out
  check "main ($mode)" tests/main.expected build/main.out

  for test in tests/*.c; do
    name=$(basename "$test" .c)
    # The heap test inspects the custom allocator, which system mode leaves out.
    [ $mode = system ] && [ $name = ch14_heap ] && continue
    $CC $flags $SOURCES "$test" -o build/$name-$mode || exit 1
    build/$name-$mode > build/$name.out
    check "$name ($mode)" tests/$name.expected build/$name.out
  done
done

echo "$pass passed, $fail failed"
[ $fail -eq 0 ]
