/* eval.c -- variables, parameter binding, calls, the evaluator. */
#include "lisp.h"

/* safe car and cdr of a ref that may not be a cell */
#define CAR(r) ((r) > 0 ? ccar[r] : NIL)
#define CDR(r) ((r) > 0 ? ccdr[r] : NIL)

/* ----------------------------------------------------- variables */
Ref symval(Ref s, Ref env)
{
    Ref e, b, v;
    if (cspec[s] == 2) return ccar[s];
    for (e = env; e != NIL; e = ccdr[e]) {
        b = ccar[e];
        if (ccar[b] == s) return ccdr[b];
    }
    v = ccar[s];
    if (v == 0) {
        lerrobj("UNBOUND-VARIABLE", "the variable ", s,
                " is unbound");
        return NIL;
    }
    return v;
}

Ref setvar(Ref s, Ref v, Ref env)
{
    Ref e, b;
    if (!ISSYM(s)) return lerror("invalid variable");
    if (cspec[s] == 2) {
        lerrobj("ERROR", "cannot assign to the constant ", s, "");
        return NIL;
    }
    for (e = env; e != NIL; e = ccdr[e]) {
        b = ccar[e];
        if (ccar[b] == s) {
            ccdr[b] = v;
            return v;
        }
    }
    ccar[s] = v;
    return v;
}

/* the key symbol of a local function name */
static Ref fkey(Ref f)
{
    Sb b;
    Ref k;
    if (SYM(f).cfk == 0) {
        sb_init(&b);
        sb_adds(&b, "#F:");
        sb_addn(&b, SYMNAME(f), SYM(f).len);
        k = intern(sb_str(&b));
        free(b.p);
        SYM(f).cfk = k;
    }
    return SYM(f).cfk;
}

Ref getfn(Ref f, Ref env)
{
    Ref k, e, b, fn;
    if (!ISSYM(f)) return lerror("illegal function call");
    if (hasfenv) {
        k = fkey(f);
        for (e = env; e != NIL; e = ccdr[e]) {
            b = ccar[e];
            if (ccar[b] == k) return ccdr[b];
        }
    }
    fn = ccdr[f];
    if (fn == 0 || fn == NIL) {
        lerrobj("UNDEFINED-FUNCTION", "undefined function ", f, "");
        return NIL;
    }
    return fn;
}

/* resolve a function designator */
Ref fnof(Ref x)
{
    Ref fn;
    if (x <= 0) {
        lerrobj("TYPE-ERROR", "the value ", x, " is not a function");
        return NIL;
    }
    if (ctag[x] == T_SYM) {
        fn = ccdr[x];
        if (fn == 0 || fn == NIL) {
            lerrobj("UNDEFINED-FUNCTION", "undefined function ", x,
                    "");
            return NIL;
        }
        return fn;
    }
    if (ctag[x] == T_PRIM || ctag[x] == T_CLOS) return x;
    lerrobj("TYPE-ERROR", "the value ", x, " is not a function");
    return NIL;
}

Ref mkclosure(Ref params, Ref body, Ref env)
{
    Ref c = newcell(T_CLOS);
    ccar[c] = params;
    ccdr[c] = body;
    XL(c) = env;
    nclos++;
    return c;
}

