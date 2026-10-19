/* lisp.h -- shared declarations of the Lisp interpreter (C89)

   This is a C version of the interpreter in ../lisp.crexx.  The
   behaviour must be the same; the cREXX source is the specification.
   The procedures keep their cREXX names where that is possible.

   RULES FOR ALL SOURCE FILES
   - strict C89 (no //, no declarations after statements, no long
     long, no snprintf, no inline); compile with -std=c89 -pedantic
   - NO LINE LONGER THAN 71 COLUMNS (make lines checks it)
   - every Lisp object is a Ref (a long), like in the cREXX version:
       ref < 0   a fixnum:  n >= 0 is -(2n+1), n < 0 is 2n
       ref > 0   a cell in the heap arrays
       ref 1     NIL        ref 2   T        ref 0   "none"
   - the heap arrays are reallocated when they grow: NEVER write
        ccar[x] = f();    when f can allocate a cell.
     Use a temporary:  t = f(); ccar[x] = t;
*/
#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

typedef long Ref;

/* the extra word of a cell */
typedef union { long l; double f; char *s; void *p; } Ext;

/* cell tags */
#define T_FREE   0
#define T_CONS   1
#define T_SYM    2
#define T_FLOAT  3
#define T_STR    4
#define T_PRIM   5
#define T_CLOS   6
#define T_MACRO  7
#define T_CHAR   8
#define T_VEC    9
#define T_HASH   10
#define T_STRUCT 11
#define T_STYPE  13

#define NIL 1L
#define TRU 2L

/* the heap: tag, two refs and one extra word per cell */
extern unsigned char *ctag;   /* cell type                         */
extern unsigned char *csid;   /* symbol: special form id           */
extern unsigned char *cspec;  /* symbol: 1 special 2 const 3 gensym */
extern Ref *ccar;  /* car / symbol value / closure parameters      */
extern Ref *ccdr;  /* cdr / symbol function / closure body         */
extern Ext *cext;  /* env, prim id, char code, float, string, ...  */
extern long ncell;

/* a symbol's cold data, found through cext[sym].l */
typedef struct Sym {
    char *name;
    long len;
    Ref chn;       /* next symbol in the hash chain */
    Ref cfk;       /* cached local-function key symbol */
    Ref cpl;       /* property list (0 none) */
    long cslot;    /* structure accessor: slot index + 1 */
} Sym;
extern Sym *syms;

#define XL(r)    (cext[r].l)
#define XF(r)    (cext[r].f)
#define XS(r)    (cext[r].s)
#define XP(r)    (cext[r].p)
#define SYM(r)   (syms[cext[r].l])
#define SYMNAME(r) (syms[cext[r].l].name)
#define SLEN(r)  (ccdr[r])      /* string length */

#define ISCONS(r) ((r) > 0 && ctag[r] == T_CONS)
#define ISSYM(r)  ((r) > 0 && ctag[r] == T_SYM)
#define ISSTR(r)  ((r) > 0 && ctag[r] == T_STR)
#define ISFLOAT(r) ((r) > 0 && ctag[r] == T_FLOAT)
#define ISCHAR(r) ((r) > 0 && ctag[r] == T_CHAR)
#define ISNUM(r)  ((r) < 0 || ((r) > 0 && ctag[r] == T_FLOAT))
#define ISFIX(r)  ((r) < 0)
#define TRUTH(b)  ((b) ? TRU : NIL)

/* integer arithmetic as in the cREXX VM: a product that does not
   fit gives 0 (and an assignment of it changes nothing) */
#define IADD(a, b) ((long)((unsigned long)(a) + (unsigned long)(b)))
#define ISUB(a, b) ((long)((unsigned long)(a) - (unsigned long)(b)))
long imul(long a, long b);
#define IMUL(a, b) imul(a, b)

/* --- memory (core.c) */
void *xmalloc(long n);
void *xrealloc(void *p, long n);
char *xstrdup(const char *z);
char *xstrndup(const char *p, long n);

/* growing string buffer */
typedef struct Sb { char *p; long n, cap; } Sb;
void sb_init(Sb *b);
void sb_addc(Sb *b, int c);
void sb_addn(Sb *b, const char *s, long n);
void sb_adds(Sb *b, const char *s);
void sb_addl(Sb *b, long v);
void sb_clear(Sb *b);
char *sb_str(Sb *b);       /* NUL terminated, owned by the buffer */

/* --- cells and objects (core.c) */
Ref newcell(int tag);
void freecell(Ref r);
Ref mkint(long n);
long intval(Ref r);
Ref mkfloat(double x);
Ref mkstrn(const char *s, long n);
Ref mkstr(const char *s);
Ref mkchar(long code);
Ref cons(Ref a, Ref d);
Ref list1(Ref a);
Ref list2(Ref a, Ref b);
Ref list3(Ref a, Ref b, Ref c);
Ref car(Ref r);
Ref cdr(Ref r);
long listlen(Ref l);
Ref arr2list(Ref *av, long from, long n);
Ref *list2arr(Ref l, long *n);   /* xmalloc'ed */
Ref reverse(Ref l);
Ref nreverse(Ref l);
Ref append2(Ref a, Ref b);
Ref intern(const char *name);
Ref mksym(const char *name);
Ref newsym(const char *name);
double fval(Ref r);
Ref makevec(long n);
Ref mkvec(Ref *a, long n);
Ref mkvecn(long n, Ref init);
Ref *vdata(Ref v);
long ncount(Ref l);
char *upstr(char *s, long n);
char *lowstr(char *s, long n);

