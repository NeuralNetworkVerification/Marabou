ROOT_DIR = ../..

TEST_TARGET = CommonTests.tests

LOCAL_INCLUDES += \
	.. \

TEST_FILES = $(wildcard Test_*.h)

SOURCES += \
	../String.cpp \
	../Error.cpp \
	../ConstSimpleData.cpp \
	../HeapData.cpp \
	../File.cpp \
	\
	../mock/Errno.cpp \
	../mock/CommonMock.cpp \

all:
	./$(TEST_TARGET)

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
