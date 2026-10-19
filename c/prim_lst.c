/* prim_lst.c -- primitives 1-100: numbers, predicates, lists,
   mapping and searching.  A(i) is the i-th argument (1-based). */
#include "lisp.h"

#define A(i) av[(i) - 1]

static Ref pf1(Ref *av, long n)
{
    (void)n;
    return car(A(1));
}

static Ref pf2(Ref *av, long n)
{
    (void)n;
    return cdr(A(1));
}

static Ref pf3(Ref *av, long n)
{
    (void)n;
    return cons(A(1), A(2));
}

static Ref pf4(Ref *av, long n)
{
    Ref r;
    long i;
    if (n == 2) return numadd(A(1), A(2));
    r = mkint(0);
    for (i = 1; i <= n; i++) r = numadd(r, A(i));
    return r;
}

static Ref pf5(Ref *av, long n)
{
    Ref r;
    long i;
    if (n == 1) return numneg(A(1));
    r = A(1);
    for (i = 2; i <= n; i++) r = numsub(r, A(i));
    return r;
}

static Ref pf6(Ref *av, long n)
{
    Ref r = mkint(1);
    long i;
    for (i = 1; i <= n; i++) r = nummul(r, A(i));
    return r;
}

static Ref pf7(Ref *av, long n)
{
    Ref r;
    long i;
    if (n == 1) return numdiv(mkint(1), A(1));
    r = A(1);
    for (i = 2; i <= n; i++) r = numdiv(r, A(i));
    return r;
}

static Ref pf8(Ref *av, long n)
{
    long i;
    for (i = 1; i < n; i++)
        if (numcmp(A(i), A(i + 1)) != 0) return NIL;
    if (unw != 0) return NIL;
    return TRU;
}

static Ref pf9(Ref *av, long n)
{
    long i;
    for (i = 1; i < n; i++)
        if (numcmp(A(i), A(i + 1)) >= 0) return NIL;
    return TRU;
}

static Ref pf10(Ref *av, long n)
{
    long i;
    for (i = 1; i < n; i++)
        if (numcmp(A(i), A(i + 1)) <= 0) return NIL;
    return TRU;
}

static Ref pf11(Ref *av, long n)
{
    long i;
    for (i = 1; i < n; i++)
        if (numcmp(A(i), A(i + 1)) > 0) return NIL;
    return TRU;
}

static Ref pf12(Ref *av, long n)
{
    long i;
    for (i = 1; i < n; i++)
        if (numcmp(A(i), A(i + 1)) < 0) return NIL;
    return TRU;
}

static Ref pf13(Ref *av, long n)
{
    (void)n;
    return numcmp(A(1), A(2)) == 0 ? NIL : TRU;
}

static Ref pf14(Ref *av, long n)
{
    (void)n;
    return numadd(A(1), mkint(1));
}

static Ref pf15(Ref *av, long n)
{
    (void)n;
    return numsub(A(1), mkint(1));
}

static Ref pf16(Ref *av, long n)
{
    (void)n;
    return A(1) == NIL ? TRU : NIL;
}

static Ref pf17(Ref *av, long n)
{
    (void)n;
    return A(1) == A(2) ? TRU : NIL;
}

static Ref pf18(Ref *av, long n)
{
    (void)n;
    return TRUTH(eql(A(1), A(2)));
}

static Ref pf19(Ref *av, long n)
{
    (void)n;
    return TRUTH(equal(A(1), A(2)));
}

static Ref pf20(Ref *av, long n)
{
    (void)n;
    return TRUTH(equalp(A(1), A(2)));
}

static Ref pf21(Ref *av, long n)
{
    return arr2list(av, 1, n);
}

static Ref pf22(Ref *av, long n)
{
    Ref r = A(n);
    long i;
    for (i = n - 1; i >= 1; i--) r = cons(A(i), r);
    return r;
}

static Ref pf23(Ref *av, long n)
{
    (void)n;
    return TRUTH(!ISCONS(A(1)));
}

static Ref pf24(Ref *av, long n)
{
    (void)n;
    return TRUTH(ISCONS(A(1)));
}

