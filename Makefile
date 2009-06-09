.PHONY: all clean mrproper

all clean mrproper:
	@$(MAKE) -C src $@
	@$(MAKE) -C tst $@
