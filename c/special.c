/* special.c -- the special forms that eval1 does not handle inline,
   places (setf), backquote, destructuring, defstruct. */
#include "lisp.h"

#define CAR(r) ((r) > 0 ? ccar[r] : NIL)
#define CDR(r) ((r) > 0 ? ccdr[r] : NIL)
#define FIRSTARG(x)  car(CDR(x))
#define SECONDARG(x) car(CDR(CDR(x)))

Ref undyn(Ref d);

/* does a tree mention the symbol RETURN-FROM or RETURN ? */
static int hasreturn(Ref x)
{
    Ref a;
    while (ISCONS(x)) {
        a = ccar[x];
        if (a == sRETFROM || a == sRETURN) return 1;
        if (ISCONS(a) && hasreturn(a)) return 1;
        x = ccdr[x];
    }
    return 0;
}

Ref blockwrap(Ref name, Ref body)
{
    if (hasreturn(body)) return list1(cons(sBLOCK, cons(name, body)));
    return body;
}

Ref stripdoc(Ref body)
{
    if (ISCONS(body)) {
        if (ISSTR(ccar[body]) && ccdr[body] != NIL) return ccdr[body];
    }
    return body;
}

static Ref stripdoc2(Ref l)
{
    if (ISCONS(l) && ISSTR(ccar[l])) return ccdr[l];
    return l;
}

Ref simpleloop(Ref body, Ref env)
{
    Ref p;
    for (;;) {
        p = body;
        while (ISCONS(p)) {
            eval(ccar[p], env);
            if (unw != 0) {
                if (unw == 3 && unwtag == NIL) {
                    unw = 0;
                    return unwval;
                }
                return NIL;
            }
            p = ccdr[p];
        }
    }
}

/* bind the pattern of destructuring-bind */
Ref destruct(Ref pat, Ref val, Ref env)
{
    int mode = 0;
    Ref p, t;
    while (ISCONS(pat)) {
        p = ccar[pat];
        if (p == sOPT) mode = 1;
        else if (p == sREST || p == sBODY) mode = 2;
        else if (mode == 2) {
            env = cons(cons(p, val), env);
            val = NIL;
            mode = 3;
        } else if (ISCONS(p)) {
            env = destruct(p, car(val), env);
            val = cdr(val);
        } else {
            if (mode == 0 && val == NIL)
                return lerror("too few elements for destructuring");
            t = car(val);
            env = cons(cons(p, t), env);
            val = cdr(val);
        }
        if (unw != 0) return env;
        pat = ccdr[pat];
    }
    if (pat != NIL) env = cons(cons(pat, val), env);
    return env;
}

/* backquote */
Ref qq(Ref x, Ref env)
{
    Ref res = NIL, p, tail = NIL, e, l, nx, t;
    int dotted = 0;
    if (!ISCONS(x)) return x;
    if (ccar[x] == sUNQ) return eval(car(ccdr[x]), env);
    p = x;
    while (ISCONS(p)) {
        if (ccar[p] == sUNQ) {
            tail = eval(car(ccdr[p]), env);
            dotted = 1;
            p = NIL;
        } else {
            e = ccar[p];
            if (ISCONS(e) && ccar[e] == sUNQS) {
                l = eval(car(ccdr[e]), env);
                if (unw != 0) return NIL;
                while (ISCONS(l)) {
                    res = cons(ccar[l], res);
                    l = ccdr[l];
                }
            } else {
                t = qq(e, env);
                if (unw != 0) return NIL;
                res = cons(t, res);
            }
            p = ccdr[p];
        }
    }
    if (!dotted) tail = p;
    while (res != NIL) {
        nx = ccdr[res];
        ccdr[res] = tail;
        tail = res;
        res = nx;
    }
    return tail;
}

/* does the condition type name match the pending error ? */
int condmatch(Ref ty)
{
    const char *nm;
    if (ty == TRU) return 1;
    if (!ISSYM(ty)) return 0;
    nm = SYMNAME(ty);
    if (!strcmp(nm, "ERROR") || !strcmp(nm, "CONDITION") ||
        !strcmp(nm, "SERIOUS-CONDITION")) return 1;
    if (!strcmp(nm, errkind)) return 1;
    if (!strcmp(nm, "SIMPLE-ERROR") && !strcmp(errkind, "ERROR"))
        return 1;
    if (!strcmp(nm, "ARITHMETIC-ERROR") &&
        !strcmp(errkind, "DIVISION-BY-ZERO")) return 1;
    if (!strcmp(nm, "CELL-ERROR") &&
        (!strcmp(errkind, "UNBOUND-VARIABLE") ||
         !strcmp(errkind, "UNDEFINED-FUNCTION"))) return 1;
    return 0;
}

