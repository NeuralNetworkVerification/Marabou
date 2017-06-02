ROOT_DIR = ../..

TEST_TARGET = Reluplex.tests

LOCAL_INCLUDES += \
	.. \
	$(COMMON_TEST_DIR) \
	$(CONFIGURATION_DIR) \

TEST_FILES = $(wildcard Test_*.h)

SOURCES += \
	GlobalConfiguration.cpp \
	\
	Error.cpp \
	Errno.cpp \
	FloatUtils.cpp \
	\
	../Tableau.cpp \

all:
	./$(TEST_TARGET)

CFLAGS += \
	-DDEBUG_ON \

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
