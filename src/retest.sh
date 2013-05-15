# !/bin/bash

set -e
make clean && make
./target/lisp800 "core800.lisp" "test/smoke.lisp"
