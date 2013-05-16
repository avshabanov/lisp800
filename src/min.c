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
 * First two bits represents a value type.
 *
 * Note, that pointer types (e.g. cons cells) - are expected to be *ALWAYS*
 * aligned at least by 4, since the trailing bits are used to represent type
 * information and considered to always be zero.
 * This seems to not be a problem for all the modern unix.
 *
 * @see #LVAL_ANUM_TYPE
 * @see #LVAL_CONS_TYPE
 * @see #LVAL_IREF_TYPE
 * @see #LVAL_JREF_TYPE
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
 * nil or ansi char or unicode char or number (fixnum)
 */
#define LVAL_ANUM_TYPE      (0)

/**
 * Cons cell
 */
#define LVAL_CONS_TYPE      (1)

/**
 * IREF objects.
 * TODO: make it clear
 *
 *   Type           Sub Code
 *  symbol              0
 *  simple-vector       3
 *  array               4
 *  package             5
 *  function            6
 *  /User Defined/      t
 */
#define LVAL_IREF_TYPE      (2)

/**
 * JREF objects.
 * TODO: make it clear
 *
 *   Type           Sub Code
 *  simple-string       20
 *  double              84
 *  simple-bit-vector   116
 *  file-stream         t
 */
#define LVAL_JREF_TYPE      (3)


/**
 * Simple string sub type
 */
#define LVAL_JREF_SIMPLE_STRING_SUBTYPE         (20)


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
    return (lval *) (o - LVAL_CONS_TYPE);
}

/**
 * c2o - cons-to-object, this function is for cons'es only
 * converts pointer to the lvalue, 1 is a list
 * 1 designates list type, so that lval & 3 == 1!
 * @see #LVAL_CONS_TYPE
 */
lval c2o(lval * c) {
    assert(LVAL_GET_TYPE((lval) c) == 0); /* expect aligned pointer */
    return (lval) c + LVAL_CONS_TYPE;
}

/**
 * cp - consp, cons check
 * Returns 1 if given object is a cons, 0 otherwise
 */
int cp(lval o) {
    return LVAL_GET_TYPE(o) == LVAL_CONS_TYPE;
}

/* car/cdr/set_car/set_cdr list functions */

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


/**
 * o2a - object-to-iref (originally array)
 */
lval * o2a(lval o) {
    assert(LVAL_GET_TYPE(o) == LVAL_IREF_TYPE);
    return (lval *) (o - LVAL_IREF_TYPE);
}

/**
 * a2o - iref-to-object
 */
lval a2o(lval * a) {
    assert(LVAL_GET_TYPE((lval) a) == 0); /* expect aligned pointer */
    return (lval) a + LVAL_IREF_TYPE;
}

/**
 * ap - iref checker
 */
int ap(lval o) {
    return LVAL_GET_TYPE(o) == LVAL_IREF_TYPE;
}

/**
 * o2s - object-to-jref (originally string)
 */
lval * o2s(lval o) {
    assert(LVAL_GET_TYPE(o) == LVAL_JREF_TYPE);
    return (lval *) (o - LVAL_JREF_TYPE);
}

lval s2o(lval * s) {
    assert(LVAL_GET_TYPE((lval) s) == 0); /* expect aligned pointer */
    return (lval) s + LVAL_JREF_TYPE;
}

int sp(lval o) {
    return LVAL_GET_TYPE(o) == LVAL_JREF_TYPE;
}

/* Printing, see lisp800.print(lval) */

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
                fprintf(os, "#\\%c", (char) i);
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
    case LVAL_IREF_TYPE:
        assert(!"Unsupported");
        break;
    default:
        fprintf(os, "<#Unknown %ld>", x);
    }
}


/**
 * Execution context data, contains all the operations context.
 *
 * @see #m0
 * @see #init_exec_context
 */
struct exec_context {
    lval * memory;
    size_t memory_size; /**< Allocated memory size, in bytes */
    lval * memf; /**< Pointer to the current memory sub block */
    lval * stack;
    size_t stack_size; /**< Allocated stack size, in bytes */
};

/* Global execution context */

static struct exec_context * ec = NULL;

/* Garbage collecting routines */

/**
 * Puts garbage collector markers
 */
void gcm(lval v) {

    /* TODO: reintroduce */
}

/**
 * Collects garbage, f is a current stack pointer
 */
lval gc(lval * f) {
    assert(f < (ec->stack + ec->stack_size / sizeof(lval)));
    /* TODO: reintroduce */
    return 0;
}


