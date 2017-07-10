ROOT_DIR = ../..

TEST_TARGET = CommonTests.tests

LOCAL_INCLUDES += \
	.. \
	$(CONFIGURATION_DIR) \

TEST_FILES = $(wildcard Test_*.h)

SOURCES += \
	GlobalConfiguration.cpp \
	\
	String.cpp \
	Error.cpp \
	ConstSimpleData.cpp \
	HeapData.cpp \
	File.cpp \
	FloatUtils.cpp \
	\
	../mock/Errno.cpp \
	../mock/CommonMock.cpp \

all:
	./$(TEST_TARGET)

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(COMMON_DIR)

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
