/* forms.c -- evalspecial: the special forms that are not handled
   inline by the evaluator loop. */
#include "lisp.h"

#define CAR(r) ((r) > 0 ? ccar[r] : NIL)
#define CDR(r) ((r) > 0 ? ccdr[r] : NIL)

static int hasclosure(Ref x)
{
    Ref a;
    while (ISCONS(x)) {
        a = ccar[x];
        if (a == sLAMBDA || a == sFUNCTION) return 1;
        if (ISCONS(a) && hasclosure(a)) return 1;
        x = ccdr[x];
    }
    return 0;
}

static int loopstat(void)
{
    if (unw == 0) return 0;
    if (unw == 3 && unwtag == NIL) {
        unw = 0;
        return 1;
    }
    return 2;
}

/* the key symbol of a local function name */
static Ref fkeyof(Ref f)
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

/* setq, defun, defvar and friends */
/* cREXX stores into any cell given as a name; keep that for the
   cells whose car and cdr are free */
static int plainname(Ref n)
{
    if (n <= 0) return 0;
    return ctag[n] == T_SYM || ctag[n] == T_CONS ||
           ctag[n] == T_FLOAT || ctag[n] == T_CHAR;
}

static Ref sf_def(int sid, Ref x, Ref env)
{
    Ref p, v, name, params, body, c, init;
    switch (sid) {
    case 21:
        p = ccdr[x];
        v = NIL;
        while (ISCONS(p)) {
            v = eval(car(CDR(p)), env);
            if (unw != 0) return NIL;
            setvar(ccar[p], v, env);
            if (unw != 0) return NIL;
            p = CDR(CDR(p));
        }
        return v;
    case 22:
        name = car(ccdr[x]);
        if (!plainname(name)) return lerror("invalid function name");
        params = car(CDR(CDR(x)));
        body = stripdoc(CDR(CDR(CDR(x))));
        c = blockwrap(name, body);
        c = mkclosure(params, c, env);
        ccdr[name] = c;
        return name;
    case 23:
    case 24:
    case 51:
        name = car(ccdr[x]);
        if (!plainname(name)) return lerror("invalid variable name");
        cspec[name] = 1;
        if (sid == 51) cspec[name] = 2;
        if (sid == 24 || sid == 51 || ccar[name] == 0) {
            init = car(CDR(CDR(x)));
            v = eval(init, env);
            if (unw != 0) return NIL;
            ccar[name] = v;
        }
        return name;
    default:
        /* 25 defmacro */
        name = car(ccdr[x]);
        if (!plainname(name)) return lerror("invalid macro name");
        c = newcell(T_MACRO);
        ccar[c] = car(CDR(CDR(x)));
        ccdr[c] = stripdoc(CDR(CDR(CDR(x))));
        XL(c) = env;
        nclos++;
        ccdr[name] = c;
        return name;
    }
}

/* setf incf decf push pushnew pop rotatef psetq */
static Ref sf_place(int sid, Ref x, Ref env)
{
    Ref p, v, pl, cur, d, q, pls, first, nxt, tmp, *vals;
    long n, k;
    switch (sid) {
    case 27:
        p = ccdr[x];
        v = NIL;
        while (ISCONS(p)) {
            v = eval(car(CDR(p)), env);
            if (unw != 0) return NIL;
            setplace(ccar[p], v, env);
            if (unw != 0) return NIL;
            p = CDR(CDR(p));
        }
        return v;
    case 28:
    case 29:
        pl = car(ccdr[x]);
        cur = eval(pl, env);
        if (unw != 0) return NIL;
        d = mkint(1);
        if (CDR(CDR(x)) != NIL) {
            d = eval(car(CDR(CDR(x))), env);
            if (unw != 0) return NIL;
        }
        if (sid == 29) d = numneg(d);
        v = numadd(cur, d);
        if (unw != 0) return NIL;
        return setplace(pl, v, env);
    case 30:
    case 52:
        v = eval(car(ccdr[x]), env);
        if (unw != 0) return NIL;
        pl = car(CDR(CDR(x)));
        cur = eval(pl, env);
        if (unw != 0) return NIL;
        if (sid == 52) {
            for (q = cur; ISCONS(q); q = ccdr[q])
                if (eql(ccar[q], v)) return cur;
        }
        return setplace(pl, cons(v, cur), env);
    case 31:
        pl = car(ccdr[x]);
        cur = eval(pl, env);
        if (unw != 0) return NIL;
        setplace(pl, cdr(cur), env);
        return car(cur);
    case 63:
        pls = ccdr[x];
        if (pls == NIL) return NIL;
        first = ccar[pls];
        cur = eval(first, env);
        p = pls;
        while (ISCONS(ccdr[p])) {
            nxt = eval(ccar[ccdr[p]], env);
            setplace(ccar[p], nxt, env);
            p = ccdr[p];
        }
        setplace(ccar[p], cur, env);
        return NIL;
    default:
        /* 64 psetq */
        p = ccdr[x];
        n = 0;
        for (q = p; ISCONS(q); q = CDR(CDR(q))) n++;
        vals = (Ref *)xmalloc((n + 1) * (long)sizeof(Ref));
        k = 0;
        for (q = p; ISCONS(q); q = CDR(CDR(q))) {
            tmp = eval(car(CDR(q)), env);
            vals[k++] = tmp;
        }
        k = 0;
        while (ISCONS(p)) {
            setvar(ccar[p], vals[k++], env);
            p = CDR(CDR(p));
        }
        free(vals);
        return NIL;
    }
}

