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
#include <setjmp.h>
#include <string.h>

#include "tscheme.h"

/* any */

/* EQ? x y */
SCM s_eq(SCM x, SCM y) {
    return EQ(x, y) ? boolean_true : boolean_false;
}

/* NEQ? x y */
SCM s_neq(SCM x, SCM y) {
    return NEQ(x, y) ? boolean_true : boolean_false;
}

/* null & pairs */

/* PAIR? x */
SCM s_pairp(SCM x) {
    return IS_PAIR(x) ? boolean_true : boolean_false;
}

/* NULL? */
SCM s_nullp(SCM x) {
    return IS_NULL(x) ? boolean_true : boolean_false;
}

/* LIST? */
SCM s_listp(SCM x) {
    SCM p = x;
    if (IS_NULL(p))
        return boolean_true;
    while (IS_PAIR(p)) {
        p = CDR(p);
        if (IS_NULL(p))
            return boolean_true;
    }
    return boolean_false;
}

/*
  #define s_cons(x,y) mk_pair(x,y)
*/

/* CAR pair */
SCM s_car(SCM pair) {
    if (!IS_PAIR(pair))
        wta_error("car", 1);
    return CAR(pair);
}

/* CDR pair */
SCM s_cdr(SCM pair) {
    if (!IS_PAIR(pair))
        wta_error("cdr", 1);
    return CDR(pair);
}

/* SET-CAR! pair x */
SCM s_setcar(SCM pair, SCM value) {
    if (!IS_PAIR(pair))
        wta_error("set-car!", 1);
    CAR(pair) = value;
    return unspecified_value;
}

/* SET-CDR! pair x */
SCM s_setcdr(SCM pair, SCM x) {
    if (!IS_PAIR(pair))
        wta_error("set-cdr!", 1);
    CDR(pair) = x;
    return unspecified_value;
}

/* LIST x* */
SCM n_list(SCM xs) {
    return xs;
}

/* LENGTH list */
SCM s_length(SCM list) {
    SCM x = list;
    long i = 0;
    if (!IS_PAIR(x))
        wta_error ("length", 1);
    while (IS_PAIR(x)) {
        i++;
        x = CDR(x);
    }
    return MK_FIXNUM(i);
}

/* MEMQ key list */
SCM s_memq(SCM key, SCM list) {
    SCM l = list;
    while (!IS_NULL(l)) {
        if (!(IS_PAIR(l) || IS_NULL(l)))
            wta_error("memq", 2);
        if (EQ(key, CAR(l)))
            return l;
        l = CDR(l);
    }
    return boolean_false;
}

/* LAST list */
SCM s_last(SCM list) {
    SCM p = list;
    if (!(IS_PAIR(p) || IS_NULL(p)))
        wta_error("last", 1);
    if (IS_PAIR(p))
        while (IS_PAIR(CDR(p)))
            p = CDR(p);
    return p;
}

/* APPEND list list */
SCM s_rec_append(SCM xxs, SCM ys) {
    SCM xs = xxs;
    if (!(IS_PAIR(xs) || IS_NULL(xs)))
        wta_error("rec-append", 1);
    if (IS_NULL(xs))
        return ys;
    else
        return CONS(CAR(xs), s_rec_append (CDR(xs), ys));
}

/* Boolean */

/* BOOLEAN? x */
SCM s_booleanp(SCM x) {
    return IS_BOOLEAN(x) ? boolean_true : boolean_false;
}

/* NOT x */
SCM s_not(SCM x) {
    return EQ(x, boolean_false) ? boolean_true : boolean_false;
}

/* Character */

/* CHAR? x */
SCM s_charp(SCM x) {
    return IS_CHARACTER(x) ? boolean_true : boolean_false;
}

/* CHAR= ch ch */
SCM s_char_equal(SCM x, SCM y) {
    if (!IS_CHARACTER(x))
        wta_error("char=?", 1);
    if (!IS_CHARACTER(y))
        wta_error("char=?", 2);
    return CHARACTER(x) == CHARACTER(y) ? boolean_true : boolean_false;
}

SCM s_char_lessthan(SCM x, SCM y) {
    if (!IS_CHARACTER(x))
        wta_error("char<?", 1);
    if (!IS_CHARACTER(y))
        wta_error("char<?", 2);
    return CHARACTER(x) < CHARACTER(y) ? boolean_true : boolean_false;
}