/* one step of macro expansion; 0 if the form is not a macro call */
Ref macroexp1(Ref x)
{
    Ref f, fn, env, r;
    Ref *av;
    long n;
    if (!ISCONS(x)) return 0;
    f = ccar[x];
    if (!ISSYM(f)) return 0;
    fn = ccdr[f];
    if (fn <= 1) return 0;
    if (ctag[fn] != T_MACRO) return 0;
    av = list2arr(ccdr[x], &n);
    env = bindparams(ccar[fn], av, n, XL(fn));
    free(av);
    if (unw != 0) return NIL;
    r = evalbody(ccdr[fn], env);
    return r;
}

/* ------------------------------------------------------- places */
static Ref notcons(Ref o)
{
    return lerrobj("ERROR", "not a cons: ", o, "");
}

static Ref setindex(Ref pl, Ref o, Ref idx, Ref v)
{
    long i;
    char m[80];
    if (idx >= 0) return lerrobj("ERROR", "bad index ", idx, "");
    i = intval(idx);
    if (o == NIL || ISCONS(o)) {
        while (i > 0 && ISCONS(o)) {
            o = ccdr[o];
            i--;
        }
        if (!ISCONS(o)) return lerror("index too large");
        ccar[o] = v;
        return v;
    }
    if (o > 0 && ctag[o] == T_VEC) {
        if (i < 0 || i >= ccdr[o]) {
            sprintf(m, "index %ld out of bounds", i);
            return lerror(m);
        }
        vdata(o)[i] = v;
        return v;
    }
    if (o > 0 && ctag[o] == T_STR) {
        if (i < 0 || i >= SLEN(o)) {
            sprintf(m, "index %ld out of bounds", i);
            return lerror(m);
        }
        if (!ISCHAR(v)) return lerror("not a character");
        XS(o)[i] = (char)XL(v);
        return v;
    }
    return lerrobj("ERROR", "invalid place: ", pl, "");
}

/* the place access functions used by setf, incf, push ... */
Ref setplace(Ref pl, Ref v, Ref env)
{
    Ref h, a, o, k, t, idx, s, tmp;
    const char *nm;
    if (ISSYM(pl)) return setvar(pl, v, env);
    if (!ISCONS(pl)) return lerror("invalid place");
    h = ccar[pl];
    a = ccdr[pl];
    if (!ISSYM(h)) return lerror("invalid place");
    nm = SYMNAME(h);
    if (SYM(h).cslot > 0) {
        o = eval(car(a), env);
        if (unw != 0) return NIL;
        if (o <= 0 || ctag[o] != T_STRUCT)
            return lerror("not a structure");
        vdata(o)[SYM(h).cslot - 1] = v;
        return v;
    }
    if (!strcmp(nm, "GETHASH")) {
        k = eval(car(a), env);
        t = eval(car(cdr(a)), env);
        if (unw != 0) return NIL;
        if (!(t > 0 && ctag[t] == T_HASH))
            return lerrobj("TYPE-ERROR", "the value ", t,
                           " is not a hash table");
        htput(t, k, v);
        return v;
    }
    if (!strcmp(nm, "GET")) {
        s = eval(car(a), env);
        k = eval(car(cdr(a)), env);
        if (unw != 0) return NIL;
        if (!ISSYM(s))
            return lerrobj("TYPE-ERROR", "the value ", s,
                           " is not a symbol");
        putprop(s, k, v);
        return v;
    }
    if (!strcmp(nm, "SYMBOL-VALUE")) {
        s = eval(car(a), env);
        if (unw != 0) return NIL;
        if (!ISSYM(s))
            return lerrobj("TYPE-ERROR", "the value ", s,
                           " is not a symbol");
        ccar[s] = v;
        return v;
    }
    o = eval(car(a), env);
    if (unw != 0) return NIL;
    if (!strcmp(nm, "CAR") || !strcmp(nm, "FIRST")) {
        if (!ISCONS(o)) return notcons(o);
        ccar[o] = v;
        return v;
    }
    if (!strcmp(nm, "CDR") || !strcmp(nm, "REST")) {
        if (!ISCONS(o)) return notcons(o);
        ccdr[o] = v;
        return v;
    }
    if (!strcmp(nm, "CADR") || !strcmp(nm, "SECOND")) {
        o = cdr(o);
        if (!ISCONS(o)) return lerror("not a cons");
        ccar[o] = v;
        return v;
    }
    if (!strcmp(nm, "THIRD")) {
        o = cdr(cdr(o));
        if (!ISCONS(o)) return lerror("not a cons");
        ccar[o] = v;
        return v;
    }
    if (!strcmp(nm, "CDDR")) {
        o = cdr(o);
        if (!ISCONS(o)) return lerror("not a cons");
        ccdr[o] = v;
        return v;
    }
    idx = eval(car(cdr(a)), env);
    if (unw != 0) return NIL;
    if (!strcmp(nm, "NTH")) {
        tmp = idx;
        idx = o;
        o = tmp;
    }
    return setindex(pl, o, idx, v);
}

