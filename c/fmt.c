/* fmt.c -- FORMAT.  The directives, their quirks and the use of
   one shared argument index (fidx) follow the cREXX version. */
#include "lisp.h"

#define SKIP (-999999L)
#define MAXPM 32
#define MAXSEMI 256

static void fmtrun(Sb *res, const char *ctl, long len, Ref *fav,
                   long fnn);

static const char DIGS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static void radixstr(Sb *o, long n, int base)
{
    char t[72];
    int k = 0;
    long q;
    if (n == 0) {
        sb_addc(o, '0');
        return;
    }
    if (n < 0) {
        sb_addc(o, '-');
        n = 0 - n;
    }
    while (n > 0) {
        q = n / base;
        t[k++] = DIGS[n - q * base];
        n = q;
    }
    while (k > 0) sb_addc(o, t[--k]);
}

/* the digits of s with commas every three places */
static void commas(Sb *o, const char *s, long n)
{
    long i, st = 0;
    if (n > 0 && s[0] == '-') {
        sb_addc(o, '-');
        st = 1;
    }
    for (i = st; i < n; i++) {
        if (i > st && (n - i) % 3 == 0) sb_addc(o, ',');
        sb_addc(o, s[i]);
    }
}

/* x with d digits after the decimal point */
static void fixedfmt(Sb *o, double x, long d)
{
    int neg = 0;
    double sc = 1;
    long i, r, len;
    char t[64];
    if (x < 0.0) {
        neg = 1;
        x = 0.0 - x;
    }
    for (i = 0; i < d; i++) sc = sc * 10;
    r = f2i(x * sc + 0.5);
    sprintf(t, "%ld", r);
    len = (long)strlen(t);
    if (neg && r != 0) sb_addc(o, '-');
    if (d > 0) {
        while (len <= d) {
            memmove(t + 1, t, (size_t)len + 1);
            t[0] = '0';
            len++;
        }
        sb_addn(o, t, len - d);
        sb_addc(o, '.');
        sb_addn(o, t + len - d, d);
    } else sb_addn(o, t, len);
}

static void fmtpad(Sb *o, const char *s, long n, long w, int padc,
                   int leftpad)
{
    long i;
    if (leftpad && w > n) for (i = n; i < w; i++) sb_addc(o, padc);
    sb_addn(o, s, n);
    if (!leftpad && w > n) for (i = n; i < w; i++) sb_addc(o, padc);
}

typedef struct Semis {
    long pos[MAXSEMI];
    char col[MAXSEMI];
    int n;
} Semis;

/* find the closing directive for an opener at or after start;
   returns its '~' index or -1 */
static long fmtfind(const char *ctl, long len, long start, Semis *sm)
{
    long i = start, j;
    int depth = 0, colon, dch;
    sm->n = 0;
    while (i < len) {
        if (ctl[i] != '~') {
            i++;
            continue;
        }
        j = i + 1;
        colon = 0;
        while (j < len && strchr("0123456789,-+#vV:@", ctl[j])) {
            if (ctl[j] == ':') colon = 1;
            j++;
        }
        if (j < len && ctl[j] == '\'') j += 2;
        while (j < len && (ctl[j] == ':' || ctl[j] == '@')) {
            if (ctl[j] == ':') colon = 1;
            j++;
        }
        dch = j < len ? ctl[j] : 0;
        if (dch && strchr("[{(<", dch)) depth++;
        else if (dch && strchr("]})>", dch)) {
            if (depth == 0) return i;
            depth--;
        } else if (dch == ';' && depth == 0 && sm->n < MAXSEMI) {
            sm->pos[sm->n] = i;
            sm->col[sm->n] = (char)colon;
            sm->n++;
        }
        i = j + 1;
    }
    return -1;
}

static Ref fmtarg(Ref *fav, long fnn)
{
    Ref v;
    if (fidx > fnn) return lerror("not enough arguments for FORMAT");
    v = fav[fidx - 1];
    fidx++;
    return v;
}