static Ref pf25(Ref *av, long n)
{
    (void)n;
    return TRUTH(A(1) == NIL || ISCONS(A(1)));
}

static Ref pf26(Ref *av, long n)
{
    (void)n;
    return TRUTH(ISSYM(A(1)));
}

static Ref pf27(Ref *av, long n)
{
    (void)n;
    return TRUTH(ISSTR(A(1)));
}

static Ref pf28(Ref *av, long n)
{
    (void)n;
    return TRUTH(ISNUM(A(1)));
}

static Ref pf29(Ref *av, long n)
{
    (void)n;
    return TRUTH(A(1) < 0);
}

static Ref pf30(Ref *av, long n)
{
    (void)n;
    return TRUTH(ISFLOAT(A(1)));
}

static Ref pf31(Ref *av, long n)
{
    (void)n;
    return TRUTH(ISCHAR(A(1)));
}

static Ref pf32(Ref *av, long n)
{
    Ref x = A(1);
    (void)n;
    return TRUTH(x > 0 && (ctag[x] == T_PRIM || ctag[x] == T_CLOS));
}

static Ref pf33(Ref *av, long n)
{
    Ref x = A(1);
    (void)n;
    return TRUTH(x > 0 && (ctag[x] == T_VEC || ctag[x] == T_STR));
}

static Ref pf34(Ref *av, long n)
{
    Ref x = A(1);
    (void)n;
    return TRUTH(x > 0 && ctag[x] == T_VEC);
}

static Ref pf35(Ref *av, long n)
{
    Ref x = A(1);
    (void)n;
    return TRUTH(x > 0 && ctag[x] == T_HASH);
}

static Ref pf36(Ref *av, long n)
{
    (void)n;
    return TRUTH(typep(A(1), intern("KEYWORD")));
}

static Ref pf37(Ref *av, long n)
{
    (void)n;
    return TRUTH(numcmp(A(1), mkint(0)) == 0);
}

static Ref pf38(Ref *av, long n)
{
    (void)n;
    return TRUTH(numcmp(A(1), mkint(0)) > 0);
}

static Ref pf39(Ref *av, long n)
{
    (void)n;
    return TRUTH(numcmp(A(1), mkint(0)) < 0);
}

static Ref pf40(Ref *av, long n)
{
    (void)n;
    if (A(1) >= 0)
        return lerrobj("TYPE-ERROR", "the value ", A(1),
                       " is not an INTEGER");
    return TRUTH(intval(A(1)) % 2 == 0);
}

static Ref pf41(Ref *av, long n)
{
    (void)n;
    if (A(1) >= 0)
        return lerrobj("TYPE-ERROR", "the value ", A(1),
                       " is not an INTEGER");
    return TRUTH(intval(A(1)) % 2 != 0);
}

static Ref pf42(Ref *av, long n)
{
    (void)n;
    return car(A(1));
}

static Ref pf43(Ref *av, long n)
{
    (void)n;
    return cdr(A(1));
}

/* CAAR CADR CDAR CDDR ... : the letters between C and R */
static Ref pf44(Ref *av, long n)
{
    const char *nm = XS(curfn);
    long i, len = (long)strlen(nm);
    Ref x = A(1);
    (void)n;
    for (i = len - 2; i >= 1; i--) {
        if (nm[i] == 'A') x = car(x);
        else x = cdr(x);
        if (unw != 0) return NIL;
    }
    return x;
}

static Ref pf45(Ref *av, long n)
{
    (void)n;
    return car(cdr(A(1)));
}

static Ref pf46(Ref *av, long n)
{
    (void)n;
    return car(cdr(cdr(A(1))));
}

static Ref pf47(Ref *av, long n)
{
    (void)n;
    return car(cdr(cdr(cdr(A(1)))));
}

static Ref pf48(Ref *av, long n)
{
    (void)n;
    return car(cdr(cdr(cdr(cdr(A(1))))));
}

static Ref pf49(Ref *av, long n)
{
    Ref l = A(2);
    long k = toint(A(1));
    (void)n;
    while (k > 0 && ISCONS(l)) {
        l = ccdr[l];
        k--;
    }
    return car(l);
}

