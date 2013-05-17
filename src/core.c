/**
 * Attempt to reimplement lisp800 step-by-step in documented and reliable way.
 * This file contains only core functionality.
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
 * Main lvalue representation.
 * </p>
 * First two bits represents a value type.
 * </p>
 * Note, that pointer types (e.g. cons cells) - are expected to be *ALWAYS*
 * aligned at least by 4, since the trailing bits are used to represent type
 * information and considered to always be zero.
 * This seems to not be a problem for all the modern unix.
 * </p>
 * Zero value is reinterpreted as nil.
 *
 * @see #LVAL_TYPE_MASK
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
 * Value that represents lval's nil
 */
#define LVAL_NIL            ((lval) 0)

/**
 * Represents bit mask that should be applied to lval to extract type info.
 * Changing this to the different value will have immediate impact on all the
 * bitwise type operations.
 */
#define LVAL_TYPE_MASK      (3)

/**
 * Extracts lval type (first two bits)
 */
#define LVAL_GET_TYPE(l) ((l) & LVAL_TYPE_MASK)

/**
 * Simple numeric objects.
 * nil or ansi char or unicode char or number (fixnum)
 */
#define LVAL_ANUM_TYPE      (0)

/**
 * Cons cell
 */
#define LVAL_CONS_TYPE      (1)

/**
 * IREF objects.
 * Sub types: symbol, simple-vector, array, package, function
 */
#define LVAL_IREF_TYPE      (2)

/**
 * JREF objects.
 * Sub types: simple-string, double, simple-bit-vector, file-stream
 */
#define LVAL_JREF_TYPE      (3)

/**
 * Char code bit - applicable for ANUM type
 */
#define LVAL_CHAR_BIT       (8)

/* Extracts numeric value from lval */
#define LVAL_AS_ANUM(l)     ((l) >> 5)

#define ANUM_AS_LVAL(l)     ((l) << 5)

/**
 * GC marker bit
 *
 * @see #gcm
 * @see #gc
 */
#define LVAL_GCM_BIT        (4)

/* Subtype codes */

#define LVAL_IREF_FUNCTION_SUBTYPE              (212)
#define LVAL_IREF_SYMBOL_SUBTYPE                (20)
#define LVAL_IREF_SIMPLE_VECTOR_SUBTYPE         (116)
#define LVAL_IREF_PACKAGE_SUBTYPE               (180)

#define LVAL_JREF_SIMPLE_STRING_SUBTYPE         (20)
#define LVAL_JREF_DOUBLE_SUBTYPE                (84)
#define LVAL_JREF_BIT_VECTOR_SUBTYPE            (116)

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
    return LVAL_GET_TYPE(c) == LVAL_CONS_TYPE ? o2c(c)[0] : 0;
}

lval cdr(lval c) {
    return LVAL_GET_TYPE(c) == LVAL_CONS_TYPE ? o2c(c)[1] : 0;
}

lval set_car(lval c, lval val) {
    assert(LVAL_GET_TYPE(c) == LVAL_CONS_TYPE);
    return o2c(c)[0] = val;
}

