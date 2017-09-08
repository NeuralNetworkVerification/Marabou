ROOT_DIR = .

SUBDIRS += \
	src \

all:
	@echo Done

.PHONY: regress

regress:
	make -C regress

.PHONY: TAGS

FINDARGS = -iname Makefile \
		-or -iname \*.mk \
		-or -iname \*.h \
		-or -iname \*.cpp \
		-or -iname \*.c | \
		$(GREP) -v TestRunner | \
		$(XARGS) $(ETAGS)

TAGS:
	$(GFIND) src/common $(FINDARGS)
	$(GFIND) src/configuration $(FINDARGS) -a
	$(GFIND) src/engine $(FINDARGS) -a
	$(GFIND) src/input_parsers $(FINDARGS) -a
	$(ETAGS) -a Makefile

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C . "
# tags-file-name: "./TAGS"
# c-basic-offset: 4
# End:
#