static Ref pf50(Ref *av, long n)
{
    Ref l = A(2);
    long k = toint(A(1));
    (void)n;
    while (k > 0 && ISCONS(l)) {
        l = ccdr[l];
        k--;
    }
    return l;
}

static Ref pf51(Ref *av, long n)
{
    Ref x = A(1);
    (void)n;
    if (x == NIL) return mkint(0);
    if (ISCONS(x)) return mkint(listlen(x));
    if (x > 0) {
        if (ctag[x] == T_STR) return mkint(SLEN(x));
        if (ctag[x] == T_VEC) return mkint(ccdr[x]);
        if (ctag[x] == T_HASH) return mkint(ccdr[x]);
    }
    return lerrobj("TYPE-ERROR", "the value ", x,
                   " is not a sequence");
}

static Ref pf52(Ref *av, long n)
{
    Ref r;
    long i;
    if (n == 0) return NIL;
    r = A(n);
    for (i = n - 1; i >= 1; i--) r = append2(A(i), r);
    return r;
}

static Ref pf53(Ref *av, long n)
{
    Ref r = NIL, p;
    long i;
    for (i = n; i >= 1; i--) {
        if (A(i) != NIL) {
            if (r == NIL) r = A(i);
            else {
                p = A(i);
                if (p <= 0)     /* cREXX panics here */
                    return lerrobj("TYPE-ERROR", "the value ", p,
                                   " is not a list");
                if (ISCONS(p)) {
                    while (ISCONS(ccdr[p])) p = ccdr[p];
                    ccdr[p] = r;
                }               /* an atom: the cdr is never seen */
                r = A(i);
            }
        }
    }
    return r;
}

static Ref revseq(Ref x, int destructive)
{
    Ref *a, *b, r;
    long k, i;
    if (x == NIL || ISCONS(x))
        return destructive ? nreverse(x) : reverse(x);
    a = seq2arr(x, &k);
    b = (Ref *)xmalloc((k + 1) * (long)sizeof(Ref));
    for (i = 0; i < k; i++) b[i] = a[k - 1 - i];
    r = arr2seq(x, b, k);
    free(a);
    free(b);
    return r;
}

static Ref pf54(Ref *av, long n)
{
    (void)n;
    return revseq(A(1), 0);
}

static Ref pf55(Ref *av, long n)
{
    (void)n;
    return revseq(A(1), 1);
}

static Ref pf56(Ref *av, long n)
{
    Ref l = A(1);
    long k = 1, m;
    if (n == 2) k = toint(A(2));
    if (l < 0) return lerrobj("TYPE-ERROR", "the value ", l,
                              " is not a list");
    m = listlen(l);
    while (m > k && l > 0) {
        l = ccdr[l];
        m--;
    }
    return l;
}

static Ref pf57(Ref *av, long n)
{
    long k = 1, m;
    long cnt, i;
    Ref *a, r;
    if (n == 2) k = toint(A(2));
    a = list2arr(A(1), &cnt);
    m = cnt - k;
    if (m < 0) m = 0;
    if (m > cnt) {              /* k < 0: reads past the end */
        if (m > 100000000L) m = 100000000L;
        a = (Ref *)xrealloc(a, (m + 1) * (long)sizeof(*a));
        for (i = cnt; i < m; i++) a[i] = 0;
    }
    r = arr2list(a, 1, m);
    free(a);
    return r;
}

static Ref pf58(Ref *av, long n)
{
    long m;
    Ref *a = list2arr(A(1), &m), r;
    (void)n;
    r = arr2list(a, 1, m);
    free(a);
    return r;
}

static Ref pf59(Ref *av, long n)
{
    (void)n;
    return copytree(A(1));
}

static Ref pf60(Ref *av, long n)
{
    (void)n;
    return mkint(listlen(A(1)));
}

static Ref pf61(Ref *av, long n)
{
    (void)n;
    return A(1);
}

static Ref pf62(Ref *av, long n)
{
    Ref fn = fnof(A(1));
    if (unw != 0) return NIL;
    return applyfn(fn, av + 1, n - 1);
}