/* ----------------------------------------------- parameter binding */
Ref bindparams(Ref params, Ref *av, long n, Ref cenv)
{
    Ref env = cenv, s, p = params, dflt, sv, v, kw;
    long i = 1, kstart = 0, j, found;
    int mode = 0;
    while (ISCONS(p)) {
        s = ccar[p];
        p = ccdr[p];
        if (s == sOPT) mode = 1;
        else if (s == sREST || s == sBODY) mode = 2;
        else if (s == sKEY) {
            mode = 3;
            kstart = i;
        } else if (s == sAUX) mode = 4;
        else if (mode == 0) {
            if (i > n) return lerror("too few arguments");
            env = cons(cons(s, av[i - 1]), env);
            i++;
        } else if (mode == 1) {
            dflt = NIL;
            sv = 0;
            if (ISCONS(s)) {
                dflt = car(cdr(s));
                sv = car(cdr(cdr(s)));
                s = ccar[s];
            }
            if (i <= n) {
                env = cons(cons(s, av[i - 1]), env);
                if (sv > 1) env = cons(cons(sv, TRU), env);
                i++;
            } else {
                v = NIL;
                if (dflt != NIL) v = eval(dflt, env);
                if (unw != 0) return NIL;
                env = cons(cons(s, v), env);
                if (sv > 1) env = cons(cons(sv, NIL), env);
            }
        } else if (mode == 2) {
            if (i <= n) env = cons(cons(s, arr2list(av, i, n)), env);
            else env = cons(cons(s, NIL), env);
            i = n + 1;
        } else if (mode == 3) {
            dflt = NIL;
            if (ISCONS(s)) {
                dflt = car(cdr(s));
                s = ccar[s];
            }
            if (!ISSYM(s)) return lerror("invalid parameter");
            {
                Sb b;
                sb_init(&b);
                sb_addc(&b, ':');
                sb_addn(&b, SYMNAME(s), SYM(s).len);
                kw = intern(sb_str(&b));
                free(b.p);
            }
            found = 0;
            j = kstart;
            while (j < n && found == 0) {
                if (av[j - 1] == kw) found = j + 1;
                j += 2;
            }
            if (found > 0) env = cons(cons(s, av[found - 1]), env);
            else {
                v = NIL;
                if (dflt != NIL) v = eval(dflt, env);
                if (unw != 0) return NIL;
                env = cons(cons(s, v), env);
            }
            i = n + 1;
        } else {
            dflt = NIL;
            if (ISCONS(s)) {
                dflt = car(cdr(s));
                s = ccar[s];
            }
            v = NIL;
            if (dflt != NIL) v = eval(dflt, env);
            if (unw != 0) return NIL;
            env = cons(cons(s, v), env);
        }
    }
    if (i <= n && mode < 2) return lerror("too many arguments");
    return env;
}

/* evaluate all but the last form of a body; return the last form,
   0 if none */
Ref evalprefix(Ref body, Ref env)
{
    if (body == NIL || !ISCONS(body)) return 0;
    while (ISCONS(ccdr[body])) {
        eval(ccar[body], env);
        if (unw != 0) return 0;
        body = ccdr[body];
    }
    return ccar[body];
}

Ref evalbody(Ref body, Ref env)
{
    Ref x = evalprefix(body, env);
    if (x == 0) return NIL;
    return eval(x, env);
}

/* ---------------------------------------------------- calling */
Ref applyfn(Ref fn, Ref *av, long n)
{
    Ref env, res;
    long nc;
    int id;
    char *m;
    Sb b;
    if (fn <= 0) {
        lerrobj("ERROR", "the value ", fn, " is not a function");
        return NIL;
    }
    switch (ctag[fn]) {
    case T_PRIM:
        curfn = fn;
        id = csid[fn];
        if (n < pmin[id] || (pmax[id] >= 0 && n > pmax[id])) {
            sb_init(&b);
            sb_adds(&b, n < pmin[id] ? "too few" : "too many");
            sb_adds(&b, " arguments to ");
            sb_adds(&b, XS(fn));
            m = sb_str(&b);
            lerror(m);
            free(b.p);
            return NIL;
        }
        return callprim(id, av, n);
    case T_CLOS:
        nc = nclos;
        env = bindparams(ccar[fn], av, n, XL(fn));
        if (unw != 0) return NIL;
        res = evalbody(ccdr[fn], env);
        if (nclos == nc) freeframe(env, XL(fn));
        return res;
    case T_SYM:
        fn = fnof(fn);
        if (unw != 0) return NIL;
        return applyfn(fn, av, n);
    default:
        lerrobj("ERROR", "the value ", fn, " is not a function");
        return NIL;
    }
}

