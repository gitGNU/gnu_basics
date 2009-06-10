.PHONY: all clean mrproper

all clean:
	@$(MAKE) -C src $@
	@$(MAKE) -C tst $@

mrproper: clean
	find . -name "*~" -exec rm {} \;
