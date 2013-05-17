
CMN     = -m32 -g
#CMN     = -m32 -O2 -fomit-frame-pointer

CFLAGS  = $(CMN) -Wall -O0
CC      = gcc
LFLAGS  = $(CMN) -lm -ldl
LINKER  = gcc


## Rules

target/lisp800: target/lisp800.o
	$(LINKER) -o target/lisp800 target/lisp800.o $(LFLAGS)

target/lisp800.o: target lisp800.c
	$(CC) $(CFLAGS) -c lisp800.c -o target/lisp800.o

target:
	mkdir target

clean:
	rm -rf target
