/* prim_num.c -- primitives 113-158: numbers and characters. */
#include "lisp.h"

#define A(i) av[(i) - 1]
#define DIVZERO "arithmetic error DIVISION-BY-ZERO signalled"

static Ref pf113(Ref *av, long n)
{
    Ref r = A(1);
    long i;
    for (i = 2; i <= n; i++)
        if (numcmp(A(i), r) > 0) r = A(i);
    if (unw != 0) return NIL;
    return r;
}

static Ref pf114(Ref *av, long n)
{
    Ref r = A(1);
    long i;
    for (i = 2; i <= n; i++)
        if (numcmp(A(i), r) < 0) r = A(i);
    if (unw != 0) return NIL;
    return r;
}

static Ref pf115(Ref *av, long n)
{
    (void)n;
    if (numcmp(A(1), mkint(0)) < 0) return numneg(A(1));
    return A(1);
}

static Ref pf116(Ref *av, long n)
{
    int c = numcmp(A(1), mkint(0));
    (void)n;
    if (ISFLOAT(A(1))) return mkfloat((double)c);
    return mkint(c);
}

/* FLOOR (kind 0) CEILING (1) TRUNCATE (2) */
static Ref divop(Ref *av, long n, int kind)
{
    Ref a = A(1), b;
    long x, y, q;
    double fx, fb;
    b = n == 2 ? A(2) : mkint(1);
    if (a < 0 && b < 0) {
        x = intval(a);
        y = intval(b);
        if (y == 0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
        q = x / y;
        if (kind == 0 && x % y != 0 && ((x < 0) != (y < 0))) q--;
        if (kind == 1 && x % y != 0 && ((x < 0) == (y < 0))) q++;
        mvlist = list2(mkint(q), mkint(ISUB(x, IMUL(q, y))));
        return mkint(q);
    }
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    fb = fval(b);
    if (fb == 0.0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
    fx = fval(a) / fb;
    q = f2i(fx);
    if (kind == 0 && (double)q > fx) q--;
    if (kind == 1 && (double)q < fx) q++;
    mvlist = list2(mkint(q), mkfloat(fval(a) - q * fb));
    return mkint(q);
}

static Ref pf117(Ref *av, long n)
{
    return divop(av, n, 0);
}

static Ref pf118(Ref *av, long n)
{
    return divop(av, n, 1);
}

static Ref pf119(Ref *av, long n)
{
    return divop(av, n, 2);
}

static Ref pf120(Ref *av, long n)
{
    Ref a = A(1), b;
    double x, fr, fb;
    long q;
    b = n == 2 ? A(2) : mkint(1);
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    fb = fval(b);
    if (fb == 0.0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
    x = fval(a) / fb;
    q = f2i(x);
    fr = x - (double)q;
    if (fr < 0.0) fr = 0.0 - fr;
    if (fr > 0.5) {
        if (x > 0.0) q++;
        else q--;
    } else if (fr == 0.5) {
        if (q % 2 != 0) {
            if (x > 0.0) q++;
            else q--;
        }
    }
    mvlist = list2(mkint(q), mkfloat(fval(a) - q * fb));
    return mkint(q);
}

static Ref pf121(Ref *av, long n)
{
    Ref a = A(1), b = A(2);
    long x, y, q;
    double fx, fb;
    (void)n;
    if (a < 0 && b < 0) {
        x = intval(a);
        y = intval(b);
        if (y == 0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
        q = x / y;
        if (x % y != 0 && ((x < 0) != (y < 0))) q--;
        return mkint(ISUB(x, IMUL(q, y)));
    }
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    fb = fval(b);
    if (fb == 0.0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
    fx = fval(a) / fb;
    q = f2i(fx);
    if ((double)q > fx) q--;
    return mkfloat(fval(a) - q * fb);
}

static Ref pf122(Ref *av, long n)
{
    Ref a = A(1), b = A(2);
    long y, q;
    double x, fb;
    (void)n;
    if (a < 0 && b < 0) {
        y = intval(b);
        if (y == 0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
        return mkint(intval(a) % y);
    }
    if (!ISNUM(a)) return notnum(a);
    if (!ISNUM(b)) return notnum(b);
    fb = fval(b);
    if (fb == 0.0) return lerrk("DIVISION-BY-ZERO", DIVZERO);
    x = fval(a) / fb;
    q = f2i(x);
    return mkfloat(fval(a) - q * fb);
}

static Ref pf123(Ref *av, long n)
{
    double x;
    (void)n;
    if (!ISNUM(A(1))) return notnum(A(1));
    x = fval(A(1));
    if (x < 0.0)
        return lerrk("ERROR", "SQRT of a negative number is not "
                              "supported");
    return mkfloat(lsqrt(x));
}

static Ref pf124(Ref *av, long n)
{
    long k = toint(A(1)), r;
    (void)n;
    if (unw != 0) return NIL;
    if (k < 0) return lerror("ISQRT of a negative number");
    r = f2i(lsqrt((double)k * 1.0));
    while (r * r > k) r--;
    while ((r + 1) * (r + 1) <= k) r++;
    return mkint(r);
}

static Ref pf125(Ref *av, long n)
{
    (void)n;
    return mkfloat(lexp(fval(A(1))));
}

static Ref pf126(Ref *av, long n)
{
    double x;
    x = fval(A(1));
    if (x <= 0.0)
        return lerrk("DIVISION-BY-ZERO",
                     "LOG of a non-positive number");
    if (n == 2) {
        return mkfloat(lln(x) / lln(fval(A(2))));
    }
    return mkfloat(lln(x));
}

static Ref pf127(Ref *av, long n)
{
    (void)n;
    return mkfloat(lsin(fval(A(1))));
}

static Ref pf128(Ref *av, long n)
{
    (void)n;
    return mkfloat(lcos(fval(A(1))));
}

static Ref pf129(Ref *av, long n)
{
    double x;
    (void)n;
    x = fval(A(1));
    return mkfloat(lsin(x) / lcos(x));
}

static Ref pf130(Ref *av, long n)
{
    double y, x, pi = 3.14159265358979323846;
    if (n == 1) return mkfloat(latan(fval(A(1))));
    y = fval(A(1));
    x = fval(A(2));
    if (x > 0.0) return mkfloat(latan(y / x));
    if (x < 0.0) {
        if (y >= 0.0) return mkfloat(latan(y / x) + pi);
        return mkfloat(latan(y / x) - pi);
    }
    if (y > 0.0) return mkfloat(pi / 2.0);
    if (y < 0.0) return mkfloat(0.0 - pi / 2.0);
    return mkfloat(0.0);
}

static Ref pf131(Ref *av, long n)
{
    Ref b = A(1), e = A(2);
    long k, m, i;
    double x, r;
    (void)n;
    if (b < 0 && e < 0) {
        k = intval(e);
        if (k >= 0) return mkint(ipow(intval(b), k));
        return mkfloat(1.0 / (double)ipow(intval(b), 0 - k));
    }
    if (!ISNUM(b)) return notnum(b);
    if (!ISNUM(e)) return notnum(e);
    if (e < 0) {
        k = intval(e);
        x = fval(b);
        r = 1.0;
        m = k;
        if (m < 0) m = 0 - m;
        for (i = 0; i < m; i++) r = r * x;
        if (k < 0) r = 1.0 / r;
        return mkfloat(r);
    }
    x = fval(b);
    if (x == 0.0) return mkfloat(0.0);
    if (x < 0.0)
        return lerrk("ERROR", "EXPT of a negative number to a float "
                              "power is not supported");
    return mkfloat(lexp(fval(e) * lln(x)));
}

static Ref pf132(Ref *av, long n)
{
    (void)n;
    if (!ISNUM(A(1))) return notnum(A(1));
    return mkfloat(fval(A(1)));
}

static Ref pf133(Ref *av, long n)
{
    long r = 0, a, b, t, i;
    for (i = 1; i <= n; i++) {
        a = toint(A(i));
        if (a < 0) a = 0 - a;
        b = r;
        while (a != 0) {
            t = b % a;
            b = a;
            a = t;
        }
        r = b;
    }
    return mkint(r);
}

static Ref pf134(Ref *av, long n)
{
    long r = 1, a, x, y, t, i;
    for (i = 1; i <= n; i++) {
        a = toint(A(i));
        if (a < 0) a = 0 - a;
        if (a == 0) return mkint(0);
        x = r;
        y = a;
        while (y != 0) {
            t = x % y;
            x = y;
            y = t;
        }
        r = r / x * a;
    }
    return mkint(r);
}

static Ref pf135(Ref *av, long n)
{
    Ref x = A(1);
    long k;
    (void)n;
    if (x < 0) {
        k = intval(x);
        if (k <= 0) return lerror("RANDOM needs a positive limit");
        return mkint(nextrand() % k);
    }
    if (!ISNUM(x)) return notnum(x);
    return mkfloat((double)nextrand() / 2147483648.0 * fval(x));
}

static Ref pf136(Ref *av, long n)
{
    long a = toint(A(1)), s = toint(A(2)), d, q;
    (void)n;
    if (s >= 0) return mkint(IMUL(a, ipow(2, s)));
    d = ipow(2, 0 - s);
    if (d == 0) return mkint(a < 0 ? -1 : 0);
    q = a / d;
    if (a % d != 0 && a < 0) q--;
    return mkint(q);
}

/* LOGAND (1) LOGIOR (2) LOGXOR (3) */
static Ref logop(Ref *av, long n, int op)
{
    long r, i, b;
    if (n == 0) return mkint(op == 1 ? -1 : 0);
    r = toint(A(1));
    for (i = 2; i <= n; i++) {
        b = toint(A(i));
        if (op == 1) r = r & b;
        else if (op == 2) r = r | b;
        else r = r ^ b;
    }
    return mkint(r);
}

static Ref pf137(Ref *av, long n)
{
    return logop(av, n, 1);
}

static Ref pf138(Ref *av, long n)
{
    return logop(av, n, 2);
}

static Ref pf139(Ref *av, long n)
{
    return logop(av, n, 3);
}

static Ref pf140(Ref *av, long n)
{
    (void)n;
    return mkint(ISUB(0 - toint(A(1)), 1));
}

/* PARSE-INTEGER */
static Ref pf141(Ref *av, long n)
{
    long j = kwidx(av, n, 2, "JUNK-ALLOWED"), len, tl, i, k;
    int junk = 0;
    char cb[2];
    const char *s;
    char *t;
    Sb m;
    Ref r;
    if (j > 0 && A(j) != NIL) junk = 1;
    s = strof(A(1), &len, cb);
    if (unw != 0) return NIL;
    i = 0;
    tl = len;
    while (i < tl && s[i] == ' ') i++;
    while (tl > i && s[tl - 1] == ' ') tl--;
    t = xstrndup(s + i, tl - i);
    tl = tl - i;
    if (junk) {
        k = 0;
        if (tl > 0 && (t[0] == '-' || t[0] == '+')) k = 1;
        j = k;
        while (j < tl && t[j] >= '0' && t[j] <= '9') j++;
        if (j == k) {
            free(t);
            return NIL;
        }
        t[j] = 0;
        tl = j;
    }
    sb_init(&m);
    if (tl == 0) {
        sb_adds(&m, "no integer in ");
        printstring(&m, s, len);
        lerror(sb_str(&m));
        free(m.p);
        free(t);
        return NIL;
    }
    k = 0;
    if (t[0] == '-' || t[0] == '+') k = 1;
    for (j = k; j < tl; j++)
        if (t[j] < '0' || t[j] > '9') break;
    if (j == tl && tl > k) {
        r = mkint(strtol(t, NULL, 10));
        free(t);
        free(m.p);
        return r;
    }
    sb_adds(&m, "junk in ");
    printstring(&m, s, len);
    lerror(sb_str(&m));
    free(m.p);
    free(t);
    return NIL;
}

/* ---------------------------------------------------- characters */
/* the cREXX code reads the char field of whatever it is given;
   anything but a character reads as 0 there */
long chr(Ref x)
{
    if (ISCHAR(x)) return XL(x);
    if (x > 0) return 0;
    lerrobj("TYPE-ERROR", "the value ", x, " is not a character");
    return 0;
}

static Ref pf142(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    return mkint(c);
}

static Ref pf143(Ref *av, long n)
{
    long c = toint(A(1));
    (void)n;
    if (unw != 0) return NIL;
    return mkchar(c);
}

static Ref pf144(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    if (c >= 97 && c <= 122) c -= 32;
    return mkchar(c);
}

static Ref pf145(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    if (c >= 65 && c <= 90) c += 32;
    return mkchar(c);
}

static int isalpha_(long c)
{
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

static Ref pf146(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(isalpha_(c));
}

static Ref pf147(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    if (c >= 48 && c <= 57) return mkint(c - 48);
    return NIL;
}

static Ref pf148(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(isalpha_(c) || (c >= 48 && c <= 57));
}

static Ref pf149(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c >= 65 && c <= 90);
}

static Ref pf150(Ref *av, long n)
{
    long c = chr(A(1));
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(c >= 97 && c <= 122);
}

static Ref pf151(Ref *av, long n)
{
    long d = toint(A(1));
    (void)n;
    if (unw != 0) return NIL;
    if (d < 0 || d > 35) return lerror("bad digit weight");
    return mkchar("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[d]);
}

/* character comparisons: op 0 = /= 1 < 2 > 3 <= 4 >= 5 */
static Ref charcmp(Ref *av, long n, int op)
{
    long i, a, b;
    int bad;
    for (i = 1; i < n; i++) {
        a = chr(A(i));
        b = chr(A(i + 1));
        if (unw != 0) return NIL;
        switch (op) {
        case 0: bad = a != b; break;       /* CHAR= */
        case 1: bad = a >= b; break;       /* CHAR< */
        case 2: bad = a <= b; break;       /* CHAR> */
        case 3: bad = a > b; break;        /* CHAR<= */
        default: bad = a < b; break;       /* CHAR>= */
        }
        if (bad) return NIL;
    }
    if (n == 1) chr(A(1));
    if (unw != 0) return NIL;
    return TRU;
}

static Ref pf152(Ref *av, long n)
{
    return charcmp(av, n, 0);
}

static Ref pf153(Ref *av, long n)
{
    long a = chr(A(1)), b = chr(A(2));
    (void)n;
    if (unw != 0) return NIL;
    return TRUTH(a != b);
}

static Ref pf154(Ref *av, long n)
{
    return charcmp(av, n, 1);
}

static Ref pf155(Ref *av, long n)
{
    return charcmp(av, n, 2);
}

static Ref pf156(Ref *av, long n)
{
    return charcmp(av, n, 3);
}

static Ref pf157(Ref *av, long n)
{
    return charcmp(av, n, 4);
}

static long upc(long c)
{
    if (c >= 97 && c <= 122) return c - 32;
    return c;
}

static Ref pf158(Ref *av, long n)
{
    long i, a, b;
    for (i = 1; i < n; i++) {
        a = chr(A(i));
        b = chr(A(i + 1));
        if (unw != 0) return NIL;
        if (upc(a) != upc(b)) return NIL;
    }
    return TRU;
}

void initprim_c(void)
{
    primtab[113] = pf113; primtab[114] = pf114; primtab[115] = pf115;
    primtab[116] = pf116; primtab[117] = pf117; primtab[118] = pf118;
    primtab[119] = pf119; primtab[120] = pf120; primtab[121] = pf121;
    primtab[122] = pf122; primtab[123] = pf123; primtab[124] = pf124;
    primtab[125] = pf125; primtab[126] = pf126; primtab[127] = pf127;
    primtab[128] = pf128; primtab[129] = pf129; primtab[130] = pf130;
    primtab[131] = pf131; primtab[132] = pf132; primtab[133] = pf133;
    primtab[134] = pf134; primtab[135] = pf135; primtab[136] = pf136;
    primtab[137] = pf137; primtab[138] = pf138; primtab[139] = pf139;
    primtab[140] = pf140; primtab[141] = pf141; primtab[142] = pf142;
    primtab[143] = pf143; primtab[144] = pf144; primtab[145] = pf145;
    primtab[146] = pf146; primtab[147] = pf147; primtab[148] = pf148;
    primtab[149] = pf149; primtab[150] = pf150; primtab[151] = pf151;
    primtab[152] = pf152; primtab[153] = pf153; primtab[154] = pf154;
    primtab[155] = pf155; primtab[156] = pf156; primtab[157] = pf157;
    primtab[158] = pf158;
}
