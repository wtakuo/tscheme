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
#include <ctype.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>

#include "tscheme.h"

static SCM do_readr(FILE *fp);
static SCM do_readparen(FILE *fp);
static SCM do_readstring(FILE *fp);
static SCM do_readtoken(FILE *fp);
static int skip_spaces(FILE *f, char *eoferr);
static int is_number_str(char *s);

static char strbuf[STRBUF_SIZE];

SCM n_read(SCM args) {
    if (check_nargs("read", args, 0, 1) == 0)
        return scm_read(stdin_value);
    else
        return scm_read(FIRST(args));
}

SCM scm_read(SCM port) {
    if (!IS_PORT(port))
        wta_error("read", 1);
    return do_read(PORT_FPTR(port));
}

SCM do_read(FILE *fp) {
    int c = skip_spaces(fp, (char *)NULL);
    if (c == EOF)
        return eof_value;
    ungetc(c, fp);
    return do_readr(fp);
}

static SCM do_readr(FILE *fp) {
    int c;
    SCM v;

    switch (c = skip_spaces (fp, "Unexpected EOF")) {
    case '(':
        return do_readparen(fp);
    case ')':
        error0 ("unexpexted close paren");
        return unspecified_value;
    case '#':
        if ((c = getc(fp)) == EOF)
            error0("unexpected EOF");
        switch (c) {
        case 't':
        case 'T':
            return boolean_true;
        case 'f':
        case 'F':
            return boolean_false;
        case '\\': 
            if ((c = getc(fp)) == EOF)
                error0("unexpected EOF");
            NEWCELL(v, T_CHARACTER);
            v->as.character = c;
            return v;
        default:
            error0("syntax");
        }
    case '\'':
        return mk_pair(sym_quote, mk_pair(do_readr(fp), NIL));
    case '`':
        return mk_pair(sym_quasiquote, mk_pair(do_readr(fp), NIL));
    case ',':
        if ((c = getc(fp)) == EOF)
            error0("unexpected EOF");
        switch (c) {
        case '@':
            return mk_pair(sym_unquote_splicing,
                           mk_pair(do_readr(fp), NIL));
        default:
            ungetc(c, fp);
            return mk_pair(sym_unquote, mk_pair(do_readr(fp), NIL));
        }
    case '"':
        return do_readstring(fp);
    default:
        ungetc(c, fp);
        return do_readtoken(fp);
    }
}

static SCM do_readparen(FILE *fp) {
    int c = skip_spaces(fp, "Unexpected EOF inside list");
    if (c == ')')
        return NIL;
    ungetc(c,fp);
    SCM tmp = do_readr(fp);
    if (EQ(tmp, sym_dot)) {
        tmp = do_readr(fp);
        c = skip_spaces(fp, "Unexpected EOF inside list");
        if (c != ')')
            error0("missing closing paren");
        return tmp;
    }
    return mk_pair(tmp, do_readparen(fp));
}

static SCM do_readstring(FILE *fp) {
    char c, *newstr;
    int i = 0;
    while ((c = getc(fp)) != EOF) {
        switch (c) {
        case '\\':
            c = getc(fp);
            if (c == EOF)
                error0("Unexpected EOF");
            switch (c) {
            case 'n':
                strbuf[i] = '\n';
                break;
            default:
                strbuf[i] = c;
                break;
            }
            break;
        case '"':
            if ((newstr = (char *)malloc(i+1)) == NULL)
                fatal_error("malloc: read");
            strbuf[i] = '\0';
            strcpy(newstr, strbuf);
            return mk_string(newstr,i);
        default:
            strbuf[i] = c;
            break;
        }
        i++;
    }
    error0("Unextected EOF");
    return unspecified_value;
}

static SCM do_readtoken(FILE *fp) {
    char c;
    int i = 0;
    while ((c = getc(fp)) != EOF) {
        if (i >= STRBUF_SIZE)
            fatal_error("I/O buffer size exceeded");
        switch (c) {
        case '(':
        case ')':
        case '\'':
        case '"':
        case '`':
        case ',':
        case ' ':
        case '\t':
        case '\n':
            ungetc(c,fp);
            strbuf[i] = '\0';
            if (is_number_str(strbuf)) {
                errno = 0;
                long n = strtol(strbuf, NULL, 10);
                if (errno == ERANGE)
                    error1("read: number out of range: %s\n", strbuf);
                return (SCM)MK_FIXNUM(n);
            }
            else
                return mk_symbol(strbuf);
        default:
            strbuf[i] = islower(c) ? toupper(c) : c;
            break;
        }
        i++;
    }
    error0("Unexptected EOF");
    return unspecified_value;
}


static int skip_spaces(FILE *f, char *eoferr) {
    int commentp = 0;
    while (1) {
        int c = getc(f);
        if (c == EOF) {
            if (eoferr)
                error0(eoferr);
            else
                return c;
        }
        if (commentp) {
            if (c == '\n')
                commentp = 0;
        }
        else if (c == ';')
            commentp = 1;
        else if (!isspace(c))
            return c;
    }
    return 0; /* dummy */
}

static int is_number_str(char *s) {
    char *p = s;
    if (p[0] == '-')
        p++;
    if (p[0] == '\0')
        return 0;
    while (p[0] != '\0') {
        if (p[0] < '0' || p[0] > '9')
            return 0;
        p++;
    }
    return 1;
}
