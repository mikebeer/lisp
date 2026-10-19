/* table.c -- hash tables, property lists, types. */
#include "lisp.h"

typedef struct HEnt {
    Ref k, v;
    long next;           /* next entry in the bucket chain */
    long onext, oprev;   /* insertion order list */
    unsigned long h;
} HEnt;

typedef struct HT {
    int mode;            /* 0 eql, 1 equal, 2 equalp */
    long nb;             /* number of buckets (a power of two) */
    long *bucket;        /* entry index or 0 */
    HEnt *ents;          /* 1-based */
    long nent, capent;   /* entries used (live and free) */
    long freel;          /* free entry list, linked by next */
    long first, last;
} HT;

#define HTOF(t) ((HT *)XP(t))

Ref mkhash(int mode)
{
    Ref c = newcell(T_HASH);
    HT *h = (HT *)xmalloc((long)sizeof(HT));
    long i;
    h->mode = mode;
    h->nb = 16;
    h->bucket = (long *)xmalloc(h->nb * (long)sizeof(long));
    for (i = 0; i < h->nb; i++) h->bucket[i] = 0;
    h->ents = NULL;
    h->nent = 0;
    h->capent = 0;
    h->freel = 0;
    h->first = 0;
    h->last = 0;
    XP(c) = h;
    ccdr[c] = 0;
    return c;
}

static unsigned long mix(unsigned long h, unsigned long v)
{
    return (h * 31UL + v) & 0x7fffffffUL;
}

static unsigned long strhash(const char *s, long n, int fold)
{
    unsigned long h = 7;
    long i;
    int c;
    for (i = 0; i < n; i++) {
        c = (unsigned char)s[i];
        if (fold && c >= 'a' && c <= 'z') c -= 32;
        h = mix(h, (unsigned long)c);
    }
    return h;
}

static unsigned long numhash(double f)
{
    if (f < 0.0) return (unsigned long)(long)(0.0 - floor(0.0 - f));
    return (unsigned long)(long)floor(f);
}

/* a hash that agrees with eql (mode 0), equal (1) and equalp (2) */
static unsigned long hkey(int mode, Ref k, int depth)
{
    unsigned long h;
    long i, n;
    int tg;
    if (k < 0) {
        if (mode == 2) return (unsigned long)intval(k);
        return (unsigned long)intval(k);
    }
    if (k == 0) return 0;
    tg = ctag[k];
    if (tg == T_FLOAT) {
        if (mode == 2) return numhash(XF(k));
        return numhash(XF(k));
    }
    if (tg == T_CHAR) {
        long c = XL(k);
        if (mode == 2 && c >= 'a' && c <= 'z') c -= 32;
        return (unsigned long)c;
    }
    if (mode >= 1 && tg == T_STR)
        return strhash(XS(k), SLEN(k), mode == 2);
    if (mode >= 1 && tg == T_CONS) {
        h = 17;
        n = 0;
        if (depth > 3) return h;
        while (ISCONS(k) && n < 8) {
            h = mix(h, hkey(mode, ccar[k], depth + 1));
            k = ccdr[k];
            n++;
        }
        return h;
    }
    if (mode >= 1 && tg == T_VEC) {
        h = 19;
        if (depth > 3) return h;
        n = ccdr[k];
        for (i = 0; i < n && i < 8; i++)
            h = mix(h, hkey(mode, vdata(k)[i], depth + 1));
        return h;
    }
    return (unsigned long)k;
}

static int hkeq(int mode, Ref a, Ref b)
{
    if (mode == 0) return eql(a, b);
    if (mode == 1) return equal(a, b);
    return equalp(a, b);
}

Ref htfind(Ref t, Ref k)
{
    HT *h = HTOF(t);
    unsigned long hv = hkey(h->mode, k, 0);
    long e = h->bucket[hv & (unsigned long)(h->nb - 1)];
    while (e != 0) {
        if (h->ents[e].h == hv && hkeq(h->mode, h->ents[e].k, k))
            return e;
        e = h->ents[e].next;
    }
    return 0;
}

Ref htget(Ref t, Ref k, int *found)
{
    long e = htfind(t, k);
    if (e == 0) {
        *found = 0;
        return NIL;
    }
    *found = 1;
    return HTOF(t)->ents[e].v;
}

