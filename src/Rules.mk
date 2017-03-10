#
# Places
#

SRC_DIR         = $(ROOT_DIR)
PROJECT_DIR     = $(ROOT_DIR)/..

TOOLS_DIR	= $(PROJECT_DIR)/tools
CXXTEST_DIR 	= $(TOOLS_DIR)/cxxtest

COMMON_DIR 	= $(SRC_DIR)/common
COMMON_MOCK_DIR = $(COMMON_DIR)/Mock
COMMON_REAL_DIR = $(COMMON_DIR)/Real
COMMON_TEST_DIR = $(COMMON_DIR)/tests

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
	-Wno-deprecated \
	-Wno-unused-but-set-variable \
	-std=c++0x \
	\
	-g \

%.obj: %.cpp
	$(COMPILE) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(LOCAL_INCLUDES))

%.obj: %.cxx
	$(COMPILE) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(LOCAL_INCLUDES))

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
	$(LINK) $(LINK_FLAGS) -o $@ $^ $(addprefix -l, $(SYSTEM_LIBRARIES)) $(addprefix -l, $(LOCAL_LIBRARIES))

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
	$(TESTGEN) --part --have-eh --abort-on-fail -o $@ $^

runner.cxx:
	$(TESTGEN) --root --have-eh --abort-on-fail --error-printer -o $@

%.tests: $(TEST_OBJECTS)
	$(LINK) -o $@ $^ $(addprefix -l, $(SYSTEM_LIBRARIES))

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