SCM s_char_lessequal(SCM x, SCM y) {
    if (!IS_CHARACTER(x))
        wta_error("char<=?", 1);
    if (!IS_CHARACTER(y))
        wta_error("char<=?", 2);
    return CHARACTER(x) <= CHARACTER(y) ? boolean_true : boolean_false;
}

SCM s_char_greaterthan(SCM x, SCM y) {
    if (!IS_CHARACTER(x))
        wta_error("char>?", 1);
    if (!IS_CHARACTER(y))
        wta_error("char>?", 2);
    return CHARACTER(x)> CHARACTER(y) ? boolean_true : boolean_false;
}

SCM s_char_greaterequal(SCM x, SCM y) {
    if (!IS_CHARACTER(x))
        wta_error("char>=?", 1);
    if (!IS_CHARACTER(y))
        wta_error("char>=?", 2);
    return CHARACTER(x)>=CHARACTER(y) ? boolean_true : boolean_false;
}


/* Symbol */

SCM s_symbolp(SCM x) {
    return IS_SYMBOL(x) ? boolean_true : boolean_false;
}

SCM s_symbol_to_string(SCM symbol) {
    if (!IS_SYMBOL(symbol))
        wta_error("symbol->string", 1);
    return SYM_PNAME(symbol);
}

SCM s_string_to_symbol(SCM s) {
    if (!IS_STRING(s))
        wta_error("string->symbol", 1);
    return (mk_symbol(STR_DATA(s)));
}

/* String */

SCM s_stringp(SCM x) {
    return IS_STRING(x) ? boolean_true : boolean_false;
}

SCM s_string_append(SCM strings) {
    SCM xs = strings;
    int nargs = 0, len = 0;
    while (!IS_NULL(xs)) {
        nargs++;
        if (!IS_STRING(CAR(xs))) wta_error("string-append", nargs);
        len += STR_DIM(CAR(xs));
        xs = CDR(xs);
    }
    char *buf;
    if ((buf = (char *)malloc(len + 1)) == NULL)
        fatal_error("malloc: s_string_append");
    xs = strings;
    int p = 0;
    while (!IS_NULL(xs)) {
        int i = 0;
        int d = STR_DIM(CAR(xs));
        char *s = STR_DATA(CAR(xs));
        while (i<d) buf[p++] = s[i++];
        xs = CDR(xs);
    }
    buf[p] = '\0';
    SCM x;
    NEWCELL(x, T_STRING);
    STR_DIM(x) = len;
    STR_DATA(x) = buf;
    return x;
}

/* Fixnum */

/* NUMBER? x */
SCM s_numberp(SCM x) {
    return IS_FIXNUM(x) ? boolean_true : boolean_false;
}

/* ZERO? n */
SCM s_zerop(SCM x) {
    if (!IS_FIXNUM(x))
        wta_error("zero?", 1);
    return EQ(MK_FIXNUM(0),x) ? boolean_true : boolean_false;
}

/* + n n */
SCM s_plus(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error("+", 1);
    if (!IS_FIXNUM(y))
        wta_error("+", 2);
    return MK_FIXNUM(FIXNUM(x) + FIXNUM(y));
}

/* - n n */
SCM s_minus(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error("-", 1);
    if (!IS_FIXNUM(y))
        wta_error("-", 2);
    return MK_FIXNUM(FIXNUM(x) - FIXNUM(y));
}

/* * n n */
SCM s_times(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error("*", 1);
    if (!IS_FIXNUM(y))
        wta_error("*", 2);
    return MK_FIXNUM(FIXNUM(x) * FIXNUM(y));
}

/* / n n */
SCM s_quotient(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error("/", 1);
    if (!IS_FIXNUM(y))
        wta_error("/", 2);
    return(MK_FIXNUM(FIXNUM(x) / FIXNUM(y)));
}

/* 1+ n */
SCM s_oneplus(SCM x) {
    if (!IS_FIXNUM(x))
        wta_error("1+", 1);
    return (MK_FIXNUM(FIXNUM(x) + 1));
}

