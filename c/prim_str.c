/* prim_str.c -- primitives 159-186: strings and symbols. */
#include "lisp.h"

#define A(i) av[(i) - 1]

static int upc(int c)
{
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

/* compare two string designators: -1, 0, 1 */
static int cmpstr(Ref a, Ref b, int fold)
{
    long la, lb, m, i;
    const char *x, *y;
    char c1[2], c2[2];
    int u, v;
    x = strof(a, &la, c1);
    y = strof(b, &lb, c2);
    if (unw != 0) return 0;
    m = la < lb ? la : lb;
    for (i = 0; i < m; i++) {
        u = (unsigned char)x[i];
        v = (unsigned char)y[i];
        if (fold) {
            u = upc(u);
            v = upc(v);
        }
        if (u < v) return -1;
        if (u > v) return 1;
    }
    if (la < lb) return -1;
    if (la > lb) return 1;
    return 0;
}

static Ref pf159(Ref *av, long n)
{
    long len;
    char cb[2];
    const char *s = strof(A(1), &len, cb);
    (void)n;
    if (unw != 0) return NIL;
    return mkstrn(s, len);
}

static Ref pf160(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 0);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c == 0);
}

static Ref pf161(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 1);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c == 0);
}

static Ref pf162(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 0);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c < 0);
}

static Ref pf163(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 0);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c > 0);
}

static Ref pf164(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 0);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c <= 0);
}

static Ref pf165(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 0);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c >= 0);
}

static Ref pf166(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 1);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c < 0);
}

static Ref pf167(Ref *av, long n)
{
    int c = cmpstr(A(1), A(2), 1);
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c > 0);
}

/* STRING-UPCASE (0) STRING-DOWNCASE (1) STRING-CAPITALIZE (2) */
static Ref casestr(Ref *av, int kind)
{
    long len;
    char cb[2];
    const char *s = strof(A(1), &len, cb);
    char *t;
    Ref r;
    Sb b;
    if (unw != 0) return NIL;
    if (kind == 2) {
        sb_init(&b);
        capitalize(&b, s, len);
        sb_str(&b);
        r = mkstrn(b.p, b.n);
        free(b.p);
        return r;
    }
    t = xstrndup(s, len);
    if (kind == 0) upstr(t, len);
    else lowstr(t, len);
    r = mkstrn(t, len);
    free(t);
    return r;
}

static Ref pf168(Ref *av, long n)
{
    (void)n;
    return casestr(av, 0);
}

static Ref pf169(Ref *av, long n)
{
    (void)n;
    return casestr(av, 1);
}

static Ref pf170(Ref *av, long n)
{
    (void)n;
    return casestr(av, 2);
}

static Ref pf171(Ref *av, long n)
{
    long lc, ls, a = 0, b;
    char c1[2], c2[2];
    const char *chars = strof(A(1), &lc, c1);
    const char *s = strof(A(2), &ls, c2);
    const char *nm = XS(curfn);
    (void)n;
    if (unw != 0) return NIL;
    b = ls;
    if (strcmp(nm, "STRING-RIGHT-TRIM") != 0)
        while (a < b && memchr(chars, s[a], (size_t)lc) != NULL) a++;
    if (strcmp(nm, "STRING-LEFT-TRIM") != 0)
        while (b > a && memchr(chars, s[b - 1], (size_t)lc) != NULL)
            b--;
    return mkstrn(s + a, b - a);
}

static Ref pf172(Ref *av, long n)
{
    int c = ' ';
    long j = kwidx(av, n, 2, "INITIAL-ELEMENT"), k, i;
    Sb b;
    Ref r;
    if (j > 0) {
        c = (int)chr(A(j));
        if (unw != 0) return NIL;
    }
    k = toint(A(1));
    if (unw != 0) return NIL;
    sb_init(&b);
    for (i = 0; i < k; i++) sb_addc(&b, c);
    sb_str(&b);
    r = mkstrn(b.p, b.n);
    free(b.p);
    return r;
}

