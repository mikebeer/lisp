# C89 version of the Lisp interpreter
CC      ?= cc
CFLAGS  ?= -O2 -std=c89 -pedantic -Wall -Wextra
LIBS    = -lm
SRC     = core.c num.c print.c read.c io.c eval.c special.c \
          forms.c fmt.c table.c prim_seq.c prim_lst.c \
          prim_vec.c prim_num.c prim_str.c prim_msc.c prims.c \
          prelude.c main.c
OBJ     = $(SRC:.c=.o)

all: lisp-c

lisp-c: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

%.o: %.c lisp.h
	$(CC) $(CFLAGS) -c $<

test: lisp-c
	sh tests/run.sh

lines:
	@awk 'length($$0)>71 {print FILENAME ": " FNR ": " length($$0)}' \
	  *.c *.h Makefile | grep . && exit 1 || echo "lines ok"

clean:
	rm -f lisp-c *.o

.PHONY: all test lines clean
