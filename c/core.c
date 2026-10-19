/* core.c -- memory, cells, symbols, lists, errors.
   C port of the heap and list helpers of lisp.crexx. */
#include "lisp.h"

/* ---------------------------------------------------------- memory */
void *xmalloc(long n)
{
    void *p;
    if (n < 1) n = 1;
    p = malloc((size_t)n);
    if (p == NULL) {
        fputs("lisp: out of memory\n", stderr);
        exit(3);
    }
    return p;
}

void *xrealloc(void *p, long n)
{
    void *q;
    if (n < 1) n = 1;
    q = p ? realloc(p, (size_t)n) : malloc((size_t)n);
    if (q == NULL) {
        fputs("lisp: out of memory\n", stderr);
        exit(3);
    }
    return q;
}

char *xstrndup(const char *p, long n)
{
    char *r = (char *)xmalloc(n + 1);
    if (n > 0) memcpy(r, p, (size_t)n);
    r[n] = 0;
    return r;
}

char *xstrdup(const char *z)
{
    return xstrndup(z, (long)strlen(z));
}

/* --------------------------------------------------- string buffer */
void sb_init(Sb *b)
{
    b->p = NULL;
    b->n = 0;
    b->cap = 0;
}

static void sb_grow(Sb *b, long need)
{
    long c = b->cap ? b->cap : 64;
    while (c < need + 1) c *= 2;
    b->p = (char *)xrealloc(b->p, c);
    b->cap = c;
}

void sb_addc(Sb *b, int c)
{
    if (b->n + 2 > b->cap) sb_grow(b, b->n + 2);
    b->p[b->n++] = (char)c;
}

void sb_addn(Sb *b, const char *s, long n)
{
    if (n <= 0) return;
    if (b->n + n + 1 > b->cap) sb_grow(b, b->n + n + 1);
    memcpy(b->p + b->n, s, (size_t)n);
    b->n += n;
}

void sb_adds(Sb *b, const char *s)
{
    sb_addn(b, s, (long)strlen(s));
}

void sb_addl(Sb *b, long v)
{
    char t[40];
    sprintf(t, "%ld", v);
    sb_adds(b, t);
}

void sb_clear(Sb *b)
{
    b->n = 0;
}

char *sb_str(Sb *b)
{
    if (b->cap == 0) sb_grow(b, 1);
    b->p[b->n] = 0;
    return b->p;
}

char *upstr(char *s, long n)
{
    long i;
    for (i = 0; i < n; i++)
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = (char)(s[i] - 32);
    return s;
}

char *lowstr(char *s, long n)
{
    long i;
    for (i = 0; i < n; i++)
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (char)(s[i] + 32);
    return s;
}

/* ------------------------------------------------------------ heap */
unsigned char *ctag, *csid, *cspec;
Ref *ccar, *ccdr;
Ext *cext;
long ncell = 0;
static long capcell = 0;
static Ref freelist = 0;
Sym *syms;
static long nsyms = 0, capsyms = 0;

int unw = 0;
Ref unwtag = 1, unwval = 1;
char *errmsg = NULL;
const char *errkind = "ERROR";
int outcol = 0, hasfenv = 0, capdepth = 0;
long ndepth = 0, maxdepth = 24000, nclos = 0;
long gensymn = 0, seed = 12345;
Ref mvlist = 0, curfn = 0, catchtags = 1;
Sb capbuf;
int eofflag = 0, fidx = 0, fstop = 0;
Ref sQUOTE, sFUNCTION, sQUASI, sUNQ, sUNQS, sLAMBDA, sOPT,
    sREST, sKEY, sAUX, sBODY, sELSE, sERROR, sTHEN, sOTHERWISE,
    sBLOCK, sRETFROM, sRETURN, sXLOOP;

static void growheap(void)
{
    long c = capcell ? capcell * 2 : 65536;
    ctag = (unsigned char *)xrealloc(ctag, c);
    csid = (unsigned char *)xrealloc(csid, c);
    cspec = (unsigned char *)xrealloc(cspec, c);
    ccar = (Ref *)xrealloc(ccar, c * (long)sizeof(Ref));
    ccdr = (Ref *)xrealloc(ccdr, c * (long)sizeof(Ref));
    cext = (Ext *)xrealloc(cext, c * (long)sizeof(Ext));
    capcell = c;
}

void heapinit(void)
{
    growheap();
    ctag[0] = T_FREE;
    ccar[0] = NIL;
    ccdr[0] = NIL;
    cext[0].l = 0;
    csid[0] = 0;
    cspec[0] = 0;
}

