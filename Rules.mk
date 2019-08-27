#
# Utilities
#

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	COMPILER = g++
endif
ifeq ($(UNAME_S),Darwin)
	COMPILER = clang++
endif

COMPILE = $(COMPILER)
LINK 	= $(COMPILER)
RM	= rm
GFIND 	= find
ETAGS	= etags
GREP 	= grep
XARGS	= xargs
CP	= cp

PERL 	= perl
TAR	= tar
UNZIP	= unzip

TESTGEN = $(CXXTEST_DIR)/cxxtestgen.pl

#
# Unzipping
#

%.unzipped: %.tar.bz2
	$(TAR) xjvf $<
	touch $@

%.unzipped: %.zip
	$(UNZIP) $<
	touch $@

#
# Compiling C/C++
#

LOCAL_INCLUDES += \
	. \
	$(COMMON_DIR) \
	$(CXXTEST_DIR) \
	$(BOOST_INCLUDES) \

CFLAGS += \
	-MMD \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-deprecated \
	-Wno-deprecated-copy \
	-Wno-maybe-uninitialized \
	-std=c++0x \

%.obj: %.cpp
	@echo "CC\t" $@
	@$(COMPILE) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(LOCAL_INCLUDES))


%.obj: %.cxx
	@echo "CC\t" $@
	@$(COMPILE) -c -o $@ $< $(CFLAGS) $(CXXFLAGS) $(addprefix -I, $(LOCAL_INCLUDES))

#
# Linking C/C++
#

LIBRARY_DIR += \

LIBRARIES += \
	pthread \

LINK_FLAGS += \

#
# Compiling Elf Files
#

ifneq ($(TARGET),)

DEPS = $(SOURCES:%.cpp=%.d)

OBJECTS = $(SOURCES:%.cpp=%.obj)

%.elf: $(OBJECTS)
	@echo "LD\t" $@
	@$(LINK) $(LINK_FLAGS) -o $@ $^ $(addprefix -L, $(LIBRARY_DIR)) $(addprefix -l, $(LIBRARIES))

.PRECIOUS: %.obj

endif

#
# Cxx Rules
#

ifneq ($(TEST_TARGET),)

CXXFLAGS += \
	-Wno-ignored-qualifiers \

CFLAGS += \
	-Wno-ignored-qualifiers \

ifeq ($(COMPILER),g++)
	GPPVERSION = $(shell $(COMPILE) --version | grep ^g++ | sed 's/^.* //g')
	GPPVERSION_MAJOR = $(shell echo $(GPPVERSION) | cut -f1 -d.)
	GPPVERSION_MINOR = $(shell echo $(GPPVERSION) | cut -f2 -d.)
	GPPVERSION_GTE_6_1 := $(shell [ $(GPPVERSION_MAJOR) -gt 6 -o \( $(GPPVERSION_MAJOR) -eq 6 -a $(GPPVERSION_MINOR) -ge 1 \) ] && echo true)
	ifeq ($(GPPVERSION_GTE_6_1),true)
		CXXFLAGS += -Wno-terminate
	endif
endif

OBJECTS = $(SOURCES:%.cpp=%.obj)

CXX_SOURCES = $(addprefix cxxtest_, $(TEST_FILES:%.h=%.cxx))
TEST_OBJECTS = $(CXX_SOURCES:%.cxx=%.obj) $(OBJECTS) runner.obj

DEPS = $(SOURCES:%.cpp=%.d) $(CXX_SOURCES:%.cxx=%.d)

cxxtest_%.cxx: %.h
	@echo "CXX\t" $@
	@$(TESTGEN) --part --have-eh --abort-on-fail -o $@ $^

runner.cxx:
	@echo "CXX\t" $@
	$(TESTGEN) --root --have-eh --abort-on-fail --error-printer -o $@

%.tests: $(TEST_OBJECTS)
	@echo "LD\t" $@
	@$(LINK) -o $@ $^ $(addprefix -L, $(LIBRARY_DIR)) $(addprefix -l, $(LIBRARIES))

.PRECIOUS: %.cxx %.obj

endif

#
# Recursive Make
#

all: $(SUBDIRS:%=%.all) $(TARGET) $(TEST_TARGET)

clean: $(SUBDIRS:%=%.clean) clean_directory

%.all:
	$(MAKE) -C $* all

%.clean:
	$(MAKE) -C $* clean

clean_directory:
	$(RM) -f *~ *.cxx *.obj *.d $(TARGET) $(TEST_TARGET)

ifneq ($(DEPS),)
-include $(DEPS)
endif

#
# Local Variables:
# compile-command: "make -C . "
# tags-file-name: "./TAGS"
# c-basic-offset: 4
# End:
#