/* Symbol introduction */

/**
 * Allocates n lval units in the context memory and returns it.
 * Returns 0 if no free cells available.
 * The caller party may use n lval blocks if the returned pointer is not null.
 */
lval * m0(int n) {
    lval * m = ec->memf; /* start searching from the first memory sub block */
    lval * p = 0; /* previous memory sub block */

    n = (n + 1) & ~1; /* round odd size to the greater even */

    /* iterate over the available sub blocks */
    for (; m; m = (lval *) m[0]) {
        if (n <= m[1]) {
            /* size requested fits into the current sub block */
            if (m[1] == n) {
                if (p) {
                    p[0] = m[0];
                } else {
                    /* allocate the entire sub block, but update memf pointer */
                    ec->memf = (lval *) m[0];
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

#if 0
/* TODO: m0 with out-of-memory reporting */
lval * oom_m0(int n, lval * g, ...) {
    lval * c = m0(n);
    if (!c) {
        va_list v;
        va_start(v, g);
        for (;;) {
            lval l = va_arg(v, lval);
            if (!l) {
                break;
            }
            gcm(l);
        }
        va_end(v);
        gc(g);
        
        /* retry */
        c = m0(n);
        if (!c) {
            fputs("Out of memory", stderr);
            exit(-1);
        }
    }
    
    return c;
}
#endif


/* LISP primitive functions */

/**
 * Creates cons cell of a and b.
 * Stack pointer f is used for garbage collecting (unused).
 */
lval cons(lval * f, lval a, lval b) {
    lval * c = m0(2); /* TODO: use oom_m0 or no gc here at all! */
    if (!c) {
        /* TODO: gc */
        fputs("Out of memory", stderr);
        exit(-1);
    }
    
    c[0] = a;
    c[1] = b;
    return c2o(c);
}

/* Allocates ordinary list */
lval l2(lval * f, lval a, lval b) {
    return cons(f, a, cons(f, b, 0));
}


/* Context initialization stuff */

static void init_exec_context(struct exec_context * c) {
    size_t stack_size = sizeof(lval) * 64 * 1024;

    c->memory_size = sizeof(lval) * 2 * 1024 * 1024;
    c->memory = malloc(c->memory_size);
    c->memf = c->memory;
    memset(c->memory, 0, c->memory_size);

    /* index of the next available sub block in memory */
    c->memf[0] = 0;
    /* first index contains available memory minus size plus two cells that
       store size */
    c->memf[1] = c->memory_size / sizeof(lval);

    c->stack = malloc(stack_size);
    memset(c->stack, 0, stack_size);
}

static void free_exec_context(struct exec_context * c) {
    free(c->memory);
    free(c->stack);
    memset(c, 0, sizeof(struct exec_context));
}

#if 1
/* Debug stuff */

static void exhaust_heap(lval * f) {
    int counter = 1000;
    int i;
    assert(ec->memory_size < 64); /* consider setting memory size small */
    for (;;) {
        lval * vt = m0(2);
        if (vt) {
            vt[0] =  ++counter;
            vt[1] =  ++counter;
        }
        fprintf(stdout, "vt=0x%08X, memf=0x%08X\n", vt, ec->memf);

        fputs("MEM:", stdout);
        for (i = 0; i < ec->memory_size / sizeof(lval); ++i) {
            fprintf(stdout, " %4d", ec->memory[i]);
        }
        fputs(".\n", stdout);

        if (vt == NULL) {
            break;
        }
    }
}

static void print_sample_cons(lval * f) {
    lval c;
    
    c = l2(f, l2(f, 0, 3 << 5), l2(f, 2 << 5, 0));
    fprintf(stdout, "c =\n");
    printval(c, stdout);
    fprintf(stdout, "\n\n");
}

#endif /* End of debug stuff */

int main(int argc, char * argv[]) {
    struct exec_context ctx;
    lval * f; /* current stack pointer, designated as `g' in lisp800.c */

    init_exec_context(&ctx);
    ec = &ctx;

    f = ec->stack;
    f += 5; /* TODO: copied, 5 is still unknown magic number */

    /* Use debug stuff */
#if 1
    if (argc > 2 && strcmp(argv[1], "testall") == 0) {
        exhaust_heap(f);
    }
#endif
    print_sample_cons(f);
    
    free_exec_context(&ctx);
    return 0;
}
