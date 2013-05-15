CFLAGS  = -m32 -g -Wall -O0
CC      = gcc
LFLAGS  = -m32 -g -lm -ldl
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