static Ref pf173(Ref *av, long n)
{
    Ref a = A(1), b = A(2), *x, *y;
    long la, lb, i, j, k, m;
    int ok;
    (void)n;
    if (ISSTR(a) && ISSTR(b)) {
        la = SLEN(a);
        lb = SLEN(b);
        if (la == 0) return mkint(0);
        for (i = 0; i + la <= lb; i++)
            if (memcmp(XS(b) + i, XS(a), (size_t)la) == 0)
                return mkint(i);
        return NIL;
    }
    x = seq2arr(a, &k);
    y = seq2arr(b, &m);
    for (i = 0; i <= m - k; i++) {
        ok = 1;
        for (j = 0; j < k; j++) {
            if (!eql(x[j], y[i + j])) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            free(x);
            free(y);
            return mkint(i);
        }
    }
    free(x);
    free(y);
    return NIL;
}

static Ref needsym(Ref x)
{
    if (ISSYM(x)) return x;
    return lerrobj("TYPE-ERROR", "the value ", x, " is not a symbol");
}

static Ref pf174(Ref *av, long n)
{
    long len;
    char cb[2];
    const char *s;
    (void)n;
    s = strof(A(1), &len, cb);
    if (unw != 0) return NIL;
    return mkstrn(s, len);
}

static Ref pf175(Ref *av, long n)
{
    long len;
    char cb[2];
    const char *s;
    char *t;
    Ref r;
    (void)n;
    s = strof(A(1), &len, cb);
    if (unw != 0) return NIL;
    t = xstrndup(s, len);
    r = intern(t);
    free(t);
    return r;
}

static Ref pf176(Ref *av, long n)
{
    long len;
    char cb[2];
    const char *s;
    char *t;
    Ref r;
    (void)n;
    s = strof(A(1), &len, cb);
    if (unw != 0) return NIL;
    t = xstrndup(s, len);
    r = newsym(t);
    free(t);
    return r;
}

static Ref pf177(Ref *av, long n)
{
    char t[40];
    (void)av;
    (void)n;
    gensymn++;
    sprintf(t, "G%ld", gensymn);
    return newsym(t);
}

static Ref pf178(Ref *av, long n)
{
    Ref v;
    (void)n;
    needsym(A(1));
    if (unw != 0) return NIL;
    v = ccar[A(1)];
    if (v == 0) {
        lerrobj("UNBOUND-VARIABLE", "the variable ", A(1),
                " is unbound");
        return NIL;
    }
    return v;
}

static Ref pf179(Ref *av, long n)
{
    (void)n;
    return fnof(A(1));
}

static Ref pf180(Ref *av, long n)
{
    (void)n;
    needsym(A(1));
    if (unw != 0) return NIL;
    ccar[A(1)] = A(2);
    return A(2);
}

static Ref pf181(Ref *av, long n)
{
    (void)n;
    needsym(A(1));
    if (unw != 0) return NIL;
    return TRUTH(ccar[A(1)] != 0);
}

static Ref pf182(Ref *av, long n)
{
    (void)n;
    needsym(A(1));
    if (unw != 0) return NIL;
    return TRUTH(ccdr[A(1)] > 1);
}

static Ref pf183(Ref *av, long n)
{
    (void)n;
    needsym(A(1));
    if (unw != 0) return NIL;
    ccar[A(1)] = 0;
    return A(1);
}

static Ref pf184(Ref *av, long n)
{
    Ref v;
    needsym(A(1));
    if (unw != 0) return NIL;
    v = getprop(A(1), A(2));
    if (v == NIL && n == 3) return A(3);
    return v;
}

static Ref pf185(Ref *av, long n)
{
    (void)n;
    return TRUTH(typep(A(1), A(2)));
}

static Ref pf186(Ref *av, long n)
{
    (void)n;
    return lobjtype(A(1));
}

void initprim_d(void)
{
    primtab[159] = pf159; primtab[160] = pf160; primtab[161] = pf161;
    primtab[162] = pf162; primtab[163] = pf163; primtab[164] = pf164;
    primtab[165] = pf165; primtab[166] = pf166; primtab[167] = pf167;
    primtab[168] = pf168; primtab[169] = pf169; primtab[170] = pf170;
    primtab[171] = pf171; primtab[172] = pf172; primtab[173] = pf173;
    primtab[174] = pf174; primtab[175] = pf175; primtab[176] = pf176;
    primtab[177] = pf177; primtab[178] = pf178; primtab[179] = pf179;
    primtab[180] = pf180; primtab[181] = pf181; primtab[182] = pf182;
    primtab[183] = pf183; primtab[184] = pf184; primtab[185] = pf185;
    primtab[186] = pf186;
}