/* -1+ n */
SCM s_minusoneplus(SCM x) {
    if (!IS_FIXNUM(x))
        wta_error("-1+", 1);
    return (MK_FIXNUM(FIXNUM(x) - 1));
}

/* = n n */
SCM s_numequal(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error("=", 1);
    if (!IS_FIXNUM(y))
        wta_error("=", 2);
    return FIXNUM(x) == FIXNUM(y) ? boolean_true : boolean_false;
}

/* < n n */
SCM s_lessthan(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error("<", 1);
    if (!IS_FIXNUM(y))
        wta_error("<", 2);
    return FIXNUM(x) < FIXNUM(y) ? boolean_true : boolean_false;
}

/* <= n n */
SCM s_lessequal (SCM x, SCM y) {
    if (!IS_FIXNUM(x)) wta_error ("<=", 1);
    if (!IS_FIXNUM(y)) wta_error ("<=", 2);
    return FIXNUM(x) <= FIXNUM(y) ? boolean_true : boolean_false;
}

/* > n n */
SCM s_greaterthan(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error(">", 1);
    if (!IS_FIXNUM(y))
        wta_error(">", 2);
    return FIXNUM(x) > FIXNUM(y) ? boolean_true : boolean_false;
}

/* >= n n */
SCM s_greaterequal(SCM x, SCM y) {
    if (!IS_FIXNUM(x))
        wta_error(">=", 1);
    if (!IS_FIXNUM(y))
        wta_error(">=", 2);
    return FIXNUM(x) >= FIXNUM(y) ? boolean_true : boolean_false;
}

SCM s_string_to_number(SCM s) {
    if (!IS_STRING(s))
        wta_error("string->number", 1);
    return MK_FIXNUM(atoi(STR_DATA(s)));
}

SCM s_number_to_string(SCM n) {
    if (!IS_FIXNUM(n))
        wta_error("number->string", 1);
    char *newstr;
    if ((newstr = (char *)malloc(32)) == NULL)
        fatal_error("malloc: s_number_to_string");
    sprintf(newstr, "%ld", FIXNUM(n));
    SCM x;
    NEWCELL(x, T_STRING);
    STR_DIM(x) = (unsigned int)strlen(newstr);
    STR_DATA(x) = newstr;
    return x;
}

/* Closure */

SCM s_closurep(SCM x) {
    return IS_CLOSURE(x) ? boolean_true : boolean_false;
}

SCM s_closure_body(SCM closure) {
    if (!IS_CLOSURE(closure))
        wta_error("closure-body", 1);
    return CDR(CLOSURE_CODE(closure));
}

SCM s_closure_vars(SCM closure) {
    if (!IS_CLOSURE(closure))
        wta_error("closure-vars", 1);
    return CAR(CLOSURE_CODE(closure));
}

SCM s_closure_env(SCM closure) {
    if (!IS_CLOSURE(closure))
        wta_error("closure-env", 1);
    return CLOSURE_ENV(closure);
}

/* FUNCTION */

SCM s_procedurep(SCM x) {
    return ((IS_CLOSURE(x) ||
             IS_SUBR0(x) ||
             IS_SUBR1(x) ||
             IS_SUBR2(x) ||
             IS_SUBR3(x) ||
             IS_SUBRN(x) ||
             IS_FSUBR(x)) ? boolean_true : boolean_false);
}

/* ENVIRONMENT */

SCM s_environmentp(SCM x) {
    return IS_ENV(x) ? boolean_true : boolean_false;
}

SCM s_the_environment(SCM args, SCM env) {
    SCM x;
    NEWCELL(x, T_ENV);
    ENV(x) = env;
    return x;
}

SCM s_listify_environment(SCM env) {
    if (!IS_ENV(env))
        wta_error("listify-environment", 1);
    return ENV(env);
}

