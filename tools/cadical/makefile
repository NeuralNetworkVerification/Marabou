CADICALBUILD=/cs/labs/guykatz/omrii99/CDCL/Marabou/tools/cadical/build
all:
	$(MAKE) -C "$(CADICALBUILD)"
clean:
	@if [ -d "$(CADICALBUILD)" ]; \
	then \
	  if [ -f "$(CADICALBUILD)"/makefile ]; \
	  then \
	     touch "$(CADICALBUILD)"/build.hpp; \
	     $(MAKE) -C "$(CADICALBUILD)" clean; \
	  fi; \
	  rm -rf "$(CADICALBUILD)"; \
	fi
	rm -f "/cs/labs/guykatz/omrii99/CDCL/Marabou/tools/cadical/src/makefile"
	rm -f "/cs/labs/guykatz/omrii99/CDCL/Marabou/tools/cadical/makefile"
test:
	$(MAKE) -C "$(CADICALBUILD)" test
cadical:
	$(MAKE) -C "$(CADICALBUILD)" cadical
mobical:
	$(MAKE) -C "$(CADICALBUILD)" mobical
update:
	$(MAKE) -C "$(CADICALBUILD)" update
format:
	$(MAKE) -C "$(CADICALBUILD)" format
.PHONY: all cadical clean mobical test format
