ROOT_DIR = ../../..

include $(ROOT_DIR)/Places.mk

TEST_TARGET = CommonTests.tests

LOCAL_INCLUDES += \
	.. \
	$(CONFIGURATION_DIR) \

TEST_FILES = $(wildcard Test_*.h)

include $(COMMON_DIR)/Sources.mk
include $(CONFIGURATION_DIR)/Sources.mk

vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(CONFIGURATION_DIR)

SOURCES += \
	../mock/Errno.cpp \
	../mock/CommonMock.cpp \

all:
	./$(TEST_TARGET)

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C ../../.. "
# tags-file-name: "../../../TAGS"
# c-basic-offset: 4
# End:
#
