# Act as if make was run in src/

.DEFAULT:
	@$(MAKE) -C src $@

all:
	@$(MAKE) -C src