Ref call1(Ref fn, Ref a)
{
    Ref v[1];
    v[0] = a;
    return applyfn(fn, v, 1);
}

Ref call2(Ref fn, Ref a, Ref b)
{
    Ref v[2];
    v[0] = a;
    v[1] = b;
    return applyfn(fn, v, 2);
}

/* inline versions of the commonest primitives; 0 when not
   applicable */
Ref fastprim(int id, Ref *av, long n)
{
    Ref a, b;
    long x, y;
    if (n == 2) {
        a = av[0];
        b = av[1];
        if (id == 3) return cons(a, b);
        if (id == 17) return a == b ? TRU : NIL;
        if (a < 0 && b < 0) {
            x = intval(a);
            y = intval(b);
            switch (id) {
            case 4: return mkint(IADD(x, y));
            case 5: return mkint(ISUB(x, y));
            case 6: return mkint(IMUL(x, y));
            case 8: return x == y ? TRU : NIL;
            case 9: return x < y ? TRU : NIL;
            case 10: return x > y ? TRU : NIL;
            case 11: return x <= y ? TRU : NIL;
            case 12: return x >= y ? TRU : NIL;
            }
        }
        return 0;
    }
    if (n == 1) {
        a = av[0];
        if (id == 1) {
            if (a == NIL) return NIL;
            if (a > 0 && ctag[a] == T_CONS) return ccar[a];
            return 0;
        }
        if (id == 2) {
            if (a == NIL) return NIL;
            if (a > 0 && ctag[a] == T_CONS) return ccdr[a];
            return 0;
        }
        if (id == 14 && a < 0) return mkint(IADD(intval(a), 1));
        if (id == 15 && a < 0) return mkint(ISUB(intval(a), 1));
        if (id == 16) return a == NIL ? TRU : NIL;
    }
    return 0;
}

/* call the primitive fn with the evaluated arguments */
static Ref callprimfn(Ref fn, Ref *av, long n)
{
    int id = csid[fn];
    Ref res;
    Sb b;
    if (id <= 17) {
        res = fastprim(id, av, n);
        if (res != 0) return res;
    }
    if (n < pmin[id] || (pmax[id] >= 0 && n > pmax[id])) {
        sb_init(&b);
        sb_adds(&b, n < pmin[id] ? "too few" : "too many");
        sb_adds(&b, " arguments to ");
        sb_adds(&b, XS(fn));
        lerror(sb_str(&b));
        free(b.p);
        return NIL;
    }
    curfn = fn;
    return callprim(id, av, n);
}

/* A call of the function fn (closure, primitive or macro) in the
   form *px.  Returns 0 when the call is finished, with its value in
   *res, and 1 when evaluation goes on in *px and *penv. */
