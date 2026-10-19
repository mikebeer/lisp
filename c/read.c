/* read.c -- the reader.  rp is a 0-based position in src. */
#include "lisp.h"

char *src = NULL;
long rp = 0, rlen = 0;

/* the character at rp+off, or -1 beyond the end */
static int pk(long off)
{
    if (rp + off >= rlen || rp + off < 0) return -1;
    return (unsigned char)src[rp + off];
}

static int isdelim(int c)
{
    if (c < 0) return 1;
    return c == ' ' || c == '(' || c == ')' || c == '\'' ||
           c == '"' || c == ';' || c == '`' || c == ',' ||
           c == '\n' || c == '\t' || c == '\r';
}

void skipws(void)
{
    int c, d;
    for (;;) {
        if (rp >= rlen) return;
        c = pk(0);
        if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
            rp++;
        } else if (c == ';') {
            while (rp < rlen && src[rp] != '\n') rp++;
        } else if (c == '#' && pk(1) == '|') {
            d = 1;
            rp += 2;
            while (rp < rlen && d > 0) {
                if (pk(0) == '|' && pk(1) == '#') {
                    d--;
                    rp += 2;
                } else if (pk(0) == '#' && pk(1) == '|') {
                    d++;
                    rp += 2;
                } else rp++;
            }
        } else return;
    }
}

/* is the text a number in REXX syntax? */
static int rexxnum(const char *t, long n)
{
    long i = 0, nd = 0;
    if (n == 0) return 0;
    if (t[i] == '+' || t[i] == '-') i++;
    while (i < n && t[i] >= '0' && t[i] <= '9') {
        i++;
        nd++;
    }
    if (i < n && t[i] == '.') {
        i++;
        while (i < n && t[i] >= '0' && t[i] <= '9') {
            i++;
            nd++;
        }
    }
    if (nd == 0) return 0;
    if (i < n && (t[i] == 'e' || t[i] == 'E')) {
        i++;
        if (i < n && (t[i] == '+' || t[i] == '-')) i++;
        nd = 0;
        while (i < n && t[i] >= '0' && t[i] <= '9') {
            i++;
            nd++;
        }
        if (nd == 0) return 0;
    }
    return i == n;
}

static int isnumtext(const char *t, long n)
{
    if (n == 0) return 0;
    if (strchr("0123456789+-.", t[0]) == NULL) return 0;
    if (n == 1 && (t[0] == '+' || t[0] == '-' || t[0] == '.'))
        return 0;
    return rexxnum(t, n);
}

static Ref parseatom(const char *t, long n)
{
    char *u;
    Ref r;
    long i, k;
    if (isnumtext(t, n)) {
        k = 0;
        for (i = 0; i < n; i++)
            if (t[i] == '.' || t[i] == 'e' || t[i] == 'E') k = 1;
        if (!k && n < 18) return mkint(strtol(t, NULL, 10));
        return mkfloat(strtod(t, NULL));
    }
    u = xstrndup(t, n);
    upstr(u, n);
    r = intern(u);
    free(u);
    return r;
}

