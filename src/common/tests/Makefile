ROOT_DIR = ../..

TEST_TARGET = CommonTests.tests

LOCAL_INCLUDES += \
	.. \

# TEST_FILES = $(wildcard Test_*.h)

TEST_FILES = \
	Test_String.h \
	Test_List.h \
	Test_Stringf.h \

SOURCES += \
	../String.cpp \

# CommonMock.cpp \
# 	Errno.cpp \
# 	SocketFactory.cpp \
# 	TestLog.cpp \
# 	FileFactory.cpp \

all:
	./$(TEST_TARGET)

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(COMMON_MOCK_DIR)

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