void capitalize(Sb *o, const char *s, long n)
{
    long k;
    int prev = ' ', c;
    for (k = 0; k < n; k++) {
        c = (unsigned char)s[k];
        if (strchr(" -_/.,;:!?()", prev) && prev != 0) {
            if (c >= 'a' && c <= 'z') c -= 32;
        } else if (c >= 'A' && c <= 'Z') c += 32;
        sb_addc(o, c);
        prev = c;
    }
}

static void fmtiter(Sb *out, const char *ctl, long len, long *pi,
                    int at, Ref *fav, long fnn)
{
    Semis sm;
    long e, i = *pi, sn, k, savei, before;
    Ref l, *sub;
    const char *body;
    e = fmtfind(ctl, len, i, &sm);
    if (e < 0) {
        lerror("unterminated ~{ in FORMAT");
        *pi = len;
        return;
    }
    body = ctl + i;
    *pi = e + 2;
    l = fmtarg(fav, fnn);
    if (unw != 0) return;
    if (at) {
        sn = fnn - fidx + 1;
        if (sn < 0) sn = 0;
        sub = (Ref *)xmalloc((sn + 1) * (long)sizeof(Ref));
        for (k = 0; k < sn; k++) sub[k] = fav[fidx + k - 1];
        fidx = fnn + 1;
    } else sub = list2arr(l, &sn);
    savei = fidx;
    fidx = 1;
    while (fidx <= sn) {
        before = fidx;
        fmtrun(out, body, e - i, sub, sn);
        if (unw != 0) {
            free(sub);
            return;
        }
        if (fstop) {
            fstop = 0;
            break;
        }
        if (fidx == before) break;
    }
    free(sub);
    if (!at) fidx = savei;
    else fidx = fnn + 1;
}

static void fmtcond(Sb *out, const char *ctl, long len, long *pi,
                    int colon, int at, Ref *fav, long fnn)
{
    Semis sm;
    long e, i = *pi, st, nc = 0, dflt = 0, p, j, k, pick;
    const char *cp[MAXSEMI + 1];
    long cl[MAXSEMI + 1];
    Ref v;
    e = fmtfind(ctl, len, i, &sm);
    if (e < 0) {
        lerror("unterminated ~[ in FORMAT");
        *pi = len;
        return;
    }
    st = i;
    for (k = 0; k < sm.n; k++) {
        p = sm.pos[k];
        cp[nc] = ctl + st;
        cl[nc] = p - st;
        nc++;
        if (sm.col[k]) dflt = nc + 1;
        j = p + 1;
        while (j < len && ctl[j] != ';') j++;
        st = j + 1;
    }
    cp[nc] = ctl + st;
    cl[nc] = e - st;
    nc++;
    *pi = e + 2;
    v = fmtarg(fav, fnn);
    if (unw != 0) return;
    pick = 0;
    if (colon) {
        pick = (v == NIL) ? 1 : 2;
    } else if (at) {
        if (v == NIL) pick = 0;
        else {
            pick = 1;
            fidx--;
        }
    } else {
        if (v < 0) pick = intval(v) + 1;
        else pick = 0;
        if (pick < 1 || pick > nc) pick = dflt;
    }
    if (pick >= 1 && pick <= nc)
        fmtrun(out, cp[pick - 1], cl[pick - 1], fav, fnn);
}