Ref newcell(int tag)
{
    Ref r;
    if (freelist) {
        r = freelist;
        freelist = ccdr[r];
    } else {
        if (ncell + 2 >= capcell) growheap();
        r = ++ncell;
    }
    ctag[r] = (unsigned char)tag;
    ccar[r] = 1;
    ccdr[r] = 1;
    cext[r].l = 0;
    csid[r] = 0;
    cspec[r] = 0;
    return r;
}

/* give a cell back; only for cells nobody can still refer to */
void freecell(Ref r)
{
    ctag[r] = T_FREE;
#ifdef POISON                     /* debugging: never reuse */
    ccar[r] = 0;
    ccdr[r] = 0;
    return;
#endif
    ccar[r] = 1;
    ccdr[r] = freelist;
    freelist = r;
}

/* give back the environment frames from top down to (not
   including) bottom: pairs and the cells that chain them */
void freeframe(Ref top, Ref bottom)
{
    Ref nx;
    while (top != bottom && top > 1) {
        nx = ccdr[top];
        freecell(ccar[top]);
        freecell(top);
        top = nx;
    }
}

/* a fixnum is a negative ref: n >= 0 is -(2n+1), n < 0 is 2n */
Ref mkint(long n)
{
    /* in the cREXX VM an overflowing 2*n is 0 */
    if (n > LONG_MAX / 2) return -1;
    if (n < LONG_MIN / 2) return 0;
    if (n >= 0) return 0 - (2 * n + 1);
    return 2 * n;
}

long intval(Ref r)
{
    if (r & 1) return (0 - r - 1) / 2;
    return r / 2;
}

Ref mkfloat(double x)
{
    Ref c = newcell(T_FLOAT);
    XF(c) = x;
    return c;
}

double fval(Ref r)
{
    if (r < 0) return (double)intval(r);
    if (ctag[r] != T_FLOAT) return 0.0;   /* cnum[r] there */
    return XF(r);
}

Ref mkstrn(const char *s, long n)
{
    Ref c = newcell(T_STR);
    char *p = xstrndup(s, n);
    XS(c) = p;
    ccdr[c] = n;
    return c;
}

Ref mkstr(const char *s)
{
    return mkstrn(s, (long)strlen(s));
}

Ref mkchar(long code)
{
    Ref c = newcell(T_CHAR);
    XL(c) = code;
    return c;
}

Ref cons(Ref a, Ref d)
{
    Ref c;
    if (freelist) {
        c = freelist;
        freelist = ccdr[c];
    } else {
        if (ncell + 2 >= capcell) growheap();
        c = ++ncell;
    }
    ctag[c] = T_CONS;
    ccar[c] = a;
    ccdr[c] = d;
    return c;
}

Ref list1(Ref a)
{
    return cons(a, NIL);
}

Ref list2(Ref a, Ref b)
{
    return cons(a, cons(b, NIL));
}

Ref list3(Ref a, Ref b, Ref c)
{
    return cons(a, cons(b, cons(c, NIL)));
}

/* the safe versions of car and cdr */
Ref car(Ref r)
{
    if (r == NIL) return NIL;
    if (ISCONS(r)) return ccar[r];
    lerrobj("TYPE-ERROR", "the value ", r, " is not a list");
    return NIL;
}

Ref cdr(Ref r)
{
    if (r == NIL) return NIL;
    if (ISCONS(r)) return ccdr[r];
    lerrobj("TYPE-ERROR", "the value ", r, " is not a list");
    return NIL;
}

/* ---------------------------------------------------- list helpers */
long listlen(Ref l)
{
    long n = 0;
    while (ISCONS(l)) {
        n++;
        l = ccdr[l];
    }
    return n;
}

/* build a Lisp list from av[from..n] (1-based like cREXX) */
Ref arr2list(Ref *av, long from, long n)
{
    Ref l = NIL;
    long i;
    for (i = n; i >= from; i--) l = cons(av[i - 1], l);
    return l;
}

Ref *list2arr(Ref l, long *n)
{
    long k = listlen(l), i = 0;
    Ref *a = (Ref *)xmalloc((k + 1) * (long)sizeof(Ref));
    while (ISCONS(l)) {
        a[i++] = ccar[l];
        l = ccdr[l];
    }
    *n = k;
    return a;
}

Ref reverse(Ref l)
{
    Ref r = NIL;
    while (ISCONS(l)) {
        r = cons(ccar[l], r);
        l = ccdr[l];
    }
    return r;
}

