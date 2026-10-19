/* print.c -- the printer: numbers, strings, every object. */
#include "lisp.h"

/* a float as the cREXX runtime writes it: the VM scales the value
   to [1,10) by a floating division, prints 14 decimals and trims
   (see extract_double_decimal in the cREXX VM), so the last digit
   can differ from printf's correctly rounded %.15g */
static void num15(Sb *o, double x)
{
    char t[64], dg[64];
    double ax, c;
    long e, i, nd = 0, ip;
    if (x == 0.0) {
        sb_adds(o, "0");
        return;
    }
    ax = x < 0.0 ? 0.0 - x : x;
    e = (long)floor(log10(ax));
    c = ax / pow(10.0, (double)e);
    if (c >= 10.0) {
        c /= 10.0;
        e++;
    }
    if (c < 1.0) {
        c *= 10.0;
        e--;
    }
    sprintf(t, "%.14f", c);
    if (strncmp(t, "10.", 3) == 0) {
        t[2] = t[1];
        t[1] = '.';
        e++;
    } else if (strncmp(t, "0.", 2) == 0 && t[2] != 0) {
        t[0] = t[2];
        memmove(t + 2, t + 3, strlen(t + 3) + 1);
        e--;
    }
    for (i = 0; t[i] != 0; i++)
        if (t[i] != '.') dg[nd++] = t[i];
    while (nd > 1 && dg[nd - 1] == '0') nd--;
    if (x < 0.0) sb_addc(o, '-');
    if (e >= 0) {
        for (ip = 0; ip <= e; ip++)
            sb_addc(o, ip < nd ? dg[ip] : '0');
        if (nd > e + 1) {
            sb_addc(o, '.');
            for (ip = e + 1; ip < nd; ip++) sb_addc(o, dg[ip]);
        }
    } else {
        sb_adds(o, "0.");
        for (ip = -1; ip > e; ip--) sb_addc(o, '0');
        for (ip = 0; ip < nd; ip++) sb_addc(o, dg[ip]);
    }
}

/* text of a float: always shows a decimal point or an exponent */
void floattext(Sb *o, double x)
{
    double ax = x, m;
    long ex = 0;
    int neg = 0;
    Sb s;
    if (x < 0.0) {
        ax = 0.0 - x;
        neg = 1;
    }
    if (ax >= 10000000.0 || (ax < 0.001 && ax > 0.0)) {
        m = ax;
        while (m >= 10.0) {
            m = m / 10.0;
            ex++;
        }
        while (m < 1.0) {
            m = m * 10.0;
            ex--;
        }
        sb_init(&s);
        num15(&s, m);
        sb_str(&s);
        if (neg) sb_addc(o, '-');
        sb_adds(o, s.p);
        if (strchr(s.p, '.') == NULL) sb_adds(o, ".0");
        free(s.p);
        sb_addc(o, 'e');
        sb_addl(o, ex);
        return;
    }
    sb_init(&s);
    num15(&s, x == 0.0 ? 0.0 : x);     /* no negative zero */
    sb_str(&s);
    if (strchr(s.p, '.') == NULL && strchr(s.p, 'e') == NULL &&
        strchr(s.p, 'E') == NULL) sb_adds(&s, ".0");
    sb_str(&s);
    if (s.p[0] == '.') sb_addc(o, '0');
    else if (s.p[0] == '-' && s.p[1] == '.') {
        sb_addc(o, '-');
        sb_addc(o, '0');
        sb_adds(o, s.p + 1);
        free(s.p);
        return;
    }
    sb_adds(o, s.p);
    free(s.p);
}

void printstring(Sb *o, const char *s, long n)
{
    long i;
    sb_addc(o, '"');
    for (i = 0; i < n; i++) {
        if (s[i] == '"') sb_adds(o, "\\\"");
        else if (s[i] == '\\') sb_adds(o, "\\\\");
        else sb_addc(o, s[i]);
    }
    sb_addc(o, '"');
}

