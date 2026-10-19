/* prim_seq.c -- helpers shared by the primitives: sequences,
   keyword arguments, string designators, sorting. */
#include "lisp.h"

/* sequence to a new array; *n gets the count */
Ref *seq2arr(Ref s, long *n)
{
    Ref *a;
    long i, k;
    if (s == NIL) {
        *n = 0;
        return (Ref *)xmalloc(sizeof(Ref));
    }
    if (ISCONS(s)) return list2arr(s, n);
    if (s > 0 && ctag[s] == T_VEC) {
        k = ccdr[s];
        a = (Ref *)xmalloc((k + 1) * (long)sizeof(Ref));
        for (i = 0; i < k; i++) a[i] = vdata(s)[i];
        *n = k;
        return a;
    }
    if (s > 0 && ctag[s] == T_STR) {
        k = SLEN(s);
        a = (Ref *)xmalloc((k + 1) * (long)sizeof(Ref));
        for (i = 0; i < k; i++) {
            Ref c = mkchar((unsigned char)XS(s)[i]);
            a[i] = c;
        }
        *n = k;
        return a;
    }
    lerrobj("TYPE-ERROR", "the value ", s, " is not a sequence");
    *n = 0;
    return (Ref *)xmalloc(sizeof(Ref));
}

/* build a sequence like proto (list, vector, string) from an array */
Ref arr2seq(Ref proto, Ref *a, long n)
{
    Sb b;
    Ref r;
    long i;
    if (proto > 0) {
        if (ctag[proto] == T_VEC) return mkvec(a, n);
        if (ctag[proto] == T_STR) {
            sb_init(&b);
            for (i = 0; i < n; i++)
                if (ISCHAR(a[i])) sb_addc(&b, (int)XL(a[i]));
            sb_str(&b);
            r = mkstrn(b.p, b.n);
            free(b.p);
            return r;
        }
    }
    return arr2list(a, 1, n);
}

/* index (1-based) of the value of keyword argument kname at or
   after position from, 0 if absent */
long kwidx(Ref *av, long n, long from, const char *kname)
{
    Sb b;
    Ref kw;
    long j;
    sb_init(&b);
    sb_addc(&b, ':');
    sb_adds(&b, kname);
    kw = intern(sb_str(&b));
    free(b.p);
    for (j = from; j < n; j += 2)
        if (av[j - 1] == kw) return j + 1;
    return 0;
}

/* the :test / :key function of a call; 0 means default */
Ref kwfn(Ref *av, long n, long from, const char *kname)
{
    long j = kwidx(av, n, from, kname);
    if (j == 0) return 0;
    if (av[j - 1] == NIL) return 0;
    return fnof(av[j - 1]);
}

int same(Ref tf, Ref a, Ref b)
{
    if (tf == 0) return eql(a, b);
    return call2(tf, a, b) != NIL;
}

Ref keyed(Ref kf, Ref x)
{
    if (kf == 0) return x;
    return call1(kf, x);
}

/* a string from a string designator; NULL (and an error) if x is
   none.  cb is a 2 byte buffer for characters. */
const char *strof(Ref x, long *len, char *cb)
{
    if (x > 0) {
        if (ctag[x] == T_STR) {
            *len = SLEN(x);
            return XS(x);
        }
        if (ctag[x] == T_SYM) {
            *len = SYM(x).len;
            return SYMNAME(x);
        }
        if (ctag[x] == T_CHAR) {
            cb[0] = (char)XL(x);
            cb[1] = 0;
            *len = 1;
            return cb;
        }
    }
    lerrobj("TYPE-ERROR", "the value ", x,
            " is not a string designator");
    *len = 0;
    return "";
}

Ref subseqn(Ref s, long st, long en)
{
    Ref *a, r;
    long n;
    char m[80];
    a = seq2arr(s, &n);
    if (unw != 0) {
        free(a);
        return NIL;
    }
    if (en < 0) en = n;
    if (st < 0 || st > en || en > n) {
        free(a);
        sprintf(m, "bad subsequence bounds %ld %ld", st, en);
        return lerror(m);
    }
    r = arr2seq(s, a + st, en - st);
    free(a);
    return r;
}

Ref copytree(Ref x)
{
    Ref r = NIL, t;
    if (!ISCONS(x)) return x;
    while (ISCONS(x)) {
        t = copytree(ccar[x]);
        r = cons(t, r);
        x = ccdr[x];
    }
    return nreverse(r);
}

/* merge sort of a[0..n-1] with the predicate pf and key kf; stable */
void msort(Ref *a, long n, Ref pf, Ref kf)
{
    Ref *b = (Ref *)xmalloc((n + 1) * (long)sizeof(Ref));
    long w = 1, i, lo, mid, hi, l, r, k, j;
    Ref x, y;
    while (w < n) {
        i = 1;
        while (i <= n) {
            lo = i;
            mid = i + w;
            hi = i + 2 * w;
            if (mid > n + 1) mid = n + 1;
            if (hi > n + 1) hi = n + 1;
            l = lo;
            r = mid;
            k = lo;
            while (l < mid && r < hi) {
                x = keyed(kf, a[r - 1]);
                y = keyed(kf, a[l - 1]);
                if (call2(pf, x, y) != NIL) {
                    b[k - 1] = a[r - 1];
                    r++;
                } else {
                    b[k - 1] = a[l - 1];
                    l++;
                }
                k++;
                if (unw != 0) {
                    free(b);
                    return;
                }
            }
            while (l < mid) b[k++ - 1] = a[l++ - 1];
            while (r < hi) b[k++ - 1] = a[r++ - 1];
            i += 2 * w;
        }
        for (j = 0; j < n; j++) a[j] = b[j];
        w *= 2;
    }
    free(b);
}
