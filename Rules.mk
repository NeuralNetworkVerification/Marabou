#
# Places
#

PROJECT_DIR       = $(ROOT_DIR)
SRC_DIR           = $(ROOT_DIR)/src

TOOLS_DIR	  = $(PROJECT_DIR)/tools
CXXTEST_DIR 	  = $(TOOLS_DIR)/cxxtest

INPUT_PARSER_DIR  = $(SRC_DIR)/input_parsers

COMMON_DIR 	  = $(SRC_DIR)/common
COMMON_MOCK_DIR   = $(COMMON_DIR)/mock
COMMON_REAL_DIR   = $(COMMON_DIR)/real
COMMON_TEST_DIR   = $(COMMON_DIR)/tests

ENGINE_DIR        = $(SRC_DIR)/engine
ENGINE_MOCK_DIR   = $(ENGINE_DIR)/mock
ENGINE_REAL_DIR   = $(ENGINE_DIR)/real
ENGINE_TEST_DIR   = $(ENGINE_DIR)/tests

CONFIGURATION_DIR = $(SRC_DIR)/configuration

BASIS_FACTORIZATION_DIR = $(SRC_DIR)/basis_factorization

REGRESS_DIR	  = $(PROJECT_DIR)/regress

#
# Utilities
#

COMPILE = g++
LINK 	= g++
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

CFLAGS += \
	-MMD \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-error=terminate \
	-Wno-deprecated \
	-std=c++0x \
	\
	-g \

%.obj: %.cpp
	@echo "CC\t" $@
	@$(COMPILE) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(LOCAL_INCLUDES))


%.obj: %.cxx
	@echo "CC\t" $@
	@$(COMPILE) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(LOCAL_INCLUDES))

#
# Linking C/C++
#

SYSTEM_LIBRARIES += \

LOCAL_LIBRARIES += \

LINK_FLAGS += \

#
# Compiling Elf Files
#

ifneq ($(TARGET),)

DEPS = $(SOURCES:%.cpp=%.d)

OBJECTS = $(SOURCES:%.cpp=%.obj)

%.elf: $(OBJECTS)
	@echo "LD\t" $@
	@$(LINK) $(LINK_FLAGS) -o $@ $^ $(addprefix -l, $(SYSTEM_LIBRARIES)) $(addprefix -l, $(LOCAL_LIBRARIES))

.PRECIOUS: %.obj

endif

#
# Cxx Rules
#

ifneq ($(TEST_TARGET),)

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
	@$(LINK) -o $@ $^ $(addprefix -l, $(SYSTEM_LIBRARIES))

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
