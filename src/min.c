/**
 * Attempt to reimplement lisp800 step-by-step in documented and reliable way.
 *
 * @author Alexander Shabanov, 2013, http://alexshabanov.com
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>

#ifndef countof
#define countof(x) (sizeof(x)/sizeof((x)[0]))
#endif


/**
 * Main lvalue representation.</p>
 *
 * First two bits represents a value type:
 * <ul>
 *  <li>0 - number, char, nil</li>
 *  <li>1 - cons</li>
 *  <li>2 - function, package, ... ???</li>
 *  <li>3 - strings, ... ???</li>
 * </ul>
 * </p>
 * Note, that pointer types (e.g. cons cells) - are expected to be *ALWAYS*
 * aligned at least by 4, since the trailing bits are used to represent type
 * information and considered to always be zero.
 * This seems to not be a problem for all the modern unix.
 */
typedef intptr_t lval;

/**
 * Integer type, compatible with lval type. All the resultant integers
 * should be placed into this type to avoid overflow error.
 */
typedef intptr_t lint;

/**
 * Extracts lval type (first two bits)
 */
#define LVAL_GET_TYPE(l) ((l) & 3)

/**
 * nil or ansi char or unicode char or number
 */
#define LVAL_ANUM_TYPE      (0)

/**
 * Cons cell
 */
#define LVAL_CONS_TYPE      (1)


/* Nil type checker */
#define LVAL_IS_NIL(l)      (0 == (l))

/* Char type code (4-th bit) */
#define LVAL_IS_CHAR(l)     ((0 != ((l) & 8)) && \
    (LVAL_ANUM_TYPE == LVAL_GET_TYPE(l)))

#define LVAL_IS_INT(l)      (0 == ((l) & 8))

/* Extracts numeric value from lval */
#define LVAL_AS_ANUM(l)     ((l) >> 5)


/* Internal manipulations */

/**
 * o2c - object-to-cons, this function is for cons'es only
 * converts lvalue to the pointer, 1 is a list
 * @see #LVAL_CONS_TYPE
 */
lval * o2c(lval o) {
    assert(LVAL_GET_TYPE(o) == LVAL_CONS_TYPE);
    return (lval *) (o - 1);
}

/**
 * c2o - cons-to-object, this function is for cons'es only
 * converts pointer to the lvalue, 1 is a list
 * 1 designates list type, so that lval & 3 == 1!
 * @see #LVAL_CONS_TYPE
 */
lval c2o(lval * c) {
    assert((((lval) c) & 3) == 0);
    return (lval) c + 1;
}

/**
 * cp - consp, cons check
 * Returns 1 if given object is a cons, 0 otherwise
 */
int cp(lval o) {
    return LVAL_GET_TYPE(o) == LVAL_CONS_TYPE;
}

/* List functions */

lval car(lval c) {
    return (c & 3) == 1 ? o2c(c)[0] : 0;
}

lval cdr(lval c) {
    return (c & 3) == 1 ? o2c(c)[1] : 0;
}

lval set_car(lval c, lval val) {
    return o2c(c)[0] = val;
}

lval set_cdr(lval c, lval val) {
    return o2c(c)[1] = val;
}

/* see lisp800.print(lval) */
void printval(lval x, FILE * os) {
    lint i;
    switch (LVAL_GET_TYPE(x)) {
    case LVAL_ANUM_TYPE:
        if (LVAL_IS_NIL(x)) {
            fputs("nil", os);
        } else {
            i = LVAL_AS_ANUM(x);
            /* TODO: unicode chars, see lisp800.print(lval) */
            if (LVAL_IS_CHAR(x)) {
                fprintf(os, "#\\%c", i);
            } else {
                fprintf(os, "%ld", (long int) i);
            }
        }
        break;
    case LVAL_CONS_TYPE:
        fputc('(', os);
        printval(car(x), os);
        for (x = cdr(x); cp(x); x = cdr(x)) {
            fputc(' ', os);
            printval(car(x), os);
        }
        if (0 != x) {
            fputs(" . ", os);
            printval(x, os);
        }
        fputc(')', os);
        break;
    default:
        fprintf(os, "<#Unknown %X>", x);
    }
}


/**
 * Execution context data, contains all the operations context
 */
struct exec_context {
    lval * memory;
    lval * memf;
    size_t memory_size;
    lval * stack;
};

/* Global execution context */
static struct exec_context * ec = NULL;


/* Symbol introduction */

/**
 * Allocates n lval units in the context memory and returns it.
 * Returns 0 if no free cells available.
 * The caller party may use n lval blocks if the returned pointer is not null.
 */
lval * m0(int n) {
    lval * m = ec->memf; /* start searching from the first memory sub block */
    lval * p = 0;

    n = (n + 1) & ~1; /* round odd size to the greater even */

    for (; m; m = (lval *) m[0]) {
        if (n <= m[1]) {
            /* size requested fits the current sub block */
            if (m[1] == n) {
                if (p) {
                    p[0] = m[0];
                } else {
                    ec->memf = (lval *) m[0]; /* this block fu */
                }
            } else {
                m[1] -= n;
                m += m[1];
            }
            return m;
        }

        p = m;
    }

    return 0;
}


/* Context initialization stuff */

static void init_exec_context(struct exec_context * c) {
    size_t stack_size = sizeof(lval) * 64 * 1024;

    c->memory_size = sizeof(lval) * 1024 * 1024;
    c->memory = malloc(c->memory_size);
    c->memf = c->memory;
    memset(c->memory, 0, c->memory_size);

    /* index of the next available sub block in memory??? */
    c->memf[0] = 0;
    /* first index contains available memory minus size plus two cells that
       store size */
    c->memf[1] = c->memory_size / sizeof(lval) - 2;

    c->stack = malloc(stack_size);
    memset(c->stack, 0, stack_size);
}

static void free_exec_context(struct exec_context * c) {
    free(c->memory);
    free(c->stack);
    memset(c, 0, sizeof(struct exec_context));
}


int main(int argc, char * argv[]) {
    struct exec_context ctx;
    lval * sp; /* current stack pointer, designated as `g' in lisp800.c */

    init_exec_context(&ctx);
    ec = &ctx;

    sp = ec->stack;
    sp += 5; /* TODO: copied, 5 is still unknown magic number */

    free_exec_context(&ctx);
    return 0;
}
