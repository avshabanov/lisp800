#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>

#ifndef countof
#define countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

/**
 * Main lvalue representation.</p>
 * First three bits are generic type tags:
 * 0xxx - used to distinguish four generic types:
 * <ul>
 *  <li>0 - numbers, chars, nil</li>
 *  <li>1 - cons</li>
 *  <li>2 - functions, pacakges, ???</li>
 *  <li>3 - strings, ???</li>
 * </ul>
 */
typedef intptr_t lval;

/* Extracts lval type (first three bits) */
#define LVAL_GET_TYPE(l) ((l) & 3)

/* nil or ansi char or unicode char or number */
#define LVAL_ANUM_TYPE      (0)
#define LVAL_CONS_TYPE      (1)


/* Nil type checker */
#define LVAL_IS_NIL(l)      (0 == (l))

/* Char type code (4-th bit) */
#define LVAL_IS_CHAR(l)     ((0 != ((l) & 8)) && (LVAL_ANUM_TYPE == LVAL_GET_TYPE(l)))
#define LVAL_IS_INT(l)      (0 == ((l) & 8))




/* Internal manipulations */

/**
 * o2c - object-to-cons
 * converts lvalue to the pointer, 1 is a list (this function is for CONS cells only)
 * @see #LVAL_CONS_TYPE
 */
lval * o2c(lval o) {
    assert(LVAL_GET_TYPE(o) == LVAL_CONS_TYPE);
    return (lval *) (o - 1);
}

/**
 * c2o - cons-to-object
 * converts pointer to the lvalue, 1 is a list (this function is for CONS cells only)
 * 1 designates list type, so that lval & 3 == 1!
 * @see #LVAL_CONS_TYPE
 */
lval c2o(lval * c) {
    assert((((lval) c) & 3) == 0);
    return (lval) c + 1;
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

/* print */

void printval(lval x, FILE * os) {
    int i;

    switch (LVAL_GET_TYPE(x)) {
    case LVAL_ANUM_TYPE:
        if (LVAL_IS_NIL(x)) {
            fputs("nil", os);
        } else {
            /* TODO: unicode chars, see lisp800.print(lval) */
            if (LVAL_IS_CHAR(x)) {

            }
        }
        break;

    default:
        fprintf(os, "<#Unknown %X>", x);
    }
}

int main(int argc, char * argv[]) {
    setbuf(stdout, NULL);
    fprintf(stdout, "sizeof(lval) = %uld\n", sizeof(lval));
    return 0;
}
