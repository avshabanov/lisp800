#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>

#ifndef countof
#define countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

/* Main lvalue representation */
typedef intptr_t lval;

/* Extracts lval type (first three bits) */
#define LVAL_GET_TYPE(l) ((l) & 3)

/* Nil type checker */
#define LVAL_IS_NIL(x)      (0 == (x))

/* Char type code (4-th bit) */
#define LVAL_IS_CHAR(x)     (1 == ((x) & 8))
#define LVAL_IS_INT(x)      (0 == ((x) & 8))

/* Internal manip helpers */

lval * o2c(lval o) {
    return (lval *) (o - 1);
}

lval c2o(lval * c) {
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



int main(int argc, char * argv[]) {
    fprintf(stdout, "sizeof(lval) = %ld\n", sizeof(lval));
    return 0;
}