/* destructively reverse (fresh conses only) */
Ref nreverse(Ref l)
{
    Ref prev = NIL, nx;
    while (ISCONS(l)) {
        nx = ccdr[l];
        ccdr[l] = prev;
        prev = l;
        l = nx;
    }
    return prev;
}

Ref append2(Ref a, Ref b)
{
    Ref r, nx;
    if (!ISCONS(a)) return b;
    r = NIL;
    while (ISCONS(a)) {
        r = cons(ccar[a], r);
        a = ccdr[a];
    }
    while (ISCONS(r)) {
        nx = ccdr[r];
        ccdr[r] = b;
        b = r;
        r = nx;
    }
    return b;
}

/* --------------------------------------------------------- symbols */
#define NBUCKET 8191
static Ref hbk[NBUCKET];

static long hashname(const char *s, long n)
{
    long h = 7, i;
    for (i = 0; i < n; i++)
        h = (h * 31 + (unsigned char)s[i]) % NBUCKET;
    return h;
}

static Ref symcell(const char *name, long n)
{
    Ref c = newcell(T_SYM);
    long i;
    if (nsyms >= capsyms) {
        capsyms = capsyms ? capsyms * 2 : 1024;
        syms = (Sym *)xrealloc(syms, capsyms * (long)sizeof(Sym));
    }
    i = nsyms++;
    syms[i].name = xstrndup(name, n);
    syms[i].len = n;
    syms[i].chn = 0;
    syms[i].cfk = 0;
    syms[i].cpl = 0;
    syms[i].cslot = 0;
    XL(c) = i;
    ccar[c] = 0;
    ccdr[c] = 0;
    return c;
}

Ref intern(const char *name)
{
    long n = (long)strlen(name);
    long h = hashname(name, n);
    Ref c = hbk[h];
    while (c != 0) {
        if (SYM(c).len == n && memcmp(SYM(c).name, name, n) == 0)
            return c;
        c = SYM(c).chn;
    }
    c = symcell(name, n);
    SYM(c).chn = hbk[h];
    hbk[h] = c;
    if (name[0] == ':') {
        ccar[c] = c;
        cspec[c] = 2;
    }
    return c;
}

/* an uninterned symbol (make-symbol, gensym) */
Ref newsym(const char *name)
{
    Ref c = symcell(name, (long)strlen(name));
    cspec[c] = 3;
    return c;
}

Ref mksym(const char *name)
{
    char *t = xstrdup(name);
    Ref r;
    upstr(t, (long)strlen(t));
    r = intern(t);
    free(t);
    return r;
}

/* -------------------------------------------------------- vectors */
/* a vector: ext = element array, cdr = fill count, car = capacity */
Ref makevec(long n)
{
    Ref v = newcell(T_VEC);
    Ref *p = (Ref *)xmalloc((n + 1) * (long)sizeof(Ref));
    XP(v) = p;
    ccdr[v] = n;
    ccar[v] = 0;
    return v;
}

Ref *vdata(Ref v)
{
    return (Ref *)XP(v);
}

Ref mkvec(Ref *a, long n)
{
    Ref v = makevec(n);
    long i;
    Ref *p = vdata(v);
    for (i = 0; i < n; i++) p[i] = a[i];
    ccar[v] = n;
    return v;
}

Ref mkvecn(long n, Ref init)
{
    Ref v;
    long i;
    Ref *p;
    if (n < 0) n = 0;
    v = makevec(n);
    p = vdata(v);
    for (i = 0; i < n; i++) p[i] = init;
    ccar[v] = n;
    return v;
}

/* ---------------------------------------------------------- errors */
static void seterr(const char *kind, const char *msg)
{
    unw = 1;
    if (errmsg) free(errmsg);
    errmsg = xstrdup(msg);
    errkind = kind;
}

Ref lerror(const char *msg)
{
    if (unw == 0) seterr("ERROR", msg);
    return NIL;
}

Ref lerrk(const char *kind, const char *msg)
{
    if (unw == 0) seterr(kind, msg);
    return NIL;
}

/* error message: pre + printed object (with escapes) + post */
Ref lerrobj(const char *kind, const char *pre, Ref o,
            const char *post)
{
    Sb b;
    if (unw != 0) return NIL;
    sb_init(&b);
    sb_adds(&b, pre);
    pr_obj(&b, o, 1);
    sb_adds(&b, post);
    seterr(kind, sb_str(&b));
    free(b.p);
    return NIL;
}

Ref notnum(Ref a)
{
    return lerrobj("TYPE-ERROR", "the value ", a,
                   " is not of type NUMBER");
}