/* do and do* */
static Ref sf_do(int sid, Ref x, Ref env)
{
    Ref specs, endc, body, nenv, blist, slist, p, spc, stp, init, vrb;
    Ref v, b, t, q, bl, sl, *vals, r;
    long k, nv;
    specs = car(ccdr[x]);
    endc = car(CDR(CDR(x)));
    body = CDR(CDR(CDR(x)));
    nenv = env;
    blist = NIL;
    slist = NIL;
    nv = 0;
    for (p = specs; ISCONS(p); p = ccdr[p]) {
        spc = ccar[p];
        stp = NIL;
        init = NIL;
        vrb = spc;
        if (ISCONS(spc)) {
            vrb = ccar[spc];
            init = car(ccdr[spc]);
            stp = CDR(CDR(spc));
        }
        if (sid == 32) v = eval(init, env);
        else v = eval(init, nenv);
        if (unw != 0) return NIL;
        b = cons(vrb, v);
        nenv = cons(b, nenv);
        blist = cons(b, blist);
        slist = cons(stp, slist);
        nv++;
    }
    blist = nreverse(blist);
    slist = nreverse(slist);
    vals = (Ref *)xmalloc((nv + 1) * (long)sizeof(Ref));
    for (;;) {
        t = eval(car(endc), nenv);
        if (unw != 0) {
            free(vals);
            return NIL;
        }
        if (t != NIL) {
            free(vals);
            return evalbody(cdr(endc), nenv);
        }
        for (q = body; ISCONS(q); q = ccdr[q]) {
            eval(ccar[q], nenv);
            k = loopstat();
            if (k == 1) {
                free(vals);
                return unwval;
            }
            if (k == 2) {
                free(vals);
                return NIL;
            }
        }
        bl = blist;
        sl = slist;
        k = 0;
        while (ISCONS(bl)) {
            if (ccar[sl] != NIL) {
                v = eval(car(ccar[sl]), nenv);
                if (unw != 0) {
                    free(vals);
                    return NIL;
                }
                if (sid == 33) ccdr[ccar[bl]] = v;
                else vals[k] = v;
            } else vals[k] = ccdr[ccar[bl]];
            k++;
            bl = ccdr[bl];
            sl = ccdr[sl];
        }
        if (sid == 32) {
            bl = blist;
            k = 0;
            while (ISCONS(bl)) {
                ccdr[ccar[bl]] = vals[k++];
                bl = ccdr[bl];
            }
        }
    }
    r = NIL;
    return r;
}

/* dolist and dotimes */
static Ref sf_dolist(int sid, Ref x, Ref env)
{
    Ref spec, vrb, l, b, nenv, body, q, r;
    long cnt, i, nmax = 0;
    int fresh, st;
    spec = car(ccdr[x]);
    vrb = car(spec);
    l = eval(car(cdr(spec)), env);
    if (unw != 0) return NIL;
    if (sid == 35) {
        if (l >= 0) return lerror("dotimes needs an integer count");
        nmax = intval(l);
    }
    b = cons(vrb, NIL);
    nenv = cons(b, env);
    body = CDR(CDR(x));
    fresh = hasclosure(body);
    if (sid == 34) {
        while (ISCONS(l)) {
            if (fresh) {
                b = cons(vrb, NIL);
                nenv = cons(b, env);
            }
            ccdr[b] = ccar[l];
            for (q = body; ISCONS(q); q = ccdr[q]) {
                eval(ccar[q], nenv);
                st = loopstat();
                if (st == 1) return unwval;
                if (st == 2) return NIL;
            }
            l = ccdr[l];
        }
        nenv = cons(cons(vrb, NIL), env);
    } else {
        for (i = 0; i < nmax; i++) {
            if (fresh) {
                b = cons(vrb, NIL);
                nenv = cons(b, env);
            }
            cnt = i;
            ccdr[b] = mkint(cnt);
            for (q = body; ISCONS(q); q = ccdr[q]) {
                eval(ccar[q], nenv);
                st = loopstat();
                if (st == 1) return unwval;
                if (st == 2) return NIL;
            }
        }
        nenv = cons(cons(vrb, mkint(nmax)), env);
    }
    r = car(cdr(cdr(spec)));
    return eval(r, nenv);
}