static void rehash(HT *h)
{
    long nb = h->nb * 2, i, e;
    free(h->bucket);
    h->bucket = (long *)xmalloc(nb * (long)sizeof(long));
    for (i = 0; i < nb; i++) h->bucket[i] = 0;
    h->nb = nb;
    for (e = h->first; e != 0; e = h->ents[e].onext) {
        i = (long)(h->ents[e].h & (unsigned long)(nb - 1));
        h->ents[e].next = h->bucket[i];
        h->bucket[i] = e;
    }
}

void htput(Ref t, Ref k, Ref v)
{
    HT *h = HTOF(t);
    long e = htfind(t, k), b;
    unsigned long hv;
    if (e > 0) {
        h->ents[e].v = v;
        return;
    }
    if (ccdr[t] >= h->nb) rehash(h);
    hv = hkey(h->mode, k, 0);
    if (h->freel) {
        e = h->freel;
        h->freel = h->ents[e].next;
    } else {
        if (h->nent + 2 > h->capent) {
            h->capent = h->capent ? h->capent * 2 : 16;
            h->ents = (HEnt *)xrealloc(h->ents,
                          (h->capent + 1) * (long)sizeof(HEnt));
        }
        e = ++h->nent;
    }
    b = (long)(hv & (unsigned long)(h->nb - 1));
    h->ents[e].k = k;
    h->ents[e].v = v;
    h->ents[e].h = hv;
    h->ents[e].next = h->bucket[b];
    h->bucket[b] = e;
    h->ents[e].onext = 0;
    h->ents[e].oprev = h->last;
    if (h->last != 0) h->ents[h->last].onext = e;
    else h->first = e;
    h->last = e;
    ccdr[t] = ccdr[t] + 1;
}

int htrem(Ref t, Ref k)
{
    HT *h = HTOF(t);
    unsigned long hv = hkey(h->mode, k, 0);
    long b = (long)(hv & (unsigned long)(h->nb - 1));
    long e = h->bucket[b], prev = 0;
    while (e != 0) {
        if (h->ents[e].h == hv && hkeq(h->mode, h->ents[e].k, k)) {
            if (prev == 0) h->bucket[b] = h->ents[e].next;
            else h->ents[prev].next = h->ents[e].next;
            if (h->ents[e].oprev != 0)
                h->ents[h->ents[e].oprev].onext = h->ents[e].onext;
            else h->first = h->ents[e].onext;
            if (h->ents[e].onext != 0)
                h->ents[h->ents[e].onext].oprev = h->ents[e].oprev;
            else h->last = h->ents[e].oprev;
            h->ents[e].next = h->freel;
            h->freel = e;
            ccdr[t] = ccdr[t] - 1;
            return 1;
        }
        prev = e;
        e = h->ents[e].next;
    }
    return 0;
}

void htclear(Ref t)
{
    HT *h = HTOF(t);
    long i;
    for (i = 0; i < h->nb; i++) h->bucket[i] = 0;
    h->nent = 0;
    h->freel = 0;
    h->first = 0;
    h->last = 0;
    ccdr[t] = 0;
}

/* snapshot of the keys and values in insertion order */
long htkeys(Ref t, Ref **ks, Ref **vs)
{
    HT *h = HTOF(t);
    long n = ccdr[t], k = 0, e;
    *ks = (Ref *)xmalloc((n + 1) * (long)sizeof(Ref));
    *vs = (Ref *)xmalloc((n + 1) * (long)sizeof(Ref));
    for (e = h->first; e != 0; e = h->ents[e].onext) {
        (*ks)[k] = h->ents[e].k;
        (*vs)[k] = h->ents[e].v;
        k++;
    }
    return k;
}

/* ------------------------------------------------ property lists */
Ref getprop(Ref s, Ref k)
{
    Ref p = SYM(s).cpl;
    while (ISCONS(p)) {
        if (ccar[ccar[p]] == k) return ccdr[ccar[p]];
        p = ccdr[p];
    }
    return NIL;
}

void putprop(Ref s, Ref k, Ref v)
{
    Ref p = SYM(s).cpl, c;
    while (ISCONS(p)) {
        if (ccar[ccar[p]] == k) {
            ccdr[ccar[p]] = v;
            return;
        }
        p = ccdr[p];
    }
    if (SYM(s).cpl == 0) SYM(s).cpl = NIL;
    c = cons(k, v);
    c = cons(c, SYM(s).cpl);
    SYM(s).cpl = c;
}

/* ----------------------------------------------------------- types */
static int symis(Ref s, const char *n)
{
    return ISSYM(s) && strcmp(SYMNAME(s), n) == 0;
}