static Ref pf63(Ref *av, long n)
{
    Ref fn = fnof(A(1)), l, r, *b;
    long k = 0, i;
    if (unw != 0) return NIL;
    b = (Ref *)xmalloc((n + listlen(n >= 2 ? A(n) : NIL) + 1) *
                       (long)sizeof(Ref));
    for (i = 2; i <= n - 1; i++) b[k++] = A(i);
    if (n >= 2) {
        for (l = A(n); ISCONS(l); l = ccdr[l]) b[k++] = ccar[l];
    }
    r = applyfn(fn, b, k);
    free(b);
    return r;
}

static Ref pf64(Ref *av, long n)
{
    mvlist = arr2list(av, 1, n);
    if (n == 0) return NIL;
    return A(1);
}

static Ref pf65(Ref *av, long n)
{
    (void)n;
    mvlist = A(1);
    return car(A(1));
}

static Ref pf66(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST");
    Ref kf = kwfn(av, n, 3, "KEY");
    Ref l = A(2), e;
    while (ISCONS(l)) {
        e = ccar[l];
        if (ISCONS(e) && same(tf, A(1), keyed(kf, ccar[e]))) return e;
        l = ccdr[l];
    }
    return NIL;
}

static Ref pf67(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST");
    Ref l = A(2), e;
    while (ISCONS(l)) {
        e = ccar[l];
        if (ISCONS(e) && same(tf, A(1), ccdr[e])) return e;
        l = ccdr[l];
    }
    return NIL;
}

static Ref pf68(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST");
    Ref kf = kwfn(av, n, 3, "KEY");
    Ref l = A(2);
    while (ISCONS(l)) {
        if (same(tf, A(1), keyed(kf, ccar[l]))) return l;
        l = ccdr[l];
    }
    return NIL;
}

static Ref pf69(Ref *av, long n)
{
    Ref l = A(2);
    (void)n;
    while (ISCONS(l)) {
        if (ccar[l] == A(1)) return l;
        l = ccdr[l];
    }
    return NIL;
}

static Ref pf70(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST");
    Ref l = A(2);
    while (ISCONS(l)) {
        if (same(tf, A(1), ccar[l])) return A(2);
        l = ccdr[l];
    }
    return cons(A(1), A(2));
}

static Ref pf71(Ref *av, long n)
{
    Ref t;
    (void)n;
    t = cons(A(1), A(2));
    return cons(t, A(3));
}

static Ref pf72(Ref *av, long n)
{
    long k, m, i;
    Ref *a = list2arr(A(1), &k), *b = list2arr(A(2), &m);
    Ref r = NIL, t, x, y;
    if (n == 3) r = A(3);
    for (i = k; i >= 1; i--) {
        x = a[i - 1];
        y = i <= m ? b[i - 1] : 0;
        t = cons(x, y);
        r = cons(t, r);
    }
    free(a);
    free(b);
    return r;
}

static Ref pf73(Ref *av, long n)
{
    Ref l = A(1);
    while (ISCONS(l)) {
        if (ccar[l] == A(2)) return car(ccdr[l]);
        l = cdr(ccdr[l]);
    }
    if (n == 3) return A(3);
    return NIL;
}

static Ref pf74(Ref *av, long n)
{
    Ref init = NIL, r = NIL;
    long j = kwidx(av, n, 2, "INITIAL-ELEMENT"), i, k;
    if (j > 0) init = A(j);
    k = toint(A(1));
    for (i = 1; i <= k; i++) r = cons(init, r);
    return r;
}

static Ref subst3(Ref nw, Ref old, Ref x)
{
    Ref r = NIL, t;
    if (eql(x, old)) return nw;
    if (!ISCONS(x)) return x;
    while (ISCONS(x)) {
        t = subst3(nw, old, ccar[x]);
        r = cons(t, r);
        x = ccdr[x];
        if (eql(x, old)) {
            x = nw;
            break;
        }
    }
    t = nreverse(r);
    if (x != NIL) {
        r = t;
        while (ISCONS(ccdr[r])) r = ccdr[r];
        ccdr[r] = x;
    }
    return t;
}

static Ref pf75(Ref *av, long n)
{
    (void)n;
    return subst3(A(1), A(2), A(3));
}