/* ----------------------------------------------------- defstruct */
Ref pidof(const char *name)
{
    return csid[ccdr[intern(name)]];
}

static Ref mkprimcell(const char *pname, const char *nm, Ref a, Ref d)
{
    Ref c = newcell(T_PRIM);
    csid[c] = (unsigned char)pidof(pname);
    XS(c) = xstrdup(nm);
    ccar[c] = a;
    ccdr[c] = d;
    return c;
}

Ref defstruct(Ref x, Ref env)
{
    Ref spec, name, sl, ty, slotl, dfl, sp, d, s, mk, p, asym, ac, t;
    char *nm, *an;
    long i;
    Sb b;
    (void)env;
    spec = car(ccdr[x]);
    name = spec;
    if (ISCONS(spec)) name = ccar[spec];
    if (!ISSYM(name)) return lerror("invalid structure name");
    nm = xstrndup(SYMNAME(name), SYM(name).len);
    sl = stripdoc2(CDR(CDR(x)));
    ty = newcell(T_STYPE);
    XS(ty) = nm;
    slotl = NIL;
    dfl = NIL;
    while (ISCONS(sl)) {
        sp = ccar[sl];
        d = NIL;
        s = sp;
        if (ISCONS(sp)) {
            s = ccar[sp];
            d = car(ccdr[sp]);
        }
        slotl = cons(s, slotl);
        dfl = cons(d, dfl);
        sl = ccdr[sl];
    }
    t = nreverse(slotl);
    ccdr[ty] = t;
    t = nreverse(dfl);
    ccar[ty] = t;
    sb_init(&b);
    sb_adds(&b, "MAKE-");
    sb_adds(&b, nm);
    mk = mkprimcell("%MKSTRUCT", sb_str(&b), ty, NIL);
    t = intern(b.p);
    ccdr[t] = mk;
    sb_clear(&b);
    sb_adds(&b, nm);
    sb_adds(&b, "-P");
    mk = mkprimcell("%STRUCTP", sb_str(&b), ty, NIL);
    t = intern(b.p);
    ccdr[t] = mk;
    sb_clear(&b);
    sb_adds(&b, "COPY-");
    sb_adds(&b, nm);
    mk = mkprimcell("%COPYSTRUCT", sb_str(&b), ty, NIL);
    t = intern(b.p);
    ccdr[t] = mk;
    p = ccdr[ty];
    i = 0;
    while (ISCONS(p)) {
        sb_clear(&b);
        sb_adds(&b, nm);
        sb_addc(&b, '-');
        if (ISSYM(ccar[p]))
            sb_addn(&b, SYMNAME(ccar[p]), SYM(ccar[p]).len);
        an = sb_str(&b);
        asym = intern(an);
        ac = mkprimcell("%STRUCTGET", an, mkint(i), ty);
        ccdr[asym] = ac;
        SYM(asym).cslot = i + 1;
        i++;
        p = ccdr[p];
    }
    free(b.p);
    return name;
}