lval set_cdr(lval c, lval val) {
    assert(LVAL_GET_TYPE(c) == LVAL_CONS_TYPE);
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

/**
 * Reinterprets jref object as zero-terminated string
 */
char * o2z(lval o) {
    assert(LVAL_GET_TYPE(o) == LVAL_JREF_TYPE);
    return (char *) (o - LVAL_JREF_TYPE + 2 * sizeof(lval));
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
    FILE * slog; /**< Log stream, nullable */
    lval pkg; /**< Current package */
    lval kwp; /**< Keyword package */
};

/* Global execution context */

static struct exec_context * ec = NULL;

/* Logs */

void log_write(const char * s, ...) {
    if (ec->slog) {
        va_list ap;
        va_start(ap, s);
        vfprintf(ec->slog, s, ap);
        va_end(ap);
    }
}

/* Printing, see lisp800.print(lval) */

void psym(lval p, lval sym, FILE * os) {
    lint i;
    if (!p) {
        fputs("#: ", os);
    } else if (p != ec->pkg) {
        lval m = car(o2a(p)[2]);
        /* TODO: remove magic numbers */
        for (i = 0; i < o2s(m)[0] / 64 - 4; i++) {
            fputc(o2z(m)[i], os);
        }
        fputc(':', os);
    }

    for (i = 0; i < o2s(sym)[0] / 64 - 4; i++) {
        fputc(o2z(sym)[i], os);
    }
}

void printval(lval x, FILE * os) {
    lval * val;
    lint i;
    switch (LVAL_GET_TYPE(x)) {
    case LVAL_ANUM_TYPE:
        if (x == LVAL_NIL) {
            fputs("nil", os);
        } else {
            i = LVAL_AS_ANUM(x);
            /* TODO: unicode chars, see lisp800.print(lval) */
            if (x & LVAL_CHAR_BIT) {
                fputs("#\\", os);
                fputc((char) i, os);
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
        if (LVAL_NIL != x) {
            fputs(" . ", os);
            printval(x, os);
        }
        fputc(')', os);
        break;

    case LVAL_IREF_TYPE:
        val = o2a(x);
        switch (val[1]) {
        case LVAL_IREF_FUNCTION_SUBTYPE:
            fputs("#<function ", os);
            /* TODO: remove magic number */
            printval(val[6], os);
            fputc('>', os);
            break;
        case LVAL_IREF_SYMBOL_SUBTYPE:
            /* TODO: remove magic number */
            psym(val[9], val[2], os);
            break;
        case LVAL_IREF_SIMPLE_VECTOR_SUBTYPE:
            fputs("#(", os);
            for (i = 0; i < (val[0] >> 8); ++i) {
                if (i > 0) {
                    fputc(' ', os);
                }
                printval(val[i + 2], os);
            }
            fputc(')', os);
            break;
        case LVAL_IREF_PACKAGE_SUBTYPE:
            fputs("#<package ", os);
            printval(car(val[2]), os);
            fputc('>', os);
            break;
        default:
            assert(!"Not implemented");
        }
        break;

    case LVAL_JREF_TYPE:
        val = o2s(x);
        switch (val[1]) {
        case LVAL_JREF_SIMPLE_STRING_SUBTYPE:
            fputc('"', os);
            for (i = 0; i < (val[0] / 64 - 4); ++i) {
                char c = o2z(x)[i];
                if (c == '\\' || c == '\"') {
                    fputc('\\', os);
                }
                fputc(c, os);
            }
            fputc('"', os);
            break;

        /* TODO: double??? */

        default:
            assert(!"Not implemented");
        }
        break;

    default:
        assert(!"Not implemented");
    }
}

/* Memory management routines */

/**
 * Puts garbage collector markers.
 */
void gcm(lval v) {
    lval * t;

    st:
    t = (lval *) (v & ~LVAL_TYPE_MASK); /* convert to typeless pointer */
    if (v & LVAL_TYPE_MASK && !(t[0] & LVAL_GCM_BIT)) {
        t[0] |= LVAL_GCM_BIT;
        switch (LVAL_GET_TYPE(v)) {
        case LVAL_CONS_TYPE:
            gcm(t[0] - LVAL_GCM_BIT);
            v = t[1];
            goto st;

        case LVAL_IREF_TYPE:
            gcm(t[1] - 4);
            /* Not implemented */
        }
        /* TODO: strings, LVAL_JREF_TYPE */
    }
}

/**
 * Collects garbage, f is a current stack pointer
 */
lval gc(lval * f) {
    log_write("; garbage collecting...\n");

    /* move memf to the previous sub blocks in hope they will be free */
    while (ec->memf) {
        lval * prev_memf = (lval *) ec->memf[0];
        memset(ec->memf, 0, sizeof(lval) * ec->memf[1]);
        ec->memf = prev_memf;
    }

    /* mark uncollectable elements (on stack) */
    for (; f > ec->stack; --f) {
        gcm(*f);
    }

    return LVAL_NIL;
}

/**
 * Allocates n lval units in the context memory and returns it.
 * Returns NULL if no free cells available.
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

    return NULL;
}

/**
 * Checked memory allocation.
 * Allocates n lval units, throws error if unable to do so.
 * Never returns NULL.
 */
lval * cm0(int n, lval * f) {
    lval * m = m0(n);
    if (NULL == m) {
        /* TODO: collect garbage multiple times, if possible */
        gc(f);
        if (NULL == m) {
            /* TODO: raise LISP error */
            fputs("Out of memory", stderr);
            exit(-1);
        }
    }
    return m;
}


/* Allocation routines */

/**
 * Allocates iref
 */
lval * ma0(lval * f, int n) {
    lval * m = cm0(n + 2, f);
    *m = n << 8; /* TODO: remove magic number */
    return m;
}

lval ma(lval * f, int n, ...) {
    /* TODO: rewrite */
    va_list v;
    int i;
    lval * m;
    va_start(v, n);
    m = cm0(n + 2, f);
    *m = n << 8;
    for (i = -1; i < n; i++) {
        m[2 + i] = va_arg(v, lval);
    }
    return a2o(m);
}

/**
 * Allocates jref
 */
lval * ms0(lval * f, int n) {
    lval * m = cm0(n / sizeof(lval) + 3, f);
    *m = (n + sizeof(lval)) << 6; /* TODO: remove magic number */
    return m;
}

/**
 * Creates cons cell of a and b.
 * Stack pointer f is used for garbage collecting.
 */
lval cons(lval * f, lval a, lval b) {
    lval * c = cm0(2, f);
    c[0] = a;
    c[1] = b;
    return c2o(c);
}

/**
 * Allocates ordinary lis
 */
lval l2(lval * f, lval a, lval b) {
    return cons(f, a, cons(f, b, LVAL_NIL));
}

/**
 * Allocates string
 */
lval strf(lval * f, const char * s) {
    int j = strlen(s);
    lval *str = ms0(f, j);
    str[1] = LVAL_JREF_SIMPLE_STRING_SUBTYPE;
    for (j++; j; j--) {
        ((char *) str)[7 + j] = s[j - 1];
    }
    return s2o(str);
}

lval mkv(lval * f) {
    /* TODO: rewrite */
    int i = 2;
    lval *r = ma0(f, 1021);
    r[1] = LVAL_IREF_SIMPLE_VECTOR_SUBTYPE;
    while (i < 1023) {
        r[i++] = 0;
    }
    return a2o(r);
}

lval mkp(lval * f, const char *s0, const char *s1) {
    /* TODO: rewrite */
    return ma(f, 6, LVAL_IREF_PACKAGE_SUBTYPE,
          l2(f, strf(f, s0), strf(f, s1)), mkv(f), mkv(f), 0, 0, 0);
}


/* Evaluation core */

#if 0
lval evca(lval * f, lval co) {
    lval ex = car(co);
    lval x = ex;
    int m;

    ag:
    xvalues = 8;

    if (cp(ex)) {
        lval fn = 8;
        if (ap(car(ex)) && o2a(car(ex))[1] == 20) {
            int i = o2a(car(ex))[7] >> 3;
            if (i > 11 && i < 34)
                return symi[i].fun(f, cdr(ex));
            fn = *binding(f, car(ex), 1, &m);
            if (m) {
                lval *g = f + 1;
                for (ex = cdr(ex); ex; ex = cdr(ex))
                    *++g = car(ex);
                x = ex = call(f, fn, g - f - 1);
                set_car(co, ex);
                goto ag;
            }
        }
        st:
        if (fn == 8) {
            if (dbgr(f, 1, car(ex), &fn))
                return fn;
            else
                goto st;
        }
        ex = cdr(ex);
        ex = call(f, fn, map_eval(f, ex));
    } else if (ap(ex) && o2a(ex)[1] == 20) {
        ex = *binding(f, ex, 0, &m);
        if (m) {
            x = ex;
            set_car(co, ex);
            goto ag;
        }
        if (ex == 8) {
            dbgr(f, 0, x, &ex);
        }
    }
    return ex == -8 ? o2a(x)[4] : ex;
}
#endif

/* Core lisp functions */

lval lprint(lval * f) {
    printval(f[1], stdout);
    return f[1];
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

    c->slog = stdout;
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
            vt[0] = ++counter;
            vt[1] = ++counter;
        }
        fprintf(stdout, "vt=0x%08X, memf=0x%08X\n", (int) vt, (int) ec->memf);

        fputs("MEM:", stdout);
        for (i = 0; i < ec->memory_size / sizeof(lval); ++i) {
            fprintf(stdout, " %4d", (int) ec->memory[i]);
        }
        fputs(".\n", stdout);

        if (vt == NULL) {
            break;
        }
    }
}

static void print_sample_cons(lval * f) {
    lval c;
    
    c = l2(f, l2(f, LVAL_NIL, ANUM_AS_LVAL(3)), l2(f, ANUM_AS_LVAL(5), LVAL_NIL));
    fprintf(stdout, "c =\n");
    printval(c, stdout);
    fprintf(stdout, "\n\n");
}

static void print_something(lval * f) {
    printval(ec->pkg, stdout);
    fprintf(stdout, "\n");
}

#endif /* End of debug stuff */

int main(int argc, char * argv[]) {
    struct exec_context ctx;
    lval * f; /* current stack pointer, designated as `g' in lisp800.c */

    init_exec_context(&ctx);
    ec = &ctx;

    f = ec->stack;
    f += 5; /* TODO: copied, 5 is still unknown magic number */

    /* init packages */
    ec->pkg = mkp(f, "CL", "COMMON-LISP");
    ec->kwp = mkp(f, "KEYWORD", "");

    /* Use debug stuff */
#if 1
    if (argc > 2 && strcmp(argv[1], "testall") == 0) {
        exhaust_heap(f);
        print_sample_cons(f);
        print_something(f);
    }
#endif
    print_something(f);
    
    free_exec_context(&ctx);
    return 0;
}
