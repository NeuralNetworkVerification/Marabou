ROOT_DIR = ../..

TEST_TARGET = CommonTests.tests

LOCAL_INCLUDES += \
	.. \

# TEST_FILES = $(wildcard Test_*.h)

TEST_FILES = \
	Test_String.h \
	Test_List.h \
	Test_Stringf.h \
	Test_Error.h \
	Test_Queue.h \
	Test_Vector.h \
	Test_Stack.h \
	Test_Set.h \
	Test_Map.h \
	Test_ConstSimpleData.h \
	Test_HeapData.h \

SOURCES += \
	../String.cpp \
	../Error.cpp \
	../ConstSimpleData.cpp \
	../HeapData.cpp \
	\
	../mock/Errno.cpp \
	../mock/CommonMock.cpp \


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
