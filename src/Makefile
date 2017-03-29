ROOT_DIR = .

SUBDIRS += \
	common \
	reluplex \

all:
	@echo Done

.PHONY: TAGS

FINDARGS = -iname Makefile \
		-or -iname \*.mk \
		-or -iname \*.h \
		-or -iname \*.cpp \
		-or -iname \*.c | \
		$(GREP) -v TestRunner | \
		$(XARGS) $(ETAGS)

TAGS:
	$(GFIND) primal $(FINDARGS) -a
	$(ETAGS) -a Makefile

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C . "
# tags-file-name: "./TAGS"
# c-basic-offset: 4
# End:
#
