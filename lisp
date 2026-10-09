#!/bin/sh
# lisp - run Lisp programs, or start a read-eval-print loop
#
#   lisp file.lisp [more.lisp ...]     load and run the files in order
#   lisp                               read forms from standard input
#
# Environment: CREXX_BIN = directory with rxvm (default: found via PATH)
HERE=$(cd "$(dirname "$0")" && pwd)
BIN=${CREXX_BIN:-$(dirname "$(command -v rxvm)")}
if [ ! -f "$HERE/build/lisp.rxbin" ]; then
  echo "lisp: run 'make' first" >&2
  exit 2
fi
if [ $# -eq 0 ]; then
  exec "$BIN/rxvm" "$HERE/build/lisp.rxbin"
fi
exec "$BIN/rxvm" "$HERE/build/lisp.rxbin" -a "$@"
