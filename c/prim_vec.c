/* prim_vec.c -- primitives 101-112: sorting, vectors, sequences. */
#include "lisp.h"

#define A(i) av[(i) - 1]

void msort(Ref *a, long n, Ref pf, Ref kf);

static const char *symtext(Ref s)
{
    if (ISSYM(s)) return SYMNAME(s);
    return "";
}

static Ref pf101(Ref *av, long n)
{
    Ref pf = fnof(A(2)), kf, *a, r;
    long k, i;
    if (unw != 0) return NIL;
    kf = kwfn(av, n, 3, "KEY");
    a = seq2arr(A(1), &k);
    msort(a, k, pf, kf);
    if (unw != 0) {
        free(a);
        return NIL;
    }
    if (A(1) > 0 && ctag[A(1)] == T_VEC) {
        for (i = 0; i < k; i++) vdata(A(1))[i] = a[i];
        free(a);
        return A(1);
    }
    r = arr2seq(A(1), a, k);
    free(a);
    return r;
}

/* ELT AREF SVREF CHAR SCHAR */
static Ref pf102(Ref *av, long n)
{
    Ref x = A(1);
    long i = toint(A(2));
    char m[100];
    (void)n;
    if (unw != 0) return NIL;
    if (x == NIL || ISCONS(x)) {
        while (i > 0 && ISCONS(x)) {
            x = ccdr[x];
            i--;
        }
        if (!ISCONS(x)) return lerror("index out of bounds");
        return ccar[x];
    }
    if (x > 0) {
        if (ctag[x] == T_VEC) {
            if (i < 0 || i >= ccdr[x]) {
                sprintf(m, "index %ld is out of bounds for the vector",
                        i);
                return lerror(m);
            }
            return vdata(x)[i];
        }
        if (ctag[x] == T_STR) {
            if (i < 0 || i >= SLEN(x)) {
                sprintf(m, "index %ld is out of bounds for the string",
                        i);
                return lerror(m);
            }
            return mkchar((unsigned char)XS(x)[i]);
        }
    }
    return lerrobj("TYPE-ERROR", "the value ", x,
                   " is not a sequence");
}

static Ref pf103(Ref *av, long n)
{
    long en = -1, st;
    if (n == 3 && A(3) != NIL) en = toint(A(3));
    st = toint(A(2));
    return subseqn(A(1), st, en);
}

static Ref pf104(Ref *av, long n)
{
    (void)n;
    return subseqn(A(1), 0, -1);
}

static Ref pf105(Ref *av, long n)
{
    return mkvec(av, n);
}

static Ref pf106(Ref *av, long n)
{
    long sz = 0, j, k, i;
    Ref init = mkint(0), v, *a;
    if (A(1) > 0 || A(1) == NIL) {
        if (ISCONS(A(1))) sz = toint(ccar[A(1)]);
    } else sz = toint(A(1));
    if (unw != 0) return NIL;
    j = kwidx(av, n, 2, "INITIAL-ELEMENT");
    if (j > 0) init = A(j);
    v = mkvecn(sz, init);
    j = kwidx(av, n, 2, "INITIAL-CONTENTS");
    if (j > 0) {
        a = seq2arr(A(j), &k);
        for (i = 0; i < k && i < ccdr[v]; i++) vdata(v)[i] = a[i];
        free(a);
    }
    j = kwidx(av, n, 2, "FILL-POINTER");
    if (j > 0) {
        if (A(j) < 0) ccdr[v] = intval(A(j));
        else if (A(j) == TRU) ccdr[v] = sz;
        else ccdr[v] = 0;
    }
    return v;
}

static int isvec(Ref v)
{
    return v > 0 && ctag[v] == T_VEC;
}

static Ref pf107(Ref *av, long n)
{
    Ref v = A(2);
    (void)n;
    if (!isvec(v))
        return lerrobj("TYPE-ERROR", "the value ", v,
                       " is not a vector");
    if (ccdr[v] >= ccar[v]) return NIL;
    vdata(v)[ccdr[v]] = A(1);
    ccdr[v] = ccdr[v] + 1;
    return mkint(ccdr[v] - 1);
}

