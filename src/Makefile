CFLAGS  = -Wall
CC      = gcc
LFLAGS  = -lm -ldl
LINKER  = gcc


## Rules

target/lisp800: target/lisp800.o
	$(LINKER) $(LFLAGS) -o target/lisp800 target/lisp800.o

target/lisp800.o: target lisp800.c
	$(CC) $(CFLAGS) -c lisp800.c -o target/lisp800.o

target:
	mkdir target

clean:
	rm -rf target