/* --- errors (core.c) */
extern int unw;        /* 0 none, 1 error, 2 throw, 3 return-from */
extern Ref unwtag, unwval;
extern char *errmsg;
extern const char *errkind;
Ref lerror(const char *msg);
Ref lerrk(const char *kind, const char *msg);
Ref lerrobj(const char *kind, const char *pre, Ref o,
            const char *post);
Ref notnum(Ref a);

/* --- globals (core.c) */
extern int outcol, hasfenv, capdepth;
extern long ndepth, maxdepth, nclos;
extern long gensymn, seed;
extern Ref mvlist, curfn, catchtags;
extern Sb capbuf;
extern int eofflag, fidx, fstop;
extern Ref sQUOTE, sFUNCTION, sQUASI, sUNQ, sUNQS, sLAMBDA, sOPT,
    sREST, sKEY, sAUX, sBODY, sELSE, sERROR, sTHEN, sOTHERWISE,
    sBLOCK, sRETFROM, sRETURN, sXLOOP;

/* --- arithmetic and equality (num.c) */
Ref numadd(Ref a, Ref b);
Ref numsub(Ref a, Ref b);
Ref nummul(Ref a, Ref b);
Ref numneg(Ref a);
Ref numdiv(Ref a, Ref b);
int numcmp(Ref a, Ref b);
long toint(Ref a);
int eql(Ref a, Ref b);
int equal(Ref a, Ref b);
int equalp(Ref a, Ref b);
double lsqrt(double x);
double lsin(double x);
double lcos(double x);
double latan(double x);
double lln(double x);
double lexp(double x);
long ipow(long b, long e);
long nextrand(void);

/* --- printer (print.c) */
void floattext(Sb *o, double x);
void pr_obj(Sb *o, Ref r, int esc);
char *printobj(Ref r, int esc);      /* malloc'ed */
void printstring(Sb *o, const char *s, long n);

/* --- reader (read.c) */
extern char *src;
extern long rp, rlen;
Ref readobj(void);
Ref readlist(void);
void skipws(void);

/* --- output (io.c) */
void emit(const char *s, long n);
void emits(const char *s);
void freshline(void);
void reporterr(void);
void repl(void);
int lines(void);
char *linein(void);          /* malloc'ed, no newline */
int runsource(const char *text, long n);
int loadfile(const char *fname);

/* --- evaluator (eval.c, special.c) */
Ref eval(Ref x, Ref env);
Ref symval(Ref s, Ref env);
Ref setvar(Ref s, Ref v, Ref env);
Ref getfn(Ref f, Ref env);
Ref fnof(Ref x);
Ref mkclosure(Ref params, Ref body, Ref env);
Ref bindparams(Ref params, Ref *av, long n, Ref cenv);
Ref applyfn(Ref fn, Ref *av, long n);
Ref call1(Ref fn, Ref a);
Ref call2(Ref fn, Ref a, Ref b);
Ref evalbody(Ref body, Ref env);
Ref evalprefix(Ref body, Ref env);
Ref evalspecial(int sid, Ref x, Ref env);
Ref qq(Ref x, Ref env);
Ref destruct(Ref pat, Ref val, Ref env);
Ref setplace(Ref pl, Ref v, Ref env);
Ref simpleloop(Ref body, Ref env);
Ref blockwrap(Ref name, Ref body);
Ref stripdoc(Ref body);
Ref defstruct(Ref x, Ref env);
int condmatch(Ref ty);
Ref macroexp1(Ref x);
Ref undyn(Ref d);
void msort(Ref *a, long n, Ref pf, Ref kf);
long chr(Ref x);
long f2i(double t);
void freeframe(Ref top, Ref bottom);

/* environment frames a running eval owns: top down to bottom */
typedef struct { Ref top, bottom; long n0; } Seg;
void heapinit(void);

/* --- tables (table.c) */
Ref mkhash(int mode);
Ref htfind(Ref t, Ref k);          /* entry or 0 */
Ref htget(Ref t, Ref k, int *found);
void htput(Ref t, Ref k, Ref v);
int htrem(Ref t, Ref k);
void htclear(Ref t);
long htkeys(Ref t, Ref **ks, Ref **vs);
Ref getprop(Ref s, Ref k);
void putprop(Ref s, Ref k, Ref v);
int typep(Ref o, Ref ty);
Ref lobjtype(Ref o);

/* --- format (fmt.c) */
void formatargs(Sb *o, Ref *av, long from, long n);
void capitalize(Sb *o, const char *s, long n);
int ok_format(Ref *av, long from);

/* --- primitives (prim*.c) */
typedef Ref (*PrimFn)(Ref *av, long n);
extern PrimFn primtab[];
extern int pmin[], pmax[];
#define NPRIM 224
Ref callprim(int id, Ref *av, long n);
Ref fastprim(int id, Ref *av, long n);
void initprims(void);
void registerprim(const char *name, int id, int mn, int mx);
void initprim_a(void);
void initprim_b(void);
void initprim_c(void);
void initprim_d(void);
void initprim_e(void);
Ref pidof(const char *name);

/* helpers shared by the primitives (prim_seq.c) */
Ref *seq2arr(Ref s, long *n);    /* xmalloc'ed */
Ref arr2seq(Ref proto, Ref *a, long n);
long kwidx(Ref *av, long n, long from, const char *kname);
Ref kwfn(Ref *av, long n, long from, const char *kname);
int same(Ref tf, Ref a, Ref b);
Ref keyed(Ref kf, Ref x);
const char *strof(Ref x, long *len, char *cb);
Ref subseqn(Ref s, long st, long en);
Ref copytree(Ref x);

/* --- start up (main.c) */
void initlisp(void);
void initprelude(void);
extern const char *prelude[];

#endif
