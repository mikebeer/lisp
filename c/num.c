/* num.c -- arithmetic, equality, and the hand written math. */
#include "lisp.h"

Ref numadd(Ref a, Ref b)
{
    if (a < 0 && b < 0) return mkint(IADD(intval(a), intval(b)));
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    return mkfloat(fval(a) + fval(b));
}

Ref numsub(Ref a, Ref b)
{
    if (a < 0 && b < 0) return mkint(ISUB(intval(a), intval(b)));
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    return mkfloat(fval(a) - fval(b));
}

Ref nummul(Ref a, Ref b)
{
    if (a < 0 && b < 0) return mkint(IMUL(intval(a), intval(b)));
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    return mkfloat(fval(a) * fval(b));
}

Ref numneg(Ref a)
{
    return numsub(mkint(0), a);
}

#define DIVZERO "arithmetic error DIVISION-BY-ZERO signalled"

Ref numdiv(Ref a, Ref b)
{
    long x, y;
    double d;
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    if (a < 0 && b < 0) {
        x = intval(a);
        y = intval(b);
        if (y == 0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
        if (x % y == 0) return mkint(x / y);
        return mkfloat((double)x / ((double)y * 1.0));
    }
    d = fval(b);
    if (d == 0.0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
    return mkfloat(fval(a) / d);
}

/* -1, 0, 1 */
int numcmp(Ref a, Ref b)
{
    long x, y;
    double u, v;
    if (a < 0 && b < 0) {
        x = intval(a);
        y = intval(b);
        if (x < y) return -1;
        if (x > y) return 1;
        return 0;
    }
    if (!ISNUM(a)) {
        notnum(a);
        return 0;
    }
    if (!ISNUM(b)) {
        notnum(b);
        return 0;
    }
    u = fval(a);
    v = fval(b);
    if (u < v) return -1;
    if (u > v) return 1;
    return 0;
}

/* "trunc(x) as .int" of the cREXX VM: trunc() takes the decimal
   text of x with 16 digits, so a large x loses its low digits and a
   value just under an integer can round up to it */
long f2i(double t)
{
    char b[48];
    double at = t < 0.0 ? 0.0 - t : t;
    long v = 0, e = 0, nd = 0, pt = -1, k, i;
    char dg[40];
    if (at < 1e9) {         /* the usual case needs no text */
        v = (long)at;
        e = 0;
        k = (long)((at - (double)v) * 1e6);
        if (k > 0 && k < 999999) return t < 0.0 ? 0 - v : v;
    }
    sprintf(b, "%.16g", at);
    for (i = 0; b[i] != 0 && b[i] != 'e'; i++) {
        if (b[i] == '.') pt = nd;
        else dg[nd++] = b[i];
    }
    if (pt < 0) pt = nd;
    if (b[i] == 'e') e = atol(b + i + 1);
    else e = 0;
    k = pt + e;                 /* digits before the point */
    v = 0;
    for (i = 0; i < k; i++) {
        if (v > LONG_MAX / 10) return t < 0.0 ? LONG_MIN : LONG_MAX;
        v = v * 10 + (i < nd ? dg[i] - '0' : 0);
    }
    return t < 0.0 ? 0 - v : v;
}

/* integer or float to a C integer (truncating) */
long toint(Ref a)
{
    if (a < 0) return intval(a);
    if (ISFLOAT(a)) {
        return f2i(XF(a));
    }
    lerrobj("TYPE-ERROR", "the value ", a, " is not of type INTEGER");
    return 0;
}

/* ------------------------------------------------------- equality */
int eql(Ref a, Ref b)
{
    if (a == b) return 1;
    if (a < 0 || b < 0) return 0;
    if (a == 0 || b == 0) return 0;
    if (ctag[a] != ctag[b]) return 0;
    if (ctag[a] == T_FLOAT) return XF(a) == XF(b);
    if (ctag[a] == T_CHAR) return XL(a) == XL(b);
    return 0;
}

static int vecsame(Ref a, Ref b, int (*f)(Ref, Ref))
{
    long i;
    Ref *p, *q;
    if (ccdr[a] != ccdr[b]) return 0;
    p = vdata(a);
    q = vdata(b);
    for (i = 0; i < ccdr[a]; i++)
        if (!f(p[i], q[i])) return 0;
    return 1;
}

int equal(Ref a, Ref b)
{
    int t;
    for (;;) {
        if (eql(a, b)) return 1;
        if (a < 0 || b < 0 || a == 0 || b == 0) return 0;
        if (ctag[a] != ctag[b]) return 0;
        t = ctag[a];
        if (t == T_STR)
            return ccdr[a] == ccdr[b] &&
                   memcmp(XS(a), XS(b), ccdr[a]) == 0;
        if (t == T_CONS) {
            if (!equal(ccar[a], ccar[b])) return 0;
            a = ccdr[a];
            b = ccdr[b];
        } else if (t == T_VEC) {
            return vecsame(a, b, equal);
        } else return 0;
    }
}

static int upc(int c)
{
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

int equalp(Ref a, Ref b)
{
    int t;
    long i;
    for (;;) {
        if (eql(a, b)) return 1;
        if (ISNUM(a) && ISNUM(b)) return fval(a) == fval(b);
        if (a < 0 || b < 0 || a == 0 || b == 0) return 0;
        if (ctag[a] != ctag[b]) return 0;
        t = ctag[a];
        if (t == T_STR) {
            if (ccdr[a] != ccdr[b]) return 0;
            for (i = 0; i < ccdr[a]; i++)
                if (upc((unsigned char)XS(a)[i]) !=
                    upc((unsigned char)XS(b)[i])) return 0;
            return 1;
        }
        if (t == T_CHAR) {
            if (XL(a) < 0 || XL(a) > 255 || XL(b) < 0 ||
                XL(b) > 255) return XL(a) == XL(b);
            return upc((int)XL(a)) == upc((int)XL(b));
        }
        if (t == T_CONS) {
            if (!equalp(ccar[a], ccar[b])) return 0;
            a = ccdr[a];
            b = ccdr[b];
        } else if (t == T_VEC) {
            return vecsame(a, b, equalp);
        } else return 0;
    }
}

/* ------------------------------------------------------------ math */
double lsqrt(double x)
{
    double m, r;
    long e = 0;
    int i;
    if (x <= 0.0) return 0.0;
    m = x;
    while (m >= 4.0) {
        m = m / 4.0;
        e++;
    }
    while (m < 0.25) {
        m = m * 4.0;
        e--;
    }
    r = (1.0 + m) / 2.0;
    for (i = 0; i < 8; i++) r = (r + m / r) / 2.0;
    while (e > 0) {
        r = r * 2.0;
        e--;
    }
    while (e < 0) {
        r = r / 2.0;
        e++;
    }
    return r;
}

double lsin(double x)
{
    double pi = 3.14159265358979323846;
    double twopi = 6.28318530717958647692;
    double r = x, r2, term, sum;
    long q, k;
    if (r > twopi || r < 0.0 - twopi) {
        q = f2i(r / twopi);
        r = r - q * twopi;
    }
    if (r > pi) r = r - twopi;
    if (r < 0.0 - pi) r = r + twopi;
    if (r > pi / 2.0) r = pi - r;
    if (r < 0.0 - pi / 2.0) r = 0.0 - pi - r;
    r2 = r * r;
    term = r;
    sum = r;
    for (k = 1; k <= 12; k++) {
        term = 0.0 - term * r2 / (double)((2 * k) * (2 * k + 1));
        sum = sum + term;
    }
    return sum;
}

double lcos(double x)
{
    return lsin(x + 1.57079632679489661923);
}

double latan(double x)
{
    int neg = 0, inv = 0, i;
    double a = x, a2, term, sum;
    long k;
    if (a < 0.0) {
        neg = 1;
        a = 0.0 - a;
    }
    if (a > 1.0) {
        inv = 1;
        a = 1.0 / a;
    }
    for (i = 0; i < 3; i++) a = a / (1.0 + lsqrt(1.0 + a * a));
    a2 = a * a;
    term = a;
    sum = a;
    for (k = 1; k <= 14; k++) {
        term = 0.0 - term * a2;
        sum = sum + term / (double)(2 * k + 1);
    }
    sum = sum * 8.0;
    if (inv) sum = 1.57079632679489661923 - sum;
    if (neg) sum = 0.0 - sum;
    return sum;
}

double lln(double x)
{
    long e = 0, k;
    double m = x, z, z2, term, sum;
    while (m >= 2.0) {
        m = m / 2.0;
        e++;
    }
    while (m < 0.5) {
        m = m * 2.0;
        e--;
    }
    z = (m - 1.0) / (m + 1.0);
    z2 = z * z;
    term = z;
    sum = z;
    for (k = 1; k <= 30; k++) {
        term = term * z2;
        sum = sum + term / (double)(2 * k + 1);
    }
    return 2.0 * sum + (double)e * 0.69314718055994530942;
}

double lexp(double x)
{
    long k = 0, i, j;
    double r = x, term = 1.0, sum = 1.0;
    while (r > 0.5 || r < -0.5) {
        r = r / 2.0;
        k++;
    }
    for (i = 1; i <= 20; i++) {
        term = term * r / (double)i;
        sum = sum + term;
    }
    for (j = 0; j < k; j++) sum = sum * sum;
    return sum;
}

/* does a * b fit a long? */
static int mulok(long a, long b)
{
    if (a == 0 || b == 0) return 1;
    if (a == -1) return b != LONG_MIN;
    if (b == -1) return a != LONG_MIN;
    if (a > 0) {
        if (b > 0) return a <= LONG_MAX / b;
        return b >= LONG_MIN / a;
    }
    if (b > 0) return a >= LONG_MIN / b;
    return a >= LONG_MAX / b;
}

long imul(long a, long b)
{
    if (!mulok(a, b)) return 0;
    return a * b;
}

/* integer power by repeated squaring */
long ipow(long b, long e)
{
    long r = 1;
    while (e > 0) {
        if (e % 2 == 1 && mulok(r, b)) r = r * b;
        if (mulok(b, b)) b = b * b;
        e = e / 2;
    }
    return r;
}

long nextrand(void)
{
    unsigned long s = (unsigned long)seed;
    s = (s * 1103515245UL + 12345UL) & 0x7fffffffUL;
    seed = (long)s;
    return seed;
}
