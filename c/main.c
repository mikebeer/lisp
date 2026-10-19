/* main.c -- initialisation and the driver.
   Usage:  lisp-c [file.lisp ...]    load and run the files
           lisp-c                    read-eval-print loop on stdin */
#include <limits.h>
#include "lisp.h"

#if defined(__linux__)
#include <sys/resource.h>
#endif

static void growstack(void)
{
#if defined(__linux__)
    struct rlimit rl;
    if (getrlimit(RLIMIT_STACK, &rl) == 0) {
        if (rl.rlim_max == RLIM_INFINITY ||
            rl.rlim_max > 512UL * 1024UL * 1024UL)
            rl.rlim_cur = 512UL * 1024UL * 1024UL;
        else rl.rlim_cur = rl.rlim_max;
        setrlimit(RLIMIT_STACK, &rl);
    }
#endif
}

static void sf(const char *name, int id)
{
    Ref s = intern(name);
    csid[s] = (unsigned char)id;
}

static void defconst(const char *name, Ref v)
{
    Ref s = intern(name);
    ccar[s] = v;
    cspec[s] = 2;
}

void initlisp(void)
{
    heapinit();
    maxdepth = 24000;
    sb_init(&capbuf);
    intern("NIL");
    intern("T");
    ccar[1] = NIL;
    ccdr[1] = NIL;
    ccar[2] = TRU;
    cspec[1] = 2;
    cspec[2] = 2;
    sQUOTE = intern("QUOTE");
    sFUNCTION = intern("FUNCTION");
    sQUASI = intern("QUASIQUOTE");
    sUNQ = intern("UNQUOTE");
    sUNQS = intern("UNQUOTE-SPLICING");
    sLAMBDA = intern("LAMBDA");
    sOPT = intern("&OPTIONAL");
    sREST = intern("&REST");
    sKEY = intern("&KEY");
    sAUX = intern("&AUX");
    sBODY = intern("&BODY");
    sELSE = intern("ELSE");
    sERROR = intern("ERROR");
    sTHEN = intern("THEN");
    sOTHERWISE = intern("OTHERWISE");
    sBLOCK = intern("BLOCK");
    sRETFROM = intern("RETURN-FROM");
    sRETURN = intern("RETURN");
    sXLOOP = intern("%LOOP");
    sf("QUOTE", 1);
    sf("FUNCTION", 2);
    sf("IF", 3);
    sf("PROGN", 4);
    sf("LOCALLY", 4);
    sf("LET", 5);
    sf("LET*", 6);
    sf("COND", 7);
    sf("CASE", 8);
    sf("AND", 9);
    sf("OR", 10);
    sf("WHEN", 11);
    sf("UNLESS", 12);
    sf("ECASE", 13);
    sf("LOOP", 14);
    sf("SETQ", 21);
    sf("DEFUN", 22);
    sf("DEFVAR", 23);
    sf("DEFPARAMETER", 24);
    sf("DEFMACRO", 25);
    sf("LAMBDA", 26);
    sf("SETF", 27);
    sf("INCF", 28);
    sf("DECF", 29);
    sf("PUSH", 30);
    sf("POP", 31);
    sf("DO", 32);
    sf("DO*", 33);
    sf("DOLIST", 34);
    sf("DOTIMES", 35);
    sf("FLET", 37);
    sf("LABELS", 38);
    sf("BLOCK", 39);
    sf("RETURN-FROM", 40);
    sf("RETURN", 41);
    sf("CATCH", 42);
    sf("THROW", 43);
    sf("HANDLER-CASE", 44);
    sf("IGNORE-ERRORS", 45);
    sf("UNWIND-PROTECT", 46);
    sf("QUASIQUOTE", 47);
    sf("THE", 48);
    sf("DECLARE", 49);
    sf("DECLAIM", 49);
    sf("PROCLAIM", 49);
    sf("IN-PACKAGE", 49);
    sf("DEFPACKAGE", 49);
    sf("DEFSTRUCT", 50);
    sf("DEFCONSTANT", 51);
    sf("PUSHNEW", 52);
    sf("ASSERT", 53);
    sf("MULTIPLE-VALUE-BIND", 54);
    sf("WITH-OUTPUT-TO-STRING", 55);
    sf("PROG1", 56);
    sf("PROG2", 57);
    sf("DESTRUCTURING-BIND", 58);
    sf("TYPECASE", 59);
    sf("ETYPECASE", 59);
    sf("TIME", 60);
    sf("MULTIPLE-VALUE-LIST", 61);
    sf("NTH-VALUE", 62);
    sf("ROTATEF", 63);
    sf("PSETQ", 64);
    defconst("PI", mkfloat(3.14159265358979323846));
    defconst("MOST-POSITIVE-FIXNUM", mkint(LONG_MAX / 2));
    defconst("MOST-NEGATIVE-FIXNUM", mkint(0 - LONG_MAX / 2));
    defconst("INTERNAL-TIME-UNITS-PER-SECOND", mkint(1000));
    ccar[intern("*STANDARD-OUTPUT*")] = TRU;
    ccar[intern("*ERROR-OUTPUT*")] = TRU;
    ccar[intern("*STANDARD-INPUT*")] = TRU;
    ccar[intern("*PRINT-PRETTY*")] = NIL;
}

/* run the library written in Lisp */
void initprelude(void)
{
    Sb b;
    int i;
    sb_init(&b);
    for (i = 0; prelude[i] != NULL; i++) sb_adds(&b, prelude[i]);
    sb_str(&b);
    runsource(b.p, b.n);
    free(b.p);
}

int main(int argc, char **argv)
{
    int i, rc = 0;
    growstack();
    initlisp();
    initprims();
    initprelude();
    if (argc >= 2) {
        for (i = 1; i < argc; i++) {
            rc = loadfile(argv[i]);
            if (rc != 0) break;
        }
        freshline();
        fflush(stdout);
        return rc;
    }
    repl();
    fflush(stdout);
    return 0;
}