static int callform(Ref *px, Ref *penv, Ref fn, Ref *res, Seg *sg)
{
    Ref x = *px, env = *penv, env2, ex, f, v, l;
    Ref avl[8], *av;
    long n, cnt, nc;
    int tf = ctag[fn];
    if (tf == T_MACRO) {
        av = list2arr(ccdr[x], &n);
        nc = nclos;
        env2 = bindparams(ccar[fn], av, n, XL(fn));
        free(av);
        if (unw != 0) {
            *res = NIL;
            return 0;
        }
        ex = evalbody(ccdr[fn], env2);
        if (nclos == nc) freeframe(env2, XL(fn));
        if (unw != 0) {
            *res = NIL;
            return 0;
        }
        if (ISCONS(ex)) {
            ccar[x] = ccar[ex];
            ccdr[x] = ccdr[ex];
        } else *px = ex;
        return 1;
    }
    if (tf != T_CLOS && tf != T_PRIM) {
        lerrobj("ERROR", "the value ", fn, " is not a function");
        *res = NIL;
        return 0;
    }
    cnt = listlen(ccdr[x]);
    if (cnt <= 8) av = avl;
    else av = (Ref *)xmalloc(cnt * (long)sizeof(Ref));
    n = 0;
    for (l = ccdr[x]; ISCONS(l); l = ccdr[l]) {
        f = ccar[l];
        if (f < 0) v = f;
        else if (ctag[f] == T_SYM) v = symval(f, env);
        else if (ctag[f] == T_CONS) v = eval(f, env);
        else v = f;
        av[n++] = v;
        if (unw != 0) {
            if (av != avl) free(av);
            *res = NIL;
            return 0;
        }
    }
    if (tf == T_PRIM) {
        *res = callprimfn(fn, av, n);
        if (av != avl) free(av);
        return 0;
    }
    nc = nclos;
    env2 = bindparams(ccar[fn], av, n, XL(fn));
    if (av != avl) free(av);
    if (unw != 0) {
        *res = NIL;
        return 0;
    }
    ex = evalprefix(ccdr[fn], env2);
    if (ex == 0 || unw != 0) {
        if (nclos == nc) freeframe(env2, XL(fn));
        *res = NIL;
        return 0;
    }
    /* the caller's frames are dead now unless a closure took them */
    if (nclos == sg->n0) freeframe(sg->top, sg->bottom);
    sg->top = env2;
    sg->bottom = XL(fn);
    sg->n0 = nc;
    *px = ex;
    *penv = env2;
    return 1;
}

/* --------------------------------------------------- the evaluator */
Ref undyn(Ref d)
{
    Ref b;
    while (d != NIL) {
        b = ccar[d];
        ccar[ccar[b]] = ccdr[b];
        d = ccdr[d];
    }
    return NIL;
}