static Ref pf108(Ref *av, long n)
{
    Ref v = A(2);
    long nc, i;
    Ref *nb;
    (void)n;
    if (!isvec(v))
        return lerrobj("TYPE-ERROR", "the value ", v,
                       " is not a vector");
    if (ccdr[v] >= ccar[v]) {
        nc = ccar[v] * 2;
        if (nc < 4) nc = 4;
        nb = (Ref *)xrealloc(vdata(v), (nc + 1) * (long)sizeof(Ref));
        for (i = ccdr[v]; i < nc; i++) nb[i] = NIL;
        XP(v) = nb;
        ccar[v] = nc;
    }
    vdata(v)[ccdr[v]] = A(1);
    ccdr[v] = ccdr[v] + 1;
    return mkint(ccdr[v] - 1);
}

static Ref pf109(Ref *av, long n)
{
    Ref v = A(1);
    (void)n;
    if (!isvec(v))
        return lerrobj("TYPE-ERROR", "the value ", v,
                       " is not a vector");
    if (ccdr[v] == 0) return lerror("nothing to pop");
    ccdr[v] = ccdr[v] - 1;
    return vdata(v)[ccdr[v]];
}

static Ref pf110(Ref *av, long n)
{
    Ref x = A(1), l;
    long i;
    (void)n;
    if (isvec(x)) {
        for (i = 0; i < ccdr[x]; i++) vdata(x)[i] = A(2);
        return x;
    }
    for (l = x; ISCONS(l); l = ccdr[l]) ccar[l] = A(2);
    return x;
}

static Ref pf111(Ref *av, long n)
{
    Ref *a, *b, r;
    long k = 0, m, i, j, cap = 16;
    const char *ty = symtext(A(1));
    a = (Ref *)xmalloc(cap * (long)sizeof(Ref));
    for (i = 2; i <= n; i++) {
        b = seq2arr(A(i), &m);
        if (k + m + 1 > cap) {
            while (k + m + 1 > cap) cap *= 2;
            a = (Ref *)xrealloc(a, cap * (long)sizeof(Ref));
        }
        for (j = 0; j < m; j++) a[k++] = b[j];
        free(b);
    }
    if (!strcmp(ty, "STRING")) r = arr2seq(mkstr(""), a, k);
    else if (!strcmp(ty, "LIST")) r = arr2list(a, 1, k);
    else r = mkvec(a, k);
    free(a);
    return r;
}

static Ref pf112(Ref *av, long n)
{
    Ref x = A(1), *a, r;
    const char *ty = symtext(A(2));
    long k;
    (void)n;
    if (!strcmp(ty, "LIST")) {
        a = seq2arr(x, &k);
        r = arr2list(a, 1, k);
        free(a);
        return r;
    }
    if (!strcmp(ty, "VECTOR") || !strcmp(ty, "SIMPLE-VECTOR")) {
        a = seq2arr(x, &k);
        r = mkvec(a, k);
        free(a);
        return r;
    }
    if (!strcmp(ty, "STRING")) {
        if (ISSTR(x)) return x;
        a = seq2arr(x, &k);
        r = arr2seq(mkstr(""), a, k);
        free(a);
        return r;
    }
    if (!strcmp(ty, "FLOAT") || !strcmp(ty, "DOUBLE-FLOAT") ||
        !strcmp(ty, "SINGLE-FLOAT")) {
        if (!ISNUM(x)) return notnum(x);
        return mkfloat(fval(x));
    }
    if (!strcmp(ty, "INTEGER")) return mkint(toint(x));
    if (!strcmp(ty, "CHARACTER")) return x;
    if (!strcmp(ty, "FUNCTION")) return fnof(x);
    return x;
}

void initprim_b(void)
{
    primtab[101] = pf101; primtab[102] = pf102;
    primtab[103] = pf103; primtab[104] = pf104;
    primtab[105] = pf105; primtab[106] = pf106;
    primtab[107] = pf107; primtab[108] = pf108;
    primtab[109] = pf109; primtab[110] = pf110;
    primtab[111] = pf111; primtab[112] = pf112;
}