int typep(Ref o, Ref ty)
{
    Ref p, lo, hi;
    const char *nm;
    if (ty == TRU) return 1;
    if (ty == NIL) return 0;
    if (ISCONS(ty)) {
        p = ccar[ty];
        if (!ISSYM(p)) return 0;
        if (symis(p, "OR")) {
            for (p = ccdr[ty]; ISCONS(p); p = ccdr[p])
                if (typep(o, ccar[p])) return 1;
            return 0;
        }
        if (symis(p, "AND")) {
            for (p = ccdr[ty]; ISCONS(p); p = ccdr[p])
                if (!typep(o, ccar[p])) return 0;
            return 1;
        }
        if (symis(p, "NOT")) return 1 - typep(o, car(ccdr[ty]));
        if (symis(p, "MEMBER")) {
            for (p = ccdr[ty]; ISCONS(p); p = ccdr[p])
                if (eql(o, ccar[p])) return 1;
            return 0;
        }
        if (symis(p, "INTEGER")) {
            if (o >= 0) return 0;
            lo = car(ccdr[ty]);
            hi = car(cdr(ccdr[ty]));
            if (lo < 0 && intval(o) < intval(lo)) return 0;
            if (hi < 0 && intval(o) > intval(hi)) return 0;
            return 1;
        }
        return 0;
    }
    if (!ISSYM(ty)) return 0;
    nm = SYMNAME(ty);
    if (!strcmp(nm, "INTEGER") || !strcmp(nm, "FIXNUM") ||
        !strcmp(nm, "RATIONAL") || !strcmp(nm, "BIGNUM"))
        return o < 0;
    if (!strcmp(nm, "NUMBER") || !strcmp(nm, "REAL"))
        return ISNUM(o);
    if (!strcmp(nm, "FLOAT") || !strcmp(nm, "SINGLE-FLOAT") ||
        !strcmp(nm, "DOUBLE-FLOAT")) return ISFLOAT(o);
    if (!strcmp(nm, "STRING") || !strcmp(nm, "SIMPLE-STRING"))
        return ISSTR(o);
    if (!strcmp(nm, "SYMBOL")) return ISSYM(o);
    if (!strcmp(nm, "KEYWORD"))
        return ISSYM(o) && SYMNAME(o)[0] == ':';
    if (!strcmp(nm, "LIST")) return o == NIL || ISCONS(o);
    if (!strcmp(nm, "CONS")) return ISCONS(o);
    if (!strcmp(nm, "NULL")) return o == NIL;
    if (!strcmp(nm, "ATOM")) return !ISCONS(o);
    if (!strcmp(nm, "CHARACTER")) return ISCHAR(o);
    if (!strcmp(nm, "BOOLEAN")) return o == NIL || o == TRU;
    if (!strcmp(nm, "FUNCTION") || !strcmp(nm, "COMPILED-FUNCTION"))
        return o > 0 && (ctag[o] == T_PRIM || ctag[o] == T_CLOS);
    if (!strcmp(nm, "VECTOR") || !strcmp(nm, "ARRAY") ||
        !strcmp(nm, "SIMPLE-VECTOR")) {
        if (o > 0) {
            if (ctag[o] == T_VEC) return 1;
            if (strcmp(nm, "SIMPLE-VECTOR") && ctag[o] == T_STR)
                return 1;
        }
        return 0;
    }
    if (!strcmp(nm, "SEQUENCE")) {
        if (o == NIL || ISCONS(o)) return 1;
        return o > 0 && (ctag[o] == T_VEC || ctag[o] == T_STR);
    }
    if (!strcmp(nm, "HASH-TABLE")) return o > 0 && ctag[o] == T_HASH;
    if (o > 0 && ctag[o] == T_STRUCT)
        return strcmp(XS(ccar[o]), nm) == 0;
    return 0;
}

Ref lobjtype(Ref o)
{
    int t;
    if (o < 0) return intern("INTEGER");
    if (o == NIL) return intern("NULL");
    if (o == TRU) return intern("BOOLEAN");
    t = ctag[o];
    switch (t) {
    case T_CONS: return intern("CONS");
    case T_SYM:
        if (SYMNAME(o)[0] == ':') return intern("KEYWORD");
        return intern("SYMBOL");
    case T_FLOAT: return intern("SINGLE-FLOAT");
    case T_STR: return intern("STRING");
    case T_PRIM:
    case T_CLOS: return intern("FUNCTION");
    case T_CHAR: return intern("CHARACTER");
    case T_VEC: return intern("VECTOR");
    case T_HASH: return intern("HASH-TABLE");
    case T_STRUCT: return intern(XS(ccar[o]));
    default: return intern("T");
    }
}
