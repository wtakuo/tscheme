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

#include <stdint.h>

#define BANNER "Tscheme\n\n"

#define PROMPT "> "

#define DEFAULT_INITFILE "init.scm"

#define YES 1
#define NO  0

#define NON_FATAL 1
#define FATAL -1

#define DEFAULT_NUMCELLS 100000
#define DEFAULT_OBARRAY_SIZE 512
#define STRBUF_SIZE 2048

/* *** Assumption ***

   1 word = 8 bytes = 64 bits

   pointer width          = 64 bits
   unsigned number length = 64 bits
   short numbe length     = 16 bits
   addressing             = byte adressing with 8 bytes boundary

   Pointer/integer conversions use uintptr_t/intptr_t (stdint.h) so that
   the tagging scheme below works regardless of pointer width (32 or 64
   bits): only the low ITYP_BITS bits of a pointer value are assumed to
   be 0, which malloc's alignment guarantees on both.

*/

/* data type implementations

   object pointer   : PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP00
   immediate fixnum : NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN01

*/

#define ITYP_BITS   2
#define ITYP_FIXNUM ((uintptr_t)1)
#define ITYP_MASK   ((uintptr_t)3)

/* Type tags */

enum {
    T_NULL = 0,
    T_FIXNUM,
    T_BOOLEAN,
    T_CHARACTER,
    T_PAIR,
    T_FREE_CELL,
    T_SYMBOL,
    T_STRING,
    T_SUBR0,
    T_SUBR1,
    T_SUBR2,
    T_SUBR3,
    T_SUBRN,
    T_FSUBR,
    T_CLOSURE,
    T_ENV,
    T_PORT,
    T_EOF_VALUE
};

struct object {

    /* GC & Type tags (16 bits) */
    unsigned short gc_tags, type_tags;

    /* Data */
    union {
        /* Boolean */
        int boolean;

        /* Char */
        int character;

        /* Null */
        struct object *null;

        /* Pairs */
        struct { struct object *car, *cdr; } pair;

        /* Symbols */
        struct { struct object *pname, *value; } symbol;

        /* Strings */
        struct { long dim; char *data; } string;

        /* Subrs (functions) */
        struct { struct object *name, *(*fun)(void); } subr;	    

        /* Closures */
        struct { struct object *env, *code; } closure;

        /* Environment */
        struct object *env;

        /* input/output port */
        struct { char *name; FILE *fptr; } port;

        /* EOF value */
        int eof_value;
    } as;
};

typedef struct object* SCM;

#define EQ(x,y)       ((uintptr_t)(x) == (uintptr_t)(y))
#define NEQ(x,y)      (!(EQ (x,y)))

#define IMM_TYPE(x) ((uintptr_t)(x) & ITYP_MASK)
#define IS_IMM(x)   (IMM_TYPE(x)!=0)

#define BOXED_TYPE(x)       ((unsigned)((x)->type_tags))
#define IS_BOXED_TYPE(x,t)  (BOXED_TYPE(x)==(unsigned)(t))
#define SET_BOXED_TYPE(x,t) ((x)->type_tags=(unsigned)(t))

#define TYPE(x)            (IS_IMM(x)?IMM_TYPE(x):BOXED_TYPE(x))
#define IS_TYPE(x,t)       (TYPE(x)==(unsigned)(t))

#define IS_FIXNUM(x) (((uintptr_t)(x))&ITYP_FIXNUM)
#define MK_FIXNUM(n) ((SCM)((((uintptr_t)(n))<<ITYP_BITS)|ITYP_FIXNUM))
#define FIXNUM(x)    ((long)(((intptr_t)(x))>>ITYP_BITS))

#define IS_BOOLEAN(x)      IS_TYPE(x,T_BOOLEAN)
#define BOOLEAN(x) ((x)->as.boolean)

#define IS_CHARACTER(x) IS_TYPE(x, T_CHARACTER)
#define CHARACTER(x)    ((x)->as.character)

#define IS_NULL(x) EQ(x, the_null_value)
#define NIL the_null_value

#define IS_PAIR(x) IS_TYPE(x,T_PAIR)
#define CONS(x,y)  mk_pair(x,y)
#define CAR(x)     ((x)->as.pair.car)
#define CDR(x)     ((x)->as.pair.cdr)
#define CAAR(x)    (CAR(CAR(x)))
#define CADR(x)    (CAR(CDR(x)))
#define CDAR(x)    (CDR(CAR(x)))
#define CDDR(x)    (CDR(CDR(x)))
#define CADDR(x)   (CAR(CDDR(x)))
#define FIRST(x)   (CAR(x))
#define SECOND(x)  (CADR(x))
#define THIRD(x)   (CADDR(x))
#define FOURTH(x)  (CADDR(CDR(x)))