static Ref pf76(Ref *av, long n)
{
    (void)n;
    return TRUTH(equal(A(1), A(2)));
}

/* is a[i] a member of the list l under the test tf ? */
static int inlist(Ref tf, Ref x, Ref l)
{
    while (ISCONS(l)) {
        if (same(tf, x, ccar[l])) return 1;
        l = ccdr[l];
    }
    return 0;
}

static Ref pf77(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST"), r = NIL;
    long k, i;
    Ref *a = list2arr(A(1), &k);
    for (i = 0; i < k; i++)
        if (!inlist(tf, a[i], A(2))) r = cons(a[i], r);
    free(a);
    return nreverse(r);
}

static Ref pf78(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST"), r = NIL;
    long k, i;
    Ref *a = list2arr(A(1), &k);
    for (i = 0; i < k; i++)
        if (inlist(tf, a[i], A(2))) r = cons(a[i], r);
    free(a);
    return nreverse(r);
}

static Ref pf79(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST"), r = A(2);
    long k, i;
    Ref *a = list2arr(A(1), &k);
    for (i = k - 1; i >= 0; i--)
        if (!inlist(tf, a[i], A(2))) r = cons(a[i], r);
    free(a);
    return r;
}

static Ref pf80(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST");
    long k, i;
    Ref *a = list2arr(A(1), &k);
    for (i = 0; i < k; i++)
        if (!inlist(tf, a[i], A(2))) {
            free(a);
            return NIL;
        }
    free(a);
    return TRU;
}

/* MAPCAR MAPC MAPCAN: kind 0, 1, 2 */
static Ref mapn(Ref *av, long n, int kind)
{
    Ref fn = fnof(A(1)), r = NIL, v, *ls, *b;
    long nl = n - 1, i;
    if (unw != 0) return NIL;
    ls = (Ref *)xmalloc((nl + 1) * (long)sizeof(Ref));
    b = (Ref *)xmalloc((nl + 1) * (long)sizeof(Ref));
    for (i = 0; i < nl; i++) ls[i] = A(i + 2);
    for (;;) {
        for (i = 0; i < nl; i++) {
            if (!ISCONS(ls[i])) {
                free(ls);
                free(b);
                if (kind == 0) return nreverse(r);
                if (kind == 1) return A(2);
                return r;
            }
            b[i] = ccar[ls[i]];
            ls[i] = ccdr[ls[i]];
        }
        v = applyfn(fn, b, nl);
        if (unw != 0) {
            free(ls);
            free(b);
            return NIL;
        }
        if (kind == 0) r = cons(v, r);
        else if (kind == 2) r = append2(r, v);
    }
}

static Ref pf81(Ref *av, long n)
{
    return mapn(av, n, 0);
}

static Ref pf82(Ref *av, long n)
{
    return mapn(av, n, 1);
}

static Ref pf83(Ref *av, long n)
{
    return mapn(av, n, 2);
}

static Ref pf84(Ref *av, long n)
{
    Ref fn = fnof(A(1)), l = A(2), r = NIL, v;
    (void)n;
    if (unw != 0) return NIL;
    while (ISCONS(l)) {
        v = call1(fn, l);
        if (unw != 0) return NIL;
        r = cons(v, r);
        l = ccdr[l];
    }
    return nreverse(r);
}

static Ref pf85(Ref *av, long n)
{
    Ref fn = fnof(A(2)), ty, v, *b, *res, r, *a;
    long nl = n - 2, i, j, k, mn = 2000000000L;
    Ref **sq;
    long *cnt;
    const char *nm;
    if (unw != 0) return NIL;
    sq = (Ref **)xmalloc((nl + 1) * (long)sizeof(Ref *));
    cnt = (long *)xmalloc((nl + 1) * (long)sizeof(long));
    for (i = 0; i < nl; i++) {
        a = seq2arr(A(i + 3), &k);
        sq[i] = a;
        cnt[i] = k;
        if (k < mn) mn = k;
    }
    res = (Ref *)xmalloc((mn + 1) * (long)sizeof(Ref));
    b = (Ref *)xmalloc((nl + 1) * (long)sizeof(Ref));
    for (j = 0; j < mn; j++) {
        for (i = 0; i < nl; i++) b[i] = sq[i][j];
        v = applyfn(fn, b, nl);
        if (unw != 0) {
            r = NIL;
            goto done;
        }
        res[j] = v;
    }
    ty = A(1);
    r = NIL;
    if (ty != NIL) {
        nm = ISSYM(ty) ? SYMNAME(ty) : "";
        if (!strcmp(nm, "LIST")) r = arr2list(res, 1, mn);
        else if (!strcmp(nm, "STRING"))
            r = arr2seq(mkstr(""), res, mn);
        else r = mkvec(res, mn);
    }
done:
    for (i = 0; i < nl; i++) free(sq[i]);
    free(sq);
    free(cnt);
    free(res);
    free(b);
    return r;
}