static void pr_char(Sb *o, long code, int esc)
{
    if (esc) {
        sb_adds(o, "#\\");
        if (code == 32) {
            sb_adds(o, "Space");
            return;
        }
        if (code == 10) {
            sb_adds(o, "Newline");
            return;
        }
        if (code == 9) {
            sb_adds(o, "Tab");
            return;
        }
        if (code == 13) {
            sb_adds(o, "Return");
            return;
        }
    }
    sb_addc(o, (int)code);
}

void pr_obj(Sb *o, Ref r, int esc)
{
    int t;
    long n, i;
    Ref fl;
    if (r < 0) {
        sb_addl(o, intval(r));
        return;
    }
    if (r == 0) {
        sb_adds(o, "#<none>");
        return;
    }
    t = ctag[r];
    switch (t) {
    case T_SYM:
        if (cspec[r] == 3 && esc) sb_adds(o, "#:");
        sb_addn(o, SYMNAME(r), SYM(r).len);
        return;
    case T_FLOAT:
        floattext(o, XF(r));
        return;
    case T_STR:
        if (esc) printstring(o, XS(r), SLEN(r));
        else sb_addn(o, XS(r), SLEN(r));
        return;
    case T_CHAR:
        pr_char(o, XL(r), esc);
        return;
    case T_CONS:
        if (ccar[r] == sQUOTE && ISCONS(ccdr[r]) &&
            ccdr[ccdr[r]] == NIL) {
            sb_addc(o, '\'');
            pr_obj(o, ccar[ccdr[r]], esc);
            return;
        }
        if (ccar[r] == sFUNCTION && ISCONS(ccdr[r]) &&
            ccdr[ccdr[r]] == NIL) {
            sb_adds(o, "#'");
            pr_obj(o, ccar[ccdr[r]], esc);
            return;
        }
        sb_addc(o, '(');
        n = 0;
        while (ISCONS(r)) {
            if (n > 0) sb_addc(o, ' ');
            pr_obj(o, ccar[r], esc);
            r = ccdr[r];
            n++;
            if (n > 100000) {
                sb_adds(o, " ...");
                r = NIL;
            }
        }
        if (r != NIL) {
            sb_adds(o, " . ");
            pr_obj(o, r, esc);
        }
        sb_addc(o, ')');
        return;
    case T_PRIM:
        sb_adds(o, "#<FUNCTION ");
        sb_adds(o, XS(r));
        sb_addc(o, '>');
        return;
    case T_CLOS:
        sb_adds(o, "#<FUNCTION (LAMBDA ");
        pr_obj(o, ccar[r], 1);
        sb_adds(o, ")>");
        return;
    case T_MACRO:
        sb_adds(o, "#<MACRO>");
        return;
    case T_VEC:
        sb_adds(o, "#(");
        n = ccdr[r];
        for (i = 0; i < n; i++) {
            if (i > 0) sb_addc(o, ' ');
            pr_obj(o, vdata(r)[i], esc);
        }
        sb_addc(o, ')');
        return;
    case T_HASH:
        sb_adds(o, "#<HASH-TABLE>");
        return;
    case T_STRUCT:
        sb_adds(o, "#S(");
        sb_adds(o, XS(ccar[r]));
        fl = ccdr[ccar[r]];
        i = 0;
        while (ISCONS(fl)) {
            sb_adds(o, " :");
            pr_obj(o, ccar[fl], 1);
            sb_addc(o, ' ');
            pr_obj(o, vdata(r)[i], esc);
            i++;
            fl = ccdr[fl];
        }
        sb_addc(o, ')');
        return;
    default:
        sb_adds(o, "#<OBJECT>");
        return;
    }
}

char *printobj(Ref r, int esc)
{
    Sb b;
    sb_init(&b);
    pr_obj(&b, r, esc);
    sb_str(&b);
    return b.p;
}
