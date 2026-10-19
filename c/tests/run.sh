#!/bin/sh
# Extra tests for the C version.  The .out files were produced by
# the cREXX interpreter (lisp.crexx), so a pass means both agree.
#   c/tests/run.sh            run all tests with c/lisp-c
#   LISP=./lisp c/tests/run.sh     check the cREXX version itself
HERE=$(cd "$(dirname "$0")" && pwd)
LISP=${LISP:-"$HERE/../lisp-c"}
pass=0; fail=0
for src in "$HERE"/*.lisp "$HERE"/*.repl; do
  case "$src" in
    *.lisp) name=$(basename "$src" .lisp) ;;
    *)      name=$(basename "$src" .repl) ;;
  esac
  case "$src" in
    *.lisp) out=$("$LISP" "$src" < /dev/null 2>&1) ;;
    *)      out=$("$LISP" < "$src" 2>&1) ;;
  esac
  if [ "$1" = "--update" ]; then
    printf '%s\n' "$out" > "$HERE/$name.out"
    echo "updated $name"
    continue
  fi
  if [ "$out" = "$(cat "$HERE/$name.out")" ]; then
    pass=$((pass+1)); echo "ok    $name"
  else
    fail=$((fail+1)); echo "FAIL  $name"
  fi
done
[ "$1" = "--update" ] && exit 0
echo "$pass passed, $fail failed"
[ $fail = 0 ]
