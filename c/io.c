/* io.c -- output, input, error reports, loading files. */
#include "lisp.h"

void emit(const char *s, long n)
{
    if (n <= 0) return;
    if (capdepth > 0) {
        sb_addn(&capbuf, s, n);
        return;
    }
    fwrite(s, 1, (size_t)n, stdout);
    if (s[n - 1] == '\n') outcol = 0;
    else outcol = 1;
}

void emits(const char *s)
{
    emit(s, (long)strlen(s));
}

void freshline(void)
{
    if (capdepth > 0) {
        if (capbuf.n > 0 && capbuf.p[capbuf.n - 1] != '\n')
            sb_addc(&capbuf, '\n');
        return;
    }
    if (outcol != 0) emit("\n", 1);
}

void reporterr(void)
{
    char *s;
    freshline();
    if (unw == 1) {
        emits("Error: ");
        emits(errmsg);
        emits("\n");
    } else if (unw == 2) {
        s = printobj(unwtag, 1);
        emits("Error: attempt to throw to a tag that does not ");
        emits("exist: ");
        emits(s);
        emits("\n");
        free(s);
    } else if (unw == 3) {
        s = printobj(unwtag, 1);
        emits("Error: return from unknown block ");
        emits(s);
        emits("\n");
        free(s);
    }
    unw = 0;
    capdepth = 0;
    sb_clear(&capbuf);
    ndepth = 0;
}

/* ----------------------------------------------------------- input */
int lines(void)
{
    int c = getc(stdin);
    if (c == EOF) return 0;
    ungetc(c, stdin);
    return 1;
}

char *linein(void)
{
    Sb b;
    int c;
    sb_init(&b);
    for (;;) {
        c = getc(stdin);
        if (c == EOF || c == '\n') break;
        sb_addc(&b, c);
    }
    return sb_str(&b);
}

/* --------------------------------------------------------- loading */
int runsource(const char *text, long n)
{
    char *osrc = src;
    long orp = rp, orl = rlen;
    int rc = 0;
    Ref x;
    src = (char *)text;
    rp = 0;
    rlen = n;
    for (;;) {
        x = readobj();
        if (unw != 0) {
            reporterr();
            rc = 1;
            break;
        }
        if (x == 0) break;
        eval(x, NIL);
        if (unw != 0) {
            reporterr();
            rc = 1;
            break;
        }
    }
    src = osrc;
    rp = orp;
    rlen = orl;
    return rc;
}

int loadfile(const char *fname)
{
    FILE *f = fopen(fname, "rb");
    Sb b;
    char buf[4096];
    size_t k;
    int rc;
    char *s;
    long n;
    if (f == NULL) {
        emits("Error: cannot open file ");
        emits(fname);
        emits("\n");
        return 1;
    }
    sb_init(&b);
    while ((k = fread(buf, 1, sizeof buf, f)) > 0)
        sb_addn(&b, buf, (long)k);
    fclose(f);
    if (b.n > 0 && b.p[b.n - 1] != '\n') sb_addc(&b, '\n');
    s = sb_str(&b);
    n = b.n;
    rc = runsource(s, n);
    return rc;
}

static int isblank_s(const char *s)
{
    for (; *s; s++)
        if (*s != ' ' && *s != '\n' && *s != '\t' && *s != '\r')
            return 0;
    return 1;
}

void repl(void)
{
    char *buf = xstrdup(""), *ln, *nb, *rest;
    long save;
    Ref x, r;
    char *s;
    emits("* ");
    outcol = 0;
    while (lines()) {
        ln = linein();
        nb = (char *)xmalloc((long)strlen(buf) + (long)strlen(ln) + 2);
        strcpy(nb, buf);
        strcat(nb, ln);
        strcat(nb, "\n");
        free(buf);
        free(ln);
        buf = nb;
        src = buf;
        rp = 0;
        rlen = (long)strlen(buf);
        for (;;) {
            eofflag = 0;
            save = rp;
            x = readobj();
            if (unw != 0) {
                if (eofflag) {
                    unw = 0;
                    rp = save;
                    break;
                }
                reporterr();
                rp = rlen;
                break;
            }
            if (x == 0) {
                rp = rlen;
                break;
            }
            mvlist = 0;
            r = eval(x, NIL);
            if (unw != 0) reporterr();
            else {
                freshline();
                s = printobj(r, 1);
                emits(s);
                emits("\n");
                free(s);
            }
        }
        rest = xstrdup(buf + rp);
        free(buf);
        buf = rest;
        if (isblank_s(buf)) buf[0] = 0;
        if (buf[0] == 0) {
            emits("* ");
            outcol = 0;
        }
    }
    freshline();
}