static void fmtparen(Sb *out, const char *ctl, long len, long *pi,
                     int colon, int at, Ref *fav, long fnn)
{
    Semis sm;
    long e, i = *pi;
    Sb s;
    e = fmtfind(ctl, len, i, &sm);
    if (e < 0) {
        lerror("unterminated ~( in FORMAT");
        *pi = len;
        return;
    }
    *pi = e + 2;
    sb_init(&s);
    fmtrun(&s, ctl + i, e - i, fav, fnn);
    sb_str(&s);
    if (colon && at) {
        upstr(s.p, s.n);
        sb_addn(out, s.p, s.n);
    } else if (at) {
        lowstr(s.p, s.n);
        if (s.n > 0) upstr(s.p, 1);
        sb_addn(out, s.p, s.n);
    } else if (colon) {
        capitalize(out, s.p, s.n);
    } else {
        lowstr(s.p, s.n);
        sb_addn(out, s.p, s.n);
    }
    free(s.p);
}

/* one directive; i points after the '~' */
static int fmtdirective(Sb *out, const char *ctl, long len, long *pi,
                        long *pm, Ref *fav, long fnn)
{
    long i = *pi, j, pc = 0, cnt, w = 0, dd, k;
    int colon = 0, at = 0, d, dch, base, padc;
    Ref v;
    Sb t, s;
    char *z;
    for (;;) {
        if (i >= len) break;
        d = (unsigned char)ctl[i];
        if (strchr("0123456789-+", d)) {
            j = i;
            i++;
            while (i < len && ctl[i] >= '0' && ctl[i] <= '9') i++;
            pc++;
            if (pc < MAXPM) pm[pc] = strtol(ctl + j, NULL, 10);
        } else if (d == '\'') {
            pc++;
            if (pc < MAXPM && i + 1 < len)
                pm[pc] = (unsigned char)ctl[i + 1];
            i += 2;
        } else if (d == 'v' || d == 'V') {
            pc++;
            v = fmtarg(fav, fnn);
            if (pc < MAXPM) pm[pc] = toint(v);
            i++;
        } else if (d == '#') {
            pc++;
            if (pc < MAXPM) pm[pc] = fnn - fidx + 1;
            i++;
        } else if (d == ',') {
            if (pc == 0) pc = 1;
            i++;
            if (i < len && ctl[i] == ',') {
                pc++;
                if (pc < MAXPM) pm[pc] = SKIP;
            }
        } else break;
    }
    if (pc >= MAXPM) pc = MAXPM - 1;
    while (i < len && (ctl[i] == ':' || ctl[i] == '@')) {
        if (ctl[i] == ':') colon = 1;
        else at = 1;
        i++;
    }
    dch = 0;
    if (i < len) dch = (unsigned char)ctl[i];
    if (dch >= 'a' && dch <= 'z') dch -= 32;
    i++;
    *pi = i;
    if (unw != 0) return 0;
    if (dch == 'A' || dch == 'S') {
        v = fmtarg(fav, fnn);
        if (unw != 0) return 0;
        z = printobj(v, dch == 'S');
        w = (long)strlen(z);
        if (pc >= 1) fmtpad(out, z, w, pm[1], ' ', at);
        else sb_addn(out, z, w);
        free(z);
    } else if (dch == 'D' || dch == 'B' || dch == 'O' || dch == 'X') {
        v = fmtarg(fav, fnn);
        if (unw != 0) return 0;
        sb_init(&s);
        if (v < 0) {
            base = 10;
            if (dch == 'B') base = 2;
            if (dch == 'O') base = 8;
            if (dch == 'X') base = 16;
            radixstr(&s, intval(v), base);
        } else {
            z = printobj(v, 0);
            sb_adds(&s, z);
            free(z);
        }
        sb_str(&s);
        sb_init(&t);
        if (colon && dch == 'D' && v < 0) commas(&t, s.p, s.n);
        else sb_addn(&t, s.p, s.n);
        sb_str(&t);
        if (at && v < 0 && intval(v) >= 0) {
            Sb u;
            sb_init(&u);
            sb_addc(&u, '+');
            sb_addn(&u, t.p, t.n);
            free(t.p);
            t = u;
            sb_str(&t);
        }
        padc = ' ';
        if (pc >= 2 && pm[2] != SKIP) padc = (int)pm[2];
        if (pc >= 1) fmtpad(out, t.p, t.n, pm[1], padc, 1);
        else sb_addn(out, t.p, t.n);
        free(s.p);
        free(t.p);
    } else if (dch == 'F' || dch == '$') {
        v = fmtarg(fav, fnn);
        if (unw != 0) return 0;
        sb_init(&s);
        if (!ISNUM(v)) {
            z = printobj(v, 0);
            sb_adds(&s, z);
            free(z);
        } else {
            dd = -1;
            w = 0;
            if (dch == '$') {
                dd = 2;
                if (pc >= 1) dd = pm[1];
            } else {
                if (pc >= 2) dd = pm[2];
                if (pc >= 1) w = pm[1];
            }
            if (dd >= 0) fixedfmt(&s, fval(v), dd);
            else floattext(&s, fval(v));
        }
        sb_str(&s);
        if (dch == 'F' && pc >= 1 && ISNUM(v))
            fmtpad(out, s.p, s.n, w, ' ', 1);
        else sb_addn(out, s.p, s.n);
        free(s.p);
    } else if (dch == 'C') {
        v = fmtarg(fav, fnn);
        if (unw != 0) return 0;
        pr_obj(out, v, 0);
    } else if (dch == '%') {
        cnt = 1;
        if (pc >= 1) cnt = pm[1];
        for (k = 0; k < cnt; k++) sb_addc(out, '\n');
    } else if (dch == '&') {
        if (out->n > 0) {
            if (out->p[out->n - 1] != '\n') sb_addc(out, '\n');
        } else if (capdepth == 0 && outcol != 0) sb_addc(out, '\n');
    } else if (dch == '~') {
        cnt = 1;
        if (pc >= 1) cnt = pm[1];
        for (k = 0; k < cnt; k++) sb_addc(out, '~');
    } else if (dch == '\n') {
        while (i < len && (ctl[i] == ' ' || ctl[i] == '\t')) i++;
        *pi = i;
    } else if (dch == '*') {
        if (colon) fidx--;
        else fidx++;
    } else if (dch == '^') {
        if (fidx > fnn) {
            fstop = 1;
            return 1;
        }
    } else if (dch == '{') {
        *pi = i;
        fmtiter(out, ctl, len, pi, at, fav, fnn);
    } else if (dch == '[') {
        *pi = i;
        fmtcond(out, ctl, len, pi, colon, at, fav, fnn);
    } else if (dch == '(') {
        *pi = i;
        fmtparen(out, ctl, len, pi, colon, at, fav, fnn);
    } else {
        char m[40];
        sprintf(m, "unknown FORMAT directive ~%c", dch);
        if (dch == 0) strcpy(m, "unknown FORMAT directive ~");
        lerror(m);
    }
    return 0;
}

static void fmtrun(Sb *res, const char *ctl, long len, Ref *fav,
                   long fnn)
{
    Sb out;
    long i = 0, j, pm[MAXPM + 2];
    int k;
    for (k = 0; k < MAXPM + 2; k++) pm[k] = 0;
    sb_init(&out);
    while (i < len) {
        if (ctl[i] != '~') {
            j = i;
            while (j < len && ctl[j] != '~') j++;
            sb_addn(&out, ctl + i, j - i);
            i = j;
        } else {
            i++;
            if (fmtdirective(&out, ctl, len, &i, pm, fav, fnn))
                break;
            if (unw != 0) break;
        }
    }
    sb_addn(res, out.p, out.n);
    free(out.p);
}

/* FORMAT with the control string in av[from] (1-based) and the
   arguments after it; the text is appended to o */
void formatargs(Sb *o, Ref *av, long from, long n)
{
    Ref c = av[from - 1];
    if (!ISSTR(c)) {
        lerror("FORMAT control is not a string");
        return;
    }
    fidx = 1;
    fstop = 0;
    fmtrun(o, XS(c), SLEN(c), av + from, n - from);
}