static long parsebase(const char *tk, long n, int base)
{
    long v = 0, i = 0, d;
    int neg = 0;
    const char *dg = "0123456789ABCDEF";
    char *q;
    int c;
    if (n > 0 && tk[0] == '-') {
        neg = 1;
        i = 1;
    }
    for (; i < n; i++) {
        c = (unsigned char)tk[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        q = (c == 0) ? NULL : strchr(dg, c);
        d = q ? (long)(q - dg) : -1;
        if (d < 0 || d >= base) {
            lerror("bad digit in number");
            return 0;
        }
        v = v * base + d;
    }
    if (neg) v = 0 - v;
    return v;
}

static Ref readstring(void)
{
    Sb s;
    int d;
    Ref r;
    sb_init(&s);
    rp++;
    while (rp < rlen && src[rp] != '"') {
        d = pk(0);
        if (d == '\\') {
            rp++;
            d = pk(0);
            if (d == 'n') d = '\n';
            else if (d == 't') d = '\t';
        }
        if (d >= 0) sb_addc(&s, d);
        rp++;
    }
    rp++;
    sb_str(&s);
    r = mkstrn(s.p, s.n);
    free(s.p);
    return r;
}

static Ref readhash(int d)
{
    Sb nm;
    int ch;
    Ref r, l;
    char *u;
    if (d == '\'') {
        rp += 2;
        r = readobj();
        return list2(sFUNCTION, r);
    }
    if (d == '\\') {
        rp += 2;
        ch = pk(0);
        rp++;
        sb_init(&nm);
        if (ch >= 0) sb_addc(&nm, ch);
        while (rp < rlen && !isdelim(pk(0))) {
            sb_addc(&nm, pk(0));
            rp++;
        }
        sb_str(&nm);
        if (nm.n == 1) r = mkchar((unsigned char)nm.p[0]);
        else {
            u = xstrndup(nm.p, nm.n);
            upstr(u, nm.n);
            if (strcmp(u, "SPACE") == 0) r = mkchar(32);
            else if (strcmp(u, "NEWLINE") == 0 ||
                     strcmp(u, "LINEFEED") == 0) r = mkchar(10);
            else if (strcmp(u, "TAB") == 0) r = mkchar(9);
            else if (strcmp(u, "RETURN") == 0) r = mkchar(13);
            else r = mkchar(nm.n ? (unsigned char)nm.p[0] : 0);
            free(u);
        }
        free(nm.p);
        return r;
    }
    if (d == '(') {
        long n, i;
        Ref v;
        rp += 2;
        l = readlist();
        n = listlen(l);
        v = makevec(n);
        for (i = 0; i < n; i++) {
            vdata(v)[i] = ccar[l];
            l = ccdr[l];
        }
        return v;
    }
    return 0;
}

/* read one object; returns 0 at end of input */
Ref readobj(void)
{
    int c, d, base;
    Ref x, r;
    Sb t;
    long st;
    skipws();
    if (rp >= rlen) return 0;
    c = pk(0);
    if (c == '(') {
        rp++;
        return readlist();
    }
    if (c == ')') {
        rp++;
        lerror("unmatched close parenthesis");
        return NIL;
    }
    if (c == '\'') {
        rp++;
        x = readobj();
        if (x == 0) {
            eofflag = 1;
            return lerror("end of input after quote");
        }
        return list2(sQUOTE, x);
    }
    if (c == '`') {
        rp++;
        x = readobj();
        return list2(sQUASI, x);
    }
    if (c == ',') {
        rp++;
        if (pk(0) == '@') {
            rp++;
            x = readobj();
            return list2(sUNQS, x);
        }
        x = readobj();
        return list2(sUNQ, x);
    }
    if (c == '"') return readstring();
    if (c == '#') {
        d = pk(1);
        if (d == '\'' || d == '\\' || d == '(') return readhash(d);
        if (d >= 0 && strchr("xXbBoO", d) != NULL) {
            rp += 2;
            st = rp;
            while (rp < rlen && !isdelim(pk(0))) rp++;
            base = 16;
            if (d == 'b' || d == 'B') base = 2;
            if (d == 'o' || d == 'O') base = 8;
            return mkint(parsebase(src + st, rp - st, base));
        }
    }
    if (c == '|') {
        rp++;
        st = rp;
        while (rp < rlen && src[rp] != '|') rp++;
        sb_init(&t);
        sb_addn(&t, src + st, rp - st);
        rp++;
        r = intern(sb_str(&t));
        free(t.p);
        return r;
    }
    st = rp;
    while (rp < rlen && !isdelim(pk(0))) rp++;
    return parseatom(src + st, rp - st);
}

Ref readlist(void)
{
    Ref head = NIL, tail = NIL, x, cell;
    int c;
    for (;;) {
        skipws();
        if (rp >= rlen) {
            eofflag = 1;
            return lerror("end of input inside a list");
        }
        c = pk(0);
        if (c == ')') {
            rp++;
            return head;
        }
        if (c == '.' && isdelim(pk(1)) && tail != NIL) {
            rp++;
            x = readobj();
            ccdr[tail] = x;
            skipws();
            if (pk(0) == ')') rp++;
            return head;
        }
        x = readobj();
        if (unw != 0) return NIL;
        cell = cons(x, NIL);
        if (head == NIL) head = cell;
        else ccdr[tail] = cell;
        tail = cell;
    }
}