static Ref pf86(Ref *av, long n)
{
    Ref fn = fnof(A(1)), kf, acc, *a, r;
    long k, fe = 0, j, hasinit = 0, st, i;
    if (unw != 0) return NIL;
    kf = kwfn(av, n, 3, "KEY");
    a = seq2arr(A(2), &k);
    j = kwidx(av, n, 3, "FROM-END");
    if (j > 0 && A(j) != NIL) fe = 1;
    j = kwidx(av, n, 3, "INITIAL-VALUE");
    if (j > 0) hasinit = 1;
    if (!fe) {
        st = 1;
        if (hasinit) acc = A(j);
        else {
            if (k == 0) {
                r = applyfn(fn, a, 0);
                free(a);
                return r;
            }
            acc = keyed(kf, a[0]);
            st = 2;
        }
        for (i = st; i <= k; i++) {
            acc = call2(fn, acc, keyed(kf, a[i - 1]));
            if (unw != 0) {
                free(a);
                return NIL;
            }
        }
        free(a);
        return acc;
    }
    st = k;
    if (hasinit) acc = A(j);
    else {
        if (k == 0) {
            r = applyfn(fn, a, 0);
            free(a);
            return r;
        }
        acc = keyed(kf, a[k - 1]);
        st = k - 1;
    }
    for (i = st; i >= 1; i--) {
        acc = call2(fn, keyed(kf, a[i - 1]), acc);
        if (unw != 0) {
            free(a);
            return NIL;
        }
    }
    free(a);
    return acc;
}

static Ref pf87(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST"), kf = kwfn(av, n, 3, "KEY");
    Ref *a, *b, r;
    long k, m = 0, i;
    a = seq2arr(A(2), &k);
    b = (Ref *)xmalloc((k + 1) * (long)sizeof(Ref));
    for (i = 0; i < k; i++) {
        if (!same(tf, A(1), keyed(kf, a[i]))) b[m++] = a[i];
        if (unw != 0) {
            free(a);
            free(b);
            return NIL;
        }
    }
    r = arr2seq(A(2), b, m);
    free(a);
    free(b);
    return r;
}

/* REMOVE-IF (keep when the predicate is false) and REMOVE-IF-NOT */
static Ref removeif(Ref *av, long n, int keepnil)
{
    Ref fn = fnof(A(1)), kf = kwfn(av, n, 3, "KEY"), v, *a, *b, r;
    long k, m = 0, i;
    a = seq2arr(A(2), &k);
    b = (Ref *)xmalloc((k + 1) * (long)sizeof(Ref));
    for (i = 0; i < k; i++) {
        v = call1(fn, keyed(kf, a[i]));
        if (unw != 0) {
            free(a);
            free(b);
            return NIL;
        }
        if ((v == NIL) == keepnil) b[m++] = a[i];
    }
    r = arr2seq(A(2), b, m);
    free(a);
    free(b);
    return r;
}

static Ref pf88(Ref *av, long n)
{
    return removeif(av, n, 1);
}

static Ref pf89(Ref *av, long n)
{
    return removeif(av, n, 0);
}