static Ref eval1(Ref x, Ref env, Seg *sg)
{
    Ref f, fn, c, p, cl, k, ks, v, bl, body, nenv, dyn, s, b, a;
    int sid, found, hit, tg;
    Ref res;
    for (;;) {
        if (x < 0) return x;
        tg = ctag[x];
        if (tg == T_SYM) return symval(x, env);
        if (tg != T_CONS) return x;
        f = ccar[x];
        sid = 0;
        if (f > 0 && ctag[f] == T_SYM) sid = csid[f];
        if (sid == 0) {
            fn = NIL;
            if (f > 0 && ctag[f] == T_CONS) {
                if (ccar[f] == sLAMBDA) {
                    fn = mkclosure(car(cdr(f)), CDR(CDR(f)), env);
                } else return lerror("illegal function call");
            } else if (f > 0 && ctag[f] == T_SYM) {
                fn = getfn(f, env);
            } else return lerror("illegal function call");
            if (unw != 0) return NIL;
            if (!callform(&x, &env, fn, &res, sg)) return res;
            continue;
        }
        switch (sid) {
        case 1:
            return CAR(CDR(x));
        case 2:
            a = CAR(CDR(x));
            if (ISCONS(a))
                return mkclosure(car(cdr(a)), CDR(CDR(a)), env);
            return getfn(a, env);
        case 3:
            p = ccdr[x];
            c = eval(CAR(p), env);
            if (unw != 0) return NIL;
            p = CDR(p);
            if (c != NIL) x = CAR(p);
            else {
                p = CDR(p);
                if (p == NIL) return NIL;
                x = CAR(p);
            }
            break;
        case 4:
            x = evalprefix(ccdr[x], env);
            if (x == 0) return NIL;
            break;
        case 5:
        case 6:
            bl = car(ccdr[x]);
            body = CDR(CDR(x));
            nenv = env;
            dyn = NIL;
            while (ISCONS(bl)) {
                b = ccar[bl];
                if (ISCONS(b)) {
                    s = ccar[b];
                    v = NIL;
                    if (ccdr[b] != NIL) {
                        if (sid == 5) v = eval(car(ccdr[b]), env);
                        else v = eval(car(ccdr[b]), nenv);
                    }
                } else {
                    s = b;
                    v = NIL;
                }
                if (unw != 0) {
                    undyn(dyn);
                    return NIL;
                }
                if (s <= 0) {
                    undyn(dyn);
                    return lerror("invalid binding");
                }
                if (cspec[s] == 1) {
                    dyn = cons(cons(s, ccar[s]), dyn);
                    ccar[s] = v;
                } else nenv = cons(cons(s, v), nenv);
                bl = ccdr[bl];
            }
            if (dyn == NIL) {
                x = evalprefix(body, nenv);
                if (x == 0) return NIL;
                env = nenv;
                sg->top = nenv;
            } else {
                v = evalbody(body, nenv);
                undyn(dyn);
                return v;
            }
            break;
        case 7:
            cl = ccdr[x];
            found = 0;
            while (ISCONS(cl) && !found) {
                c = ccar[cl];
                v = eval(CAR(c), env);
                if (unw != 0) return NIL;
                if (v != NIL) {
                    if (CDR(c) == NIL) return v;
                    x = evalprefix(ccdr[c], env);
                    if (x == 0) return NIL;
                    found = 1;
                }
                cl = ccdr[cl];
            }
            if (!found) return NIL;
            break;
        case 8:
        case 13:
            k = eval(car(ccdr[x]), env);
            if (unw != 0) return NIL;
            cl = CDR(CDR(x));
            found = 0;
            while (ISCONS(cl) && !found) {
                c = ccar[cl];
                ks = CAR(c);
                hit = 0;
                if (ks == TRU || ks == sOTHERWISE) hit = 1;
                else if (ISCONS(ks)) {
                    while (ISCONS(ks)) {
                        if (eql(ccar[ks], k)) hit = 1;
                        ks = ccdr[ks];
                    }
                } else if (eql(ks, k)) hit = 1;
                if (hit) {
                    found = 1;
                    x = evalprefix(CDR(c), env);
                    if (x == 0) return NIL;
                }
                cl = ccdr[cl];
            }
            if (!found) {
                if (sid == 13)
                    return lerrobj("ERROR",
                        "no matching ECASE clause for ", k, "");
                return NIL;
            }
            break;
        case 9:
            p = ccdr[x];
            if (p == NIL) return TRU;
            while (ISCONS(ccdr[p])) {
                v = eval(ccar[p], env);
                if (unw != 0) return NIL;
                if (v == NIL) return NIL;
                p = ccdr[p];
            }
            x = ccar[p];
            break;
        case 10:
            p = ccdr[x];
            if (p == NIL) return NIL;
            while (ISCONS(ccdr[p])) {
                v = eval(ccar[p], env);
                if (unw != 0) return NIL;
                if (v != NIL) return v;
                p = ccdr[p];
            }
            x = ccar[p];
            break;
        case 11:
        case 12:
            c = eval(car(ccdr[x]), env);
            if (unw != 0) return NIL;
            if (sid == 11 && c == NIL) return NIL;
            if (sid == 12 && c != NIL) return NIL;
            x = evalprefix(CDR(CDR(x)), env);
            if (x == 0) return NIL;
            break;
        case 14:
            /* loop: a simple loop or the extended form */
            if (ISCONS(ccdr[x]) && ISCONS(car(ccdr[x])))
                return simpleloop(ccdr[x], env);
            ccar[x] = sXLOOP;
            break;
        default:
            return evalspecial(sid, x, env);
        }
    }
}

Ref eval(Ref x, Ref env)
{
    Ref r;
    Seg sg;
    int tg;
    if (x < 0) return x;
    tg = ctag[x];
    if (tg == T_SYM) return symval(x, env);
    if (tg != T_CONS) return x;
    ndepth++;
    if (ndepth > maxdepth) {
        ndepth--;
        return lerror("evaluation nested too deeply "
                      "(stack exhausted)");
    }
    sg.top = sg.bottom = env;
    sg.n0 = nclos;
    r = eval1(x, env, &sg);
    if (nclos == sg.n0) freeframe(sg.top, sg.bottom);
    ndepth--;
    return r;
}
