ROOT_DIR = ../../..

include $(ROOT_DIR)/Places.mk

TEST_TARGET = basis_factorization.tests

LOCAL_INCLUDES += \
	.. \
	$(COMMON_TEST_DIR) \
	$(CONFIGURATION_DIR) \

TEST_FILES = $(wildcard Test_*.h)

include $(BASIS_FACTORIZATION_DIR)/Sources.mk
include $(COMMON_DIR)/Sources.mk
include $(COMMON_REAL_DIR)/Sources.mk
include $(CONFIGURATION_DIR)/Sources.mk

vpath %.cpp $(BASIS_FACTORIZATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)
vpath %.cpp $(CONFIGURATION_DIR)

SOURCES += \
	\

all:
	./$(TEST_TARGET)

CFLAGS += \
	-DDEBUG_ON \

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C ../../.. "
# tags-file-name: "../../../TAGS"
# c-basic-offset: 4
# End:
#
