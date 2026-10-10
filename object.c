/*
 * Tscheme: A Tiny Scheme Interpreter
 * Copyright (c) 1995-2013 Takuo WATANABE (Tokyo Institute of Technology)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "tscheme.h"

/* pairs */

SCM mk_pair(SCM car, SCM cdr) {
    SCM x;
    NEWCELL(x, T_PAIR);
    CAR(x) = car;
    CDR(x) = cdr;
    return x;
}

/* strings */

SCM mk_string(char *string, long dim) {
    char *newstr;
    if ((newstr = (char *)malloc ((unsigned)dim+1)) == NULL)
        fatal_error("malloc: mk_string");
    strcpy(newstr, string);
    SCM x;
    NEWCELL(x, T_STRING);
    STR_DIM(x) = dim;
    STR_DATA(x) = newstr;
    return x;
}

/* symbols */

SCM newsym(SCM pname, SCM value) {
    SCM x;
    NEWCELL(x, T_SYMBOL);
    SYM_PNAME(x) = pname;
    SYM_VALUE(x) = value;
    return x;
}

SCM mk_symbol(char *name) {
    /* computre hash location */
    unsigned long hash = 0, c;
    unsigned char *s = (unsigned char *)name;
    while ((c = *s++))
        hash = ((hash * 17)^c) % obarray_dim;
    SCM sl = obarray[hash];
    /* check if the symbol already exists */
    for (SCM l = sl; !IS_NULL(l); l = CDR(l))
        if (strcmp(name, STR_DATA(SYM_PNAME(CAR(l)))) == 0)
            return CAR(l);
    /* allocate new symbol */
    SCM x = newsym(mk_string(name, strlen(name)), unbound_value);
    obarray[hash] = mk_pair(x, sl);
    return x;
}

/* Subrs */

SCM mk_subr(char *name, SCM (*fun)(void), int nargs) {
    SCM subr;
    switch (nargs) {
    case 0:
        NEWCELL(subr, T_SUBR0);
        break;
    case 1:
        NEWCELL(subr, T_SUBR1);
        break;
    case 2:
        NEWCELL(subr, T_SUBR2);
        break;
    case 3:
        NEWCELL(subr, T_SUBR3);
        break;
    default:
        NEWCELL(subr, T_SUBRN);
        break;
    }
    SCM symbol = mk_symbol (name);
    SYM_VALUE(symbol) = subr;
    SUBR_NAME(subr) = symbol;
    SUBR_FUN(subr) = fun;
    return subr;
}

SCM mk_fsubr(char *name, SCM (*fun)(void)) {
    SCM subr;
    NEWCELL(subr, T_FSUBR);
    SCM symbol = mk_symbol(name);
    SYM_VALUE(symbol) = subr;
    SUBR_NAME(subr) = symbol;
    SUBR_FUN(subr) = fun;
    return subr;
}

/* Closures */

SCM mk_closure(SCM args, SCM code, SCM env) {
    SCM closure;
    NEWCELL(closure, T_CLOSURE);
    CLOSURE_CODE(closure) = mk_pair(args, code);
    CLOSURE_ENV(closure) = env;
    return closure;
}