static Ref pf90(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 2, "TEST"), kf = kwfn(av, n, 2, "KEY");
    Ref *a, *b, r;
    long k, m = 0, i, j;
    int dup;
    a = seq2arr(A(1), &k);
    b = (Ref *)xmalloc((k + 1) * (long)sizeof(Ref));
    for (i = 0; i < k; i++) {
        dup = 0;
        for (j = i + 1; j < k; j++) {
            if (same(tf, keyed(kf, a[i]), keyed(kf, a[j]))) {
                dup = 1;
                break;
            }
        }
        if (!dup) b[m++] = a[i];
    }
    r = arr2seq(A(1), b, m);
    free(a);
    free(b);
    return r;
}

/* position (1-based) of item in the array, 0 if absent */
static long findpos(Ref *a, long n, Ref item, Ref tf, Ref kf,
                    int fromend)
{
    long i;
    if (fromend) {
        for (i = n; i >= 1; i--)
            if (same(tf, item, keyed(kf, a[i - 1]))) return i;
        return 0;
    }
    for (i = 1; i <= n; i++)
        if (same(tf, item, keyed(kf, a[i - 1]))) return i;
    return 0;
}

static Ref pf91(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST"), kf = kwfn(av, n, 3, "KEY");
    Ref *a, r;
    long k, p;
    a = seq2arr(A(2), &k);
    p = findpos(a, k, A(1), tf, kf, 0);
    r = p == 0 ? NIL : a[p - 1];
    free(a);
    return r;
}

static Ref pf92(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST"), kf = kwfn(av, n, 3, "KEY");
    Ref *a;
    long k, p, j;
    int fe = 0;
    a = seq2arr(A(2), &k);
    j = kwidx(av, n, 3, "FROM-END");
    if (j > 0 && A(j) != NIL) fe = 1;
    p = findpos(a, k, A(1), tf, kf, fe);
    free(a);
    if (p == 0) return NIL;
    return mkint(p - 1);
}

static Ref pf93(Ref *av, long n)
{
    Ref fn = fnof(A(1)), kf = kwfn(av, n, 3, "KEY"), *a, r, v;
    long k, i;
    a = seq2arr(A(2), &k);
    for (i = 0; i < k; i++) {
        v = call1(fn, keyed(kf, a[i]));
        if (v != NIL) {
            r = a[i];
            free(a);
            return r;
        }
        if (unw != 0) break;
    }
    free(a);
    return NIL;
}

static Ref pf94(Ref *av, long n)
{
    Ref fn = fnof(A(1)), kf = kwfn(av, n, 3, "KEY"), *a, v;
    long k, i;
    a = seq2arr(A(2), &k);
    for (i = 0; i < k; i++) {
        v = call1(fn, keyed(kf, a[i]));
        if (v != NIL) {
            free(a);
            return mkint(i);
        }
        if (unw != 0) break;
    }
    free(a);
    return NIL;
}

static Ref pf95(Ref *av, long n)
{
    Ref tf = kwfn(av, n, 3, "TEST"), kf = kwfn(av, n, 3, "KEY");
    Ref *a;
    long k, c = 0, i;
    a = seq2arr(A(2), &k);
    for (i = 0; i < k; i++)
        if (same(tf, A(1), keyed(kf, a[i]))) c++;
    free(a);
    return mkint(c);
}

static Ref pf96(Ref *av, long n)
{
    Ref fn = fnof(A(1)), kf = kwfn(av, n, 3, "KEY"), *a;
    long k, c = 0, i;
    a = seq2arr(A(2), &k);
    for (i = 0; i < k; i++) {
        if (call1(fn, keyed(kf, a[i])) != NIL) c++;
        if (unw != 0) break;
    }
    free(a);
    if (unw != 0) return NIL;
    return mkint(c);
}

/* SOME (kind 0), EVERY (1) */
static Ref someevery(Ref *av, long n, int every)
{
    Ref fn = fnof(A(1)), *a, *b = NULL, v;
    long k, m, i;
    a = seq2arr(A(2), &k);
    if (n == 3) {
        b = seq2arr(A(3), &m);
        if (m < k) k = m;
    }
    for (i = 0; i < k; i++) {
        if (n == 3) v = call2(fn, a[i], b[i]);
        else v = call1(fn, a[i]);
        if (unw != 0) break;
        if (!every && v != NIL) {
            free(a);
            free(b);
            return v;
        }
        if (every && v == NIL) {
            free(a);
            free(b);
            return NIL;
        }
    }
    free(a);
    free(b);
    if (unw != 0) return NIL;
    return every ? TRU : NIL;
}

