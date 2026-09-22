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

#include "tscheme.h"

static void do_write(SCM x, FILE *fp, int displayp);
static void do_write_pair(SCM x, FILE *fp, int displayp);

SCM s_open_input_file(SCM file) {
    SCM port;
    FILE *fp;

    if (!IS_STRING(file))
        wta_error("open-input-file", 1);

    if ((fp = fopen(STR_DATA(file), "r"))) {
        NEWCELL(port, T_PORT);
        PORT_NAME(port) = STR_DATA(file);
        PORT_FPTR(port) = fp;
        return port;
    }
    else {
        error1("Cannot open file %s\n", STR_DATA(file));
        return unspecified_value; /* dummy */
    }
}

SCM s_open_output_file(SCM file) {
    SCM port;
    FILE *fp;

    if (!IS_STRING(file))
        wta_error("open-output-file", 1);

    if ((fp = fopen(STR_DATA(file), "w"))) {
        NEWCELL(port, T_PORT);
        PORT_NAME(port) = STR_DATA(file);
        PORT_FPTR(port) = fp;
        return port;
    }
    else {
        error1("Cannot open file: %s\n", STR_DATA(file));
        return unspecified_value; /* dummy */
    }
}

SCM s_close_input_port(SCM port) {
    if (!IS_PORT(port))
        wta_error("close-input-port", 1);
    fclose(PORT_FPTR(port));
    return unspecified_value;
}

SCM s_close_output_port(SCM port) {
    if (!IS_PORT(port))
        wta_error("close-output-port", 1);
    fclose(PORT_FPTR(port));
    return unspecified_value;
}


SCM n_write(SCM args) {
    if (check_nargs("write", args, 1, 2) == 1)
        return scm_write(FIRST(args), stdout_value, 0);
    else
        return scm_write(FIRST(args), SECOND(args), 0);
}

SCM n_display(SCM args) {
    if (check_nargs("display", args, 1, 2) == 1)
        return scm_write(FIRST(args), stdout_value, 1);
    else
        return scm_write(FIRST(args), SECOND(args), 1);
}

SCM n_newline(SCM args) {
    SCM port;
    if (check_nargs("newline", args, 0, 1) == 0)
        putchar('\n');
    else {
        port = FIRST(args);
        if (!IS_PORT(port))
            wta_error("newline", 1);
        fputc('\n', PORT_FPTR(port));
    }
    return unspecified_value;
}


SCM s_eof_objectp(SCM x) {
    return EQ(x,eof_value) ? boolean_true : boolean_false;
}


SCM scm_write(SCM data, SCM port, int displayp) {
    FILE *fp;
    if (!IS_TYPE(port, T_PORT))
        wta_error((displayp ? "display" : "write"), 2);
    fp = PORT_FPTR(port);
    do_write(data, fp, displayp);
    return unspecified_value;
}

SCM s_load(SCM file) {
    if (!IS_STRING(file))
        wta_error("load", 1);

    do_load(STR_DATA(file));
    return unspecified_value;
}

void do_load(char *file) {
    FILE *fp;
    if ((fp = fopen(file, "r")) == 0)
        error1("sys:load: cannot open file %s\n", file);
    else {
        fprintf(stderr, "Loading %s ... ", file);
        SCM e;
        while (NEQ((e = do_read (fp)), eof_value)) {
            evaluate(e, NIL);
            /* fputc ('.', stderr); */
        }
        fclose(fp);
        fprintf(stderr, "done!\n");
    }
}

void do_load_if_exists(char *file) {
    FILE *fp;
    if ((fp = fopen(file, "r")) != 0) {
        fprintf(stderr, "Loading %s ", file);
        SCM e;
        while (NEQ((e = do_read (fp)), eof_value)) {
            evaluate(e, NIL);
            fputc('.', stderr);
        }
        fclose(fp);
        fprintf(stderr, "done!\n");
    }
}

