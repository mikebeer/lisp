/* prim_msc.c -- primitives 187-223: hash tables, errors,
   structures, input and output, system. */
#include "lisp.h"

#define A(i) av[(i) - 1]

static const char *nameof(Ref x)
{
    if (ISSYM(x)) return SYMNAME(x);
    if (x > 0 && ctag[x] == T_PRIM) return XS(x);
    return "";
}

static Ref needhash(Ref t)
{
    if (t > 0 && ctag[t] == T_HASH) return t;
    return lerrobj("TYPE-ERROR", "the value ", t,
                   " is not a hash table");
}

static Ref pf187(Ref *av, long n)
{
    int mode = 0;
    long j = kwidx(av, n, 1, "TEST");
    const char *nm;
    if (j > 0) {
        nm = nameof(A(j));
        if (!strcmp(nm, "EQUAL")) mode = 1;
        else if (!strcmp(nm, "EQUALP")) mode = 2;
    }
    return mkhash(mode);
}

static Ref pf188(Ref *av, long n)
{
    int found;
    Ref v;
    needhash(A(2));
    if (unw != 0) return NIL;
    v = htget(A(2), A(1), &found);
    if (!found) {
        mvlist = list2(NIL, NIL);
        if (n == 3) return A(3);
        return NIL;
    }
    mvlist = list2(v, TRU);
    return v;
}

static Ref pf189(Ref *av, long n)
{
    (void)n;
    needhash(A(2));
    if (unw != 0) return NIL;
    return TRUTH(htrem(A(2), A(1)));
}

static Ref pf190(Ref *av, long n)
{
    (void)n;
    needhash(A(1));
    if (unw != 0) return NIL;
    htclear(A(1));
    return A(1);
}

static Ref pf191(Ref *av, long n)
{
    (void)n;
    needhash(A(1));
    if (unw != 0) return NIL;
    return mkint(ccdr[A(1)]);
}

static Ref pf192(Ref *av, long n)
{
    Ref fn = fnof(A(1)), *ks, *vs;
    long k, i;
    (void)n;
    if (unw != 0) return NIL;
    needhash(A(2));
    if (unw != 0) return NIL;
    k = htkeys(A(2), &ks, &vs);
    for (i = 0; i < k; i++) {
        call2(fn, ks[i], vs[i]);
        if (unw != 0) break;
    }
    free(ks);
    free(vs);
    return NIL;
}

static Ref hashlist(Ref t, int keys)
{
    Ref *ks, *vs, r = NIL;
    long k, i;
    needhash(t);
    if (unw != 0) return NIL;
    k = htkeys(t, &ks, &vs);
    for (i = k - 1; i >= 0; i--) r = cons(keys ? ks[i] : vs[i], r);
    free(ks);
    free(vs);
    return r;
}

static Ref pf193(Ref *av, long n)
{
    (void)n;
    return hashlist(A(1), 1);
}

static Ref pf194(Ref *av, long n)
{
    (void)n;
    return hashlist(A(1), 0);
}

static Ref pf195(Ref *av, long n)
{
    Sb s;
    char *z;
    if (!ISSTR(A(1))) {
        z = printobj(A(1), 0);
        lerror(z);
        free(z);
        return NIL;
    }
    sb_init(&s);
    formatargs(&s, av, 1, n);
    lerror(sb_str(&s));
    free(s.p);
    return NIL;
}

static Ref pf196(Ref *av, long n)
{
    Sb s;
    freshline();
    sb_init(&s);
    sb_adds(&s, "WARNING: ");
    formatargs(&s, av, 1, n);
    sb_addc(&s, '\n');
    emit(s.p, s.n);
    free(s.p);
    return NIL;
}

/* %MKSTRUCT: the constructor of a structure */
static Ref pf197(Ref *av, long n)
{
    Ref ty = ccar[curfn], slots = ccdr[ty], dfl = ccar[ty], inst, v;
    long cnt = listlen(slots), i = 0, j;
    Ref *d;
    Sb b;
    d = (Ref *)xmalloc((cnt + 1) * (long)sizeof(Ref));
    for (j = 0; j < cnt; j++) d[j] = NIL;
    inst = newcell(T_STRUCT);
    ccar[inst] = ty;
    XP(inst) = d;
    ccdr[inst] = cnt;
    while (ISCONS(slots)) {
        sb_init(&b);
        if (ISSYM(ccar[slots]))
            sb_addn(&b, SYMNAME(ccar[slots]), SYM(ccar[slots]).len);
        j = kwidx(av, n, 1, sb_str(&b));
        free(b.p);
        if (j > 0) d[i] = A(j);
        else {
            v = eval(ccar[dfl], NIL);
            if (unw != 0) return NIL;
            d[i] = v;
        }
        i++;
        slots = ccdr[slots];
        dfl = ccdr[dfl];
    }
    return inst;
}