void init_subrs(void) {
    /* Any */
    mk_subr("EQ?", (SCM (*)(void))s_eq, 2);
    mk_subr("NEQ?", (SCM (*)(void))s_neq, 2);

    /* Null & Pairs */
    mk_subr("PAIR?", (SCM (*)(void))s_pairp, 1);
    mk_subr("NULL?", (SCM (*)(void))s_nullp, 1);
    mk_subr("LIST?", (SCM (*)(void))s_listp, 1);
    mk_subr("CONS", (SCM (*)(void))mk_pair, 2);
    mk_subr("CAR", (SCM (*)(void))s_car, 1);
    mk_subr("CDR", (SCM (*)(void))s_cdr, 1);
    mk_subr("SET-CAR!", (SCM (*)(void))s_setcar, 2);
    mk_subr("SET-CDR!", (SCM (*)(void))s_setcdr, 2);
    mk_subr("LIST", (SCM (*)(void))n_list, -1);
    mk_subr("LENGTH", (SCM (*)(void))s_length, 1);
    mk_subr("MEMQ", (SCM (*)(void))s_memq, 2);
    mk_subr("LAST", (SCM (*)(void))s_last, 1);
    mk_subr("REC-APPEND", (SCM (*)(void))s_rec_append, 2);

    /* char */
    mk_subr("CHAR?", (SCM (*)(void))s_charp, 1);
    mk_subr("CHAR=?", (SCM (*)(void))s_char_equal, 2);
    mk_subr("CHAR<?", (SCM (*)(void))s_char_lessthan, 2);
    mk_subr("CHAR<=?", (SCM (*)(void))s_char_lessequal, 2);
    mk_subr("CHAR>?", (SCM (*)(void))s_char_greaterthan, 2);
    mk_subr("CHAR>=?", (SCM (*)(void))s_char_greaterequal, 2);

    /* BOOL */
    mk_subr("BOOLEAN?", (SCM (*)(void))s_booleanp, 1);
    mk_subr("NOT", (SCM (*)(void))s_not, 1);

    /* SYMBOL */
    mk_subr("SYMBOL?", (SCM (*)(void))s_symbolp, 1);
    mk_subr("SYMBOL->STRING", (SCM (*)(void))s_symbol_to_string, 1);
    mk_subr("STRING->SYMBOL", (SCM (*)(void))s_string_to_symbol, 1);

    /* String */
    mk_subr("STRING?", (SCM (*)(void))s_stringp, 1);
    mk_subr("STRING-APPEND", (SCM (*)(void))s_string_append, -1);

    /* Fixnum */
    mk_subr("NUMBER?", (SCM (*)(void))s_numberp, 1);
    mk_subr("ZERO?", (SCM (*)(void))s_zerop, 1);
    mk_subr("+", (SCM (*)(void))s_plus, 2);
    mk_subr("-", (SCM (*)(void))s_minus, 2);
    mk_subr("*", (SCM (*)(void))s_times, 2);
    mk_subr("/", (SCM (*)(void))s_quotient, 2);
    mk_subr("1+", (SCM (*)(void))s_oneplus, 1);
    mk_subr("-1+", (SCM (*)(void))s_minusoneplus, 1);
    mk_subr("=", (SCM (*)(void))s_numequal, 2);
    mk_subr("<", (SCM (*)(void))s_lessthan, 2);
    mk_subr("<=", (SCM (*)(void))s_lessequal, 2);
    mk_subr(">", (SCM (*)(void))s_greaterthan, 2);
    mk_subr(">=", (SCM (*)(void))s_greaterequal, 2);
    mk_subr("STRING->NUMBER", (SCM (*)(void))s_string_to_number, 1);
    mk_subr("NUMBER->STRING", (SCM (*)(void))s_number_to_string, 1);

    /* Closure */
    mk_subr("CLOSURE?", (SCM (*)(void))s_closurep, 1);
    mk_subr("CLOSURE-BODY", (SCM (*)(void))s_closure_body, 1);
    mk_subr("CLOSURE-VARS", (SCM (*)(void))s_closure_vars, 1);
    mk_subr("CLOSURE-ENV", (SCM (*)(void))s_closure_env, 1);

    /* Function */
    mk_subr("PROCEDURE?", (SCM (*)(void))s_procedurep, 1);

    /* Environment */
    mk_subr("ENVIRONMENT?", (SCM (*)(void))s_environmentp, 1);
    mk_fsubr("THE-ENVIRONMENT", (SCM (*)(void))s_the_environment);
    mk_subr("LISTIFY-ENVIRONMENT", (SCM (*)(void))s_listify_environment, 1);

    /* Special */
    mk_subr("SYS:EVAL", (SCM (*)(void))evaluate, 2);
}