static Ref pf97(Ref *av, long n)
{
    return someevery(av, n, 0);
}

static Ref pf98(Ref *av, long n)
{
    return someevery(av, n, 1);
}

static Ref pf99(Ref *av, long n)
{
    Ref fn = fnof(A(1)), *a;
    long k, i;
    (void)n;
    a = seq2arr(A(2), &k);
    for (i = 0; i < k; i++) {
        if (call1(fn, a[i]) != NIL) {
            free(a);
            return NIL;
        }
        if (unw != 0) break;
    }
    free(a);
    return TRU;
}

static Ref pf100(Ref *av, long n)
{
    Ref fn = fnof(A(1)), *a;
    long k, i;
    (void)n;
    a = seq2arr(A(2), &k);
    for (i = 0; i < k; i++) {
        if (call1(fn, a[i]) == NIL) {
            free(a);
            return TRU;
        }
        if (unw != 0) break;
    }
    free(a);
    return NIL;
}

void initprim_a(void)
{
    primtab[1] = pf1;   primtab[2] = pf2;   primtab[3] = pf3;
    primtab[4] = pf4;   primtab[5] = pf5;   primtab[6] = pf6;
    primtab[7] = pf7;   primtab[8] = pf8;   primtab[9] = pf9;
    primtab[10] = pf10; primtab[11] = pf11; primtab[12] = pf12;
    primtab[13] = pf13; primtab[14] = pf14; primtab[15] = pf15;
    primtab[16] = pf16; primtab[17] = pf17; primtab[18] = pf18;
    primtab[19] = pf19; primtab[20] = pf20; primtab[21] = pf21;
    primtab[22] = pf22; primtab[23] = pf23; primtab[24] = pf24;
    primtab[25] = pf25; primtab[26] = pf26; primtab[27] = pf27;
    primtab[28] = pf28; primtab[29] = pf29; primtab[30] = pf30;
    primtab[31] = pf31; primtab[32] = pf32; primtab[33] = pf33;
    primtab[34] = pf34; primtab[35] = pf35; primtab[36] = pf36;
    primtab[37] = pf37; primtab[38] = pf38; primtab[39] = pf39;
    primtab[40] = pf40; primtab[41] = pf41; primtab[42] = pf42;
    primtab[43] = pf43; primtab[44] = pf44; primtab[45] = pf45;
    primtab[46] = pf46; primtab[47] = pf47; primtab[48] = pf48;
    primtab[49] = pf49; primtab[50] = pf50; primtab[51] = pf51;
    primtab[52] = pf52; primtab[53] = pf53; primtab[54] = pf54;
    primtab[55] = pf55; primtab[56] = pf56; primtab[57] = pf57;
    primtab[58] = pf58; primtab[59] = pf59; primtab[60] = pf60;
    primtab[61] = pf61; primtab[62] = pf62; primtab[63] = pf63;
    primtab[64] = pf64; primtab[65] = pf65; primtab[66] = pf66;
    primtab[67] = pf67; primtab[68] = pf68; primtab[69] = pf69;
    primtab[70] = pf70; primtab[71] = pf71; primtab[72] = pf72;
    primtab[73] = pf73; primtab[74] = pf74; primtab[75] = pf75;
    primtab[76] = pf76; primtab[77] = pf77; primtab[78] = pf78;
    primtab[79] = pf79; primtab[80] = pf80; primtab[81] = pf81;
    primtab[82] = pf82; primtab[83] = pf83; primtab[84] = pf84;
    primtab[85] = pf85; primtab[86] = pf86; primtab[87] = pf87;
    primtab[88] = pf88; primtab[89] = pf89; primtab[90] = pf90;
    primtab[91] = pf91; primtab[92] = pf92; primtab[93] = pf93;
    primtab[94] = pf94; primtab[95] = pf95; primtab[96] = pf96;
    primtab[97] = pf97; primtab[98] = pf98; primtab[99] = pf99;
    primtab[100] = pf100;
}