static Ref pf198(Ref *av, long n)
{
    Ref x = A(1);
    (void)n;
    if (x > 0 && ctag[x] == T_STRUCT && ccar[x] == ccar[curfn])
        return TRU;
    return NIL;
}

static Ref pf199(Ref *av, long n)
{
    Ref x = A(1);
    long i;
    char *s;
    Sb b;
    (void)n;
    if (x <= 0 || ctag[x] != T_STRUCT) {
        sb_init(&b);
        sb_adds(&b, "the value ");
        s = printobj(x, 1);
        sb_adds(&b, s);
        free(s);
        sb_adds(&b, " is not a ");
        if (ctag[ccdr[curfn]] == T_STR) sb_adds(&b, XS(ccdr[curfn]));
        else sb_adds(&b, "structure");
        lerrk("TYPE-ERROR", sb_str(&b));
        free(b.p);
        return NIL;
    }
    i = intval(ccar[curfn]);
    if (i >= ccdr[x]) return lerror("structure has no such slot");
    return vdata(x)[i];
}

static Ref pf200(Ref *av, long n)
{
    Ref x = A(1), inst;
    long i, cnt;
    Ref *d;
    (void)n;
    if (x <= 0 || ctag[x] != T_STRUCT)
        return lerrobj("TYPE-ERROR", "the value ", x,
                       " is not a structure");
    cnt = ccdr[x];
    d = (Ref *)xmalloc((cnt + 1) * (long)sizeof(Ref));
    for (i = 0; i < cnt; i++) d[i] = vdata(x)[i];
    inst = newcell(T_STRUCT);
    ccar[inst] = ccar[x];
    ccdr[inst] = cnt;
    XP(inst) = d;
    return inst;
}

static void emitobj(Ref x, int esc)
{
    char *z = printobj(x, esc);
    emits(z);
    free(z);
}

static Ref pf201(Ref *av, long n)
{
    (void)n;
    emit("\n", 1);
    emitobj(A(1), 1);
    emit(" ", 1);
    return A(1);
}

static Ref pf202(Ref *av, long n)
{
    (void)n;
    emitobj(A(1), 1);
    return A(1);
}

static Ref pf203(Ref *av, long n)
{
    (void)n;
    emitobj(A(1), 0);
    return A(1);
}

static Ref pf205(Ref *av, long n)
{
    (void)av;
    (void)n;
    emit("\n", 1);
    return NIL;
}

static Ref pf206(Ref *av, long n)
{
    (void)av;
    (void)n;
    freshline();
    return NIL;
}

static Ref pf207(Ref *av, long n)
{
    long len;
    char cb[2];
    const char *s = strof(A(1), &len, cb);
    (void)n;
    emit(s, len);       /* cREXX goes on after an error here */
    if (strcmp(XS(curfn), "WRITE-LINE") == 0) emit("\n", 1);
    return A(1);
}

static Ref pf208(Ref *av, long n)
{
    char c;
    (void)n;
    c = (char)chr(A(1));
    if (unw != 0) return NIL;
    emit(&c, 1);
    return A(1);
}

static Ref pf209(Ref *av, long n)
{
    Ref dest = A(1), r;
    Sb s;
    sb_init(&s);
    if (ISSTR(dest)) {
        formatargs(&s, av, 1, n);
        if (unw != 0) {
            free(s.p);
            return NIL;
        }
        emit(s.p, s.n);
        free(s.p);
        return NIL;
    }
    if (n < 2) return lerror("FORMAT control is not a string");
    formatargs(&s, av, 2, n);
    if (unw != 0) {
        free(s.p);
        return NIL;
    }
    if (dest == NIL) {
        sb_str(&s);
        r = mkstrn(s.p, s.n);
        free(s.p);
        return r;
    }
    emit(s.p, s.n);
    free(s.p);
    return NIL;
}

static Ref pf210(Ref *av, long n)
{
    char *z = printobj(A(1), 0);
    Ref r = mkstr(z);
    (void)n;
    free(z);
    return r;
}

