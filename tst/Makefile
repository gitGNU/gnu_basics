export EXTRA_CPPFLAGS=-I../include
export EXTRA_CFLAGS=-O0 -g3
export EXTRA_LDFLAGS=-L../src -lb6

.PHONY: all clean mrproper

all clean:
	@$(MAKE) X="deque" SRC="deque.c test.c" -f ../build/Makefile $@
	@$(MAKE) X="list" SRC="list.c test.c" -f ../build/Makefile $@
	@$(MAKE) X="tree" SRC="node.c assert.c tree.c" -f ../build/Makefile $@
	@$(MAKE) X="splay" SRC="node.c assert.c splay.c" -f ../build/Makefile $@