#define IS_SYMBOL(x) IS_TYPE(x,T_SYMBOL)
#define SYM_PNAME(x) ((x)->as.symbol.pname)
#define SYM_VALUE(x) ((x)->as.symbol.value)

#define IS_STRING(x) IS_TYPE(x,T_STRING)
#define STR_DIM(x) ((x)->as.string.dim)
#define STR_DATA(x) ((x)->as.string.data)

#define IS_SUBR0(x) IS_TYPE(x,T_SUBR0)
#define IS_SUBR1(x) IS_TYPE(x,T_SUBR1)
#define IS_SUBR2(x) IS_TYPE(x,T_SUBR2)
#define IS_SUBR3(x) IS_TYPE(x,T_SUBR3)
#define IS_SUBRN(x) IS_TYPE(x,T_SUBRN)
#define IS_FSUBR(x) IS_TYPE(x,T_FSUBR)
#define SUBR_NAME(x) ((x)->as.subr.name)
#define SUBR_FUN(x) ((x)->as.subr.fun)

#define IS_CLOSURE(x) IS_TYPE(x,T_CLOSURE)
#define CLOSURE_ENV(x) ((x)->as.closure.env)
#define CLOSURE_CODE(x) ((x)->as.closure.code)

#define IS_ENV(x) IS_TYPE(x,T_ENV)
#define ENV(x) ((x)->as.env)

#define IS_PORT(x)   IS_TYPE(x,T_PORT)
#define PORT_NAME(x) ((x)->as.port.name)
#define PORT_FPTR(x) ((x)->as.port.fptr)

#define IS_EOF_VALUE(x) IS_TYPE(x,T_EOF_VALUE)
#define EOF_VALUE(x) ((x)->as.eof_value)

#define IS_FREE_CELL(x) IS_TYPE(x,T_FREE_CELL)


/* Garbage collection */

#define GC_TAGS(x)  ((x)->gc_tags)
#define MARK(x)     (GC_TAGS(x) = (unsigned short)1)
#define UNMARK(x)   (GC_TAGS(x) = (unsigned short)0)
#define MARKED(x)   (GC_TAGS(x) == (unsigned short)1)
#define UNMARKED(x) (GC_TAGS(x) == (unsigned short)0)

#define NEWCELL(_place, _type)                  \
    { if IS_NULL(free_list) gc();               \
        _place = free_list;                     \
        free_list = CDR(free_list);             \
        UNMARK(_place);                         \
        SET_BOXED_TYPE(_place, _type);          \
    }

/* external variable declarations */

/* error.c */
extern jmp_buf error_return;

/* storage.c */
/* extern SCM heap_start, heap_end; */
extern SCM free_list;
extern SCM stack_start;
extern SCM obarray[];
extern long obarray_dim;
extern SCM the_null_value, boolean_true, boolean_false;
extern SCM unbound_value, unspecified_value, 
    stdin_value, stdout_value, stderr_value, eof_value,
    sym_quote, sym_quasiquote, sym_unquote, sym_unquote_splicing,
    sym_lambda, sym_and, sym_or, sym_let, sym_let_star, sym_letrec,
    sym_begin, sym_do, sym_delay, sym_if, sym_cond, sym_case, sym_else,
    sym_set, sym_define, sym_dot;
extern SCM sym_toplevel;

/* function prototypes */

/* main.c */
void interrupt_handler(int sig);

/* storage.c */
void gc(void);
void init_storage(unsigned heapsize);
void show_obarray(void);

/* object.c */
SCM mk_pair(SCM car, SCM cdr);
SCM mk_string(char *string, long dim);
SCM newsym(SCM pname, SCM value);
SCM mk_symbol(char *name);
SCM mk_subr(char *name, SCM (*fun)(void), int nargs);
SCM mk_fsubr(char *name, SCM (*fun)(void));
SCM mk_closure(SCM args, SCM code, SCM env);

/* subrs.c */

void init_subrs(void);

/* eval.c */

SCM evaluate(SCM exp, SCM env);

/* error.c */

void wta_error(char *fname, int argno);
void wna_error(char *fname, int nargs);
void fatal_error(char *message);
void error0(char *message);
void error1(char *message, char *arg);
int check_nargs(char *fname, SCM args, int min, int max);

/* io.c */

SCM scm_write(SCM data, SCM port, int displayp);
void do_load(char *file);
void do_load_if_exists(char *file);
void init_io_subrs(void);

/* read.c */

SCM n_read(SCM args);
SCM scm_read(SCM port);
SCM do_read(FILE *fp);

/* misc.c */

int memq(SCM key, SCM list);
SCM map1(SCM (*f)(SCM), SCM list);

/* EOF */
