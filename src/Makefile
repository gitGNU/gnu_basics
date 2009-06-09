export EXTRA_CPPFLAGS=-I../include
export EXTRA_CFLAGS=-O0 -g3

export SRC=$(wildcard *.c)
export A=libb6.a
export L=libb6.so.1

.PHONY: all clean mrproper

all clean:
	@$(MAKE) -f ../build/Makefile $@

mrproper: clean
	$(RM) *~
