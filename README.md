lisp800
=======

lisp800 is based on Teemu Kalvas's lisp500. Lisp800 is an attempt to clean up the original Lisp500 source code to make it more useful for embedding and hacking.
The main goal is to make interpreter embeddable and multiple context-oriented.

## How to start
```bash
  cd src
  make
  rlwrap ./target/lisp800 init800.lisp
```
Note, that ``rlwrap`` is not mandatory, i.e. you can run this as ``./target/lisp800 init800.lisp`` but the the latter lacks convenient readline wrapper's features you may want to have.

## How to run smoke test
```bash
  cd src
  ./retest.sh
```
The execution output of the last command should contain PASSED at the last line, e.g.:
```text
OK: 1 is (FOO)
OK: T is (EQUAL (MACROEXPAND-1 (QUOTE (DEFWRAP FOO))) (QUOTE (DEFUN FOO NIL 1)))
PASSED
```