static Ref pf211(Ref *av, long n)
{
    char *z = printobj(A(1), 1);
    Ref r = mkstr(z);
    (void)n;
    free(z);
    return r;
}

static Ref pf212(Ref *av, long n)
{
    char *z;
    Ref r;
    if (!lines()) {
        if (n >= 3 && A(2) == NIL) return A(3);
        if (n >= 2 && A(2) != NIL)
            return lerror("end of file on standard input");
        return NIL;
    }
    z = linein();
    r = mkstr(z);
    free(z);
    return r;
}

/* READ-FROM-STRING and READ */
static Ref pf213(Ref *av, long n)
{
    char *txt, *osrc = src, cb[2];
    long orp = rp, orl = rlen, len;
    const char *s;
    Ref x;
    (void)n;
    if (strcmp(XS(curfn), "READ") == 0) {
        if (!lines()) return NIL;
        txt = linein();
    } else {
        s = strof(A(1), &len, cb);
        if (unw != 0) return NIL;
        txt = xstrndup(s, len);
    }
    src = txt;
    rp = 0;
    rlen = (long)strlen(txt);
    x = readobj();
    src = osrc;
    rp = orp;
    rlen = orl;
    free(txt);
    if (x == 0) return NIL;
    return x;
}

static Ref pf214(Ref *av, long n)
{
    (void)n;
    return eval(A(1), NIL);
}

static Ref pf215(Ref *av, long n)
{
    long len;
    char cb[2], *f;
    const char *s;
    int rc;
    (void)n;
    s = strof(A(1), &len, cb);
    if (unw != 0) return NIL;
    f = xstrndup(s, len);
    rc = loadfile(f);
    free(f);
    if (rc != 0) return NIL;
    return TRU;
}

static Ref pf216(Ref *av, long n)
{
    Ref x = A(1), nx;
    (void)n;
    for (;;) {
        nx = macroexp1(x);
        if (unw != 0) return NIL;
        if (nx == 0) return x;
        x = nx;
        if (strcmp(XS(curfn), "MACROEXPAND-1") == 0) return x;
    }
}

static Ref pf217(Ref *av, long n)
{
    (void)av;
    (void)n;
    return NIL;
}

static long secs_midnight(void)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (tm == NULL) return 0;
    return (long)tm->tm_hour * 3600L + (long)tm->tm_min * 60L +
           (long)tm->tm_sec;
}

static Ref pf218(Ref *av, long n)
{
    (void)av;
    (void)n;
    return mkint(secs_midnight());
}

static Ref pf219(Ref *av, long n)
{
    static long base = -1;
    long ms;
    (void)av;
    (void)n;
    if (base < 0) base = secs_midnight() * 1000L;
    ms = (long)((double)clock() * 1000.0 / (double)CLOCKS_PER_SEC);
    return mkint(base + ms);
}

static Ref pf221(Ref *av, long n)
{
    int rc = 0;
    freshline();
    if (n == 1 && A(1) < 0) rc = (int)intval(A(1));
    fflush(stdout);
    exit(rc);
    return NIL;
}

static Ref pf222(Ref *av, long n)
{
    (void)n;
    if (!ISSYM(A(1)))
        return lerrobj("TYPE-ERROR", "the value ", A(1),
                       " is not a symbol");
    if (SYM(A(1)).cpl == 0) return NIL;
    return SYM(A(1)).cpl;
}

void initprim_e(void)
{
    primtab[187] = pf187; primtab[188] = pf188; primtab[189] = pf189;
    primtab[190] = pf190; primtab[191] = pf191; primtab[192] = pf192;
    primtab[193] = pf193; primtab[194] = pf194; primtab[195] = pf195;
    primtab[196] = pf196; primtab[197] = pf197; primtab[198] = pf198;
    primtab[199] = pf199; primtab[200] = pf200; primtab[201] = pf201;
    primtab[202] = pf202; primtab[203] = pf203; primtab[204] = pf202;
    primtab[205] = pf205; primtab[206] = pf206; primtab[207] = pf207;
    primtab[208] = pf208; primtab[209] = pf209; primtab[210] = pf210;
    primtab[211] = pf211; primtab[212] = pf212; primtab[213] = pf213;
    primtab[214] = pf214; primtab[215] = pf215; primtab[216] = pf216;
    primtab[217] = pf217; primtab[218] = pf218; primtab[219] = pf219;
    primtab[220] = pf217; primtab[221] = pf221; primtab[222] = pf222;
    primtab[223] = pf217;
}