/* flet labels block return-from return catch throw */
static Ref sf_flow(int sid, Ref x, Ref env)
{
    Ref nenv, cl, p, d, name, c, r, tag, vf, v, q;
    int found;
    char *s;
    Sb b;
    switch (sid) {
    case 37:
    case 38:
        hasfenv = 1;
        nenv = env;
        cl = NIL;
        for (p = car(ccdr[x]); ISCONS(p); p = ccdr[p]) {
            d = ccar[p];
            name = CAR(d);
            if (!ISSYM(name)) return lerror("invalid function name");
            c = blockwrap(name, CDR(CDR(d)));
            c = mkclosure(car(CDR(d)), c, env);
            nenv = cons(cons(fkeyof(name), c), nenv);
            cl = cons(c, cl);
        }
        if (sid == 38) {
            while (ISCONS(cl)) {
                XL(ccar[cl]) = nenv;
                cl = ccdr[cl];
            }
        }
        return evalbody(CDR(CDR(x)), nenv);
    case 39:
        name = car(ccdr[x]);
        r = evalbody(CDR(CDR(x)), env);
        if (unw == 3 && unwtag == name) {
            unw = 0;
            r = unwval;
        }
        return r;
    case 40:
    case 41:
        if (sid == 40) {
            tag = car(ccdr[x]);
            vf = car(CDR(CDR(x)));
        } else {
            tag = NIL;
            vf = car(ccdr[x]);
        }
        v = eval(vf, env);
        if (unw != 0) return NIL;
        unw = 3;
        unwtag = tag;
        unwval = v;
        return NIL;
    case 42:
        tag = eval(car(ccdr[x]), env);
        if (unw != 0) return NIL;
        catchtags = cons(tag, catchtags);
        r = evalbody(CDR(CDR(x)), env);
        c = catchtags;
        catchtags = ccdr[catchtags];
        freecell(c);
        if (unw == 2 && eql(unwtag, tag)) {
            unw = 0;
            r = unwval;
        }
        return r;
    default:
        /* 43 throw */
        tag = eval(car(ccdr[x]), env);
        if (unw != 0) return NIL;
        v = eval(car(CDR(CDR(x))), env);
        if (unw != 0) return NIL;
        found = 0;
        for (q = catchtags; ISCONS(q); q = ccdr[q])
            if (eql(ccar[q], tag)) found = 1;
        if (!found) {
            s = printobj(tag, 1);
            sb_init(&b);
            sb_adds(&b, "attempt to throw to a tag that does not ");
            sb_adds(&b, "exist: ");
            sb_adds(&b, s);
            lerrk("CONTROL-ERROR", sb_str(&b));
            free(b.p);
            free(s);
            return NIL;
        }
        unw = 2;
        unwtag = tag;
        unwval = v;
        return NIL;
    }
}

/* handler-case ignore-errors unwind-protect */
static Ref sf_handler(int sid, Ref x, Ref env)
{
    Ref r, cl, c, cond, nenv, vr;
    int su;
    Ref st, sv;
    char *sm;
    const char *sk;
    if (sid == 44) {
        r = eval(car(ccdr[x]), env);
        if (unw == 1) {
            for (cl = CDR(CDR(x)); ISCONS(cl); cl = ccdr[cl]) {
                c = ccar[cl];
                if (condmatch(CAR(c))) {
                    cond = mkstr(errmsg);
                    unw = 0;
                    nenv = env;
                    vr = car(car(CDR(c)));
                    if (vr != NIL) nenv = cons(cons(vr, cond), env);
                    return evalbody(CDR(CDR(c)), nenv);
                }
            }
        }
        return r;
    }
    if (sid == 45) {
        r = evalbody(ccdr[x], env);
        if (unw == 1) {
            unw = 0;
            return NIL;
        }
        return r;
    }
    /* 46 unwind-protect */
    r = eval(car(ccdr[x]), env);
    su = unw;
    st = unwtag;
    sv = unwval;
    sm = errmsg ? xstrdup(errmsg) : NULL;
    sk = errkind;
    unw = 0;
    evalbody(CDR(CDR(x)), env);
    if (unw == 0) {
        unw = su;
        unwtag = st;
        unwval = sv;
        if (errmsg) free(errmsg);
        errmsg = sm;
        errkind = sk;
    } else if (sm) free(sm);
    return r;
}

