# !/bin/bash

set -e
make clean && make
./target/lisp800 "init800.lisp" "test/smoke.lisp"
