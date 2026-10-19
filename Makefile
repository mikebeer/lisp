# Build the Lisp interpreter.
#
#   make                 builds build/lisp.rxbin
#   make test            runs every program in tests/ and compares the output
#   make c               builds the C89 version, c/lisp-c
#   make test-c          runs the tests with the C89 version
#
# CREXX_BIN is the directory holding the cREXX tools (rxc, rxas, rxvm).
CREXX_BIN ?= $(dir $(shell which rxc))
RXC  = $(CREXX_BIN)/rxc
RXAS = $(CREXX_BIN)/rxas

all: build/lisp.rxbin

build/lisp.rxbin: lisp.crexx
	mkdir -p build
	cp lisp.crexx build/lisp.crexx
	cd build && $(RXC) -n lisp > lisp.log 2>&1 || (cat lisp.log; exit 1)
	cd build && $(RXAS) lisp

test: all
	./tests/run.sh

c:
	$(MAKE) -C c

test-c: c
	LISP=$(CURDIR)/c/lisp-c ./tests/run.sh

clean:
	rm -rf build

.PHONY: all test c test-c clean