static void do_write(SCM x, FILE *fp, int displayp) {
    switch (TYPE(x)) {
    case T_FIXNUM:
        fprintf(fp, "%ld", FIXNUM(x));
        break;
    case T_BOOLEAN:
        if (EQ(x, boolean_true))
            fprintf(fp, "#t");
        else if (EQ(x, boolean_false))
            fprintf(fp, "#f");
        else
            error0("unknown bool value");
        break;
    case T_CHARACTER:
        fprintf(fp, "#\\%c", CHARACTER(x));
        break;
    case T_NULL:
        fprintf(fp, "()");
        break;
    case T_PAIR:
        do_write_pair(x, fp, displayp);
        break;
    case T_SYMBOL:
        fprintf(fp, "%s", STR_DATA(SYM_PNAME(x)));
        break;
    case T_STRING:
        fprintf(fp, (displayp ? "%s" : "\"%s\""), STR_DATA(x));
        break;
    case T_SUBR0:
    case T_SUBR1:
    case T_SUBR2:
    case T_SUBR3:
    case T_SUBRN:
        fprintf(fp, "#<subr %s>", STR_DATA(SYM_PNAME(SUBR_NAME(x))));
        break;
    case T_FSUBR:
        fprintf(fp, "#<fsubr %s>", STR_DATA(SYM_PNAME(SUBR_NAME(x))));
        break;
    case T_CLOSURE:
        fprintf(fp, "#<closure %lx>", (unsigned long)(uintptr_t)x);
        break;
    case T_ENV:
        fprintf(fp, "#<environment %lx>", (unsigned long)(uintptr_t)x);
        break;
    case T_PORT:
        fprintf(fp, "#<port %s>", PORT_NAME(x));
        break;
    case T_EOF_VALUE:
        fprintf(fp, "#<eof>");
        break;
    case T_FREE_CELL:
        error0("Why free-cell comes here?");
        break;
    default:
        error0("Write: unknown value");
        break;
    }
}

static void do_write_pair(SCM x, FILE *fp, int displayp) {
    SCM p;
    fputc ('(', fp);
    p = x;
 print_cdr_loop:
    do_write(CAR(p), fp, displayp);
    switch (TYPE(CDR(p))) {
    case T_NULL:
        fputc(')', fp);
        break;
    case T_PAIR:
        fputc(' ', fp);
        p = CDR(p);
        goto print_cdr_loop;
    default:
        fputs(" . ", fp);
        do_write(CDR(p), fp, displayp);
        fputc(')', fp);
    }
}

SCM s_show_obarray(void) {
    int i;
    for (i = 0; i < obarray_dim; i++) {
        if (!IS_NULL(obarray[i])) {
            printf("%3d : ", i);
            scm_write(obarray[i], stdout_value, 0);
            putchar('\n');
        }
    }
    return unspecified_value;
}

void init_io_subrs(void) {
    mk_subr("OPEN-INPUT-FILE", (SCM (*)(void))s_open_input_file, 1);
    mk_subr("OPEN-OUTPUT-FILE", (SCM (*)(void))s_open_output_file, 1);
    mk_subr("CLOSE-INPUT-PORT", (SCM (*)(void))s_close_input_port, 1);
    mk_subr("CLOSE-OUTPUT-PORT", (SCM (*)(void))s_close_output_port, 1);
    mk_subr("WRITE", (SCM (*)(void))n_write, -1);
    mk_subr("DISPLAY", (SCM (*)(void))n_display, -1);
    mk_subr("NEWLINE", (SCM (*)(void))n_newline, -1);
    mk_subr("EOF-OBJECT?", (SCM (*)(void))s_eof_objectp, 1);
    mk_subr("LOAD", (SCM (*)(void))s_load, 1);
    mk_subr("SHOW-OBARRAY", (SCM (*)(void))s_show_obarray, 0);

    /* For now, the following function is defined in a separate file */
    mk_subr("READ", (SCM (*)(void))n_read, -1);
}