/* values and friends */
static Ref sf_values(int sid, Ref x, Ref env)
{
    Ref r, vals, nenv, vs, l, nn, v, s;
    long i, k;
    char *res;
    Sb old;
    switch (sid) {
    case 54:
        mvlist = 0;
        r = eval(car(CDR(CDR(x))), env);
        if (unw != 0) return NIL;
        vals = mvlist;
        if (vals == 0) vals = list1(r);
        mvlist = 0;
        nenv = env;
        for (vs = car(ccdr[x]); ISCONS(vs); vs = ccdr[vs]) {
            v = car(vals);
            nenv = cons(cons(ccar[vs], v), nenv);
            vals = cdr(vals);
        }
        return evalbody(CDR(CDR(CDR(x))), nenv);
    case 55:
        old = capbuf;
        sb_init(&capbuf);
        capdepth++;
        s = car(car(ccdr[x]));
        nenv = cons(cons(s, TRU), env);
        evalbody(CDR(CDR(x)), nenv);
        capdepth--;
        res = sb_str(&capbuf);
        r = mkstrn(res, capbuf.n);
        free(capbuf.p);
        capbuf = old;
        return r;
    case 61:
        mvlist = 0;
        r = eval(car(ccdr[x]), env);
        if (unw != 0) return NIL;
        if (mvlist != 0) {
            l = mvlist;
            mvlist = 0;
            return l;
        }
        return list1(r);
    default:
        /* 62 nth-value */
        nn = eval(car(ccdr[x]), env);
        mvlist = 0;
        r = eval(car(CDR(CDR(x))), env);
        if (unw != 0) return NIL;
        vals = mvlist;
        if (vals == 0) vals = list1(r);
        mvlist = 0;
        k = nn < 0 ? intval(nn) : 0;
        for (i = 1; i <= k; i++) vals = cdr(vals);
        return car(vals);
    }
}

Ref evalspecial(int sid, Ref x, Ref env)
{
    Ref v, body, r, cl, c, ty, nenv, k;
    switch (sid) {
    case 21: case 22: case 23: case 24: case 25: case 51:
        return sf_def(sid, x, env);
    case 26:
        return mkclosure(car(ccdr[x]), CDR(CDR(x)), env);
    case 27: case 28: case 29: case 30: case 31: case 52:
    case 63: case 64:
        return sf_place(sid, x, env);
    case 32: case 33:
        return sf_do(sid, x, env);
    case 34: case 35:
        return sf_dolist(sid, x, env);
    case 37: case 38: case 39: case 40: case 41: case 42:
    case 43:
        return sf_flow(sid, x, env);
    case 44: case 45: case 46:
        return sf_handler(sid, x, env);
    case 54: case 55: case 61: case 62:
        return sf_values(sid, x, env);
    case 47:
        return qq(car(ccdr[x]), env);
    case 48:
        return eval(car(CDR(CDR(x))), env);
    case 49:
        return NIL;
    case 50:
        return defstruct(x, env);
    case 53:
        v = eval(car(ccdr[x]), env);
        if (unw != 0) return NIL;
        if (v == NIL)
            return lerrobj("ERROR", "assertion failed: ",
                           car(ccdr[x]), "");
        return NIL;
    case 56:
    case 57:
        body = ccdr[x];
        if (sid == 57) {
            eval(CAR(body), env);
            if (unw != 0) return NIL;
            body = CDR(body);
        }
        r = eval(CAR(body), env);
        if (unw != 0) return NIL;
        evalbody(CDR(body), env);
        return r;
    case 58:
        v = eval(car(CDR(CDR(x))), env);
        if (unw != 0) return NIL;
        nenv = destruct(car(ccdr[x]), v, env);
        if (unw != 0) return NIL;
        return evalbody(CDR(CDR(CDR(x))), nenv);
    case 59:
        k = eval(car(ccdr[x]), env);
        if (unw != 0) return NIL;
        for (cl = CDR(CDR(x)); ISCONS(cl); cl = ccdr[cl]) {
            c = ccar[cl];
            ty = CAR(c);
            if (ty == TRU || ty == sOTHERWISE)
                return evalbody(CDR(c), env);
            if (typep(k, ty)) return evalbody(CDR(c), env);
        }
        return NIL;
    case 60:
        return eval(car(ccdr[x]), env);
    default:
        return lerror("unknown special form");
    }
}
