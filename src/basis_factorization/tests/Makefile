ROOT_DIR = ../../..

TEST_TARGET = basis_factorization.tests

LOCAL_INCLUDES += \
	.. \
	$(COMMON_TEST_DIR) \
	$(CONFIGURATION_DIR) \

TEST_FILES = $(wildcard Test_*.h)

SOURCES += \
	GlobalConfiguration.cpp \
	\
	Errno.cpp \
	Error.cpp \
	FloatUtils.cpp \
	MString.cpp \
	TimeUtils.cpp \
	\
	AlmostDiagonalMatrix.cpp \
	EtaMatrix.cpp \
	ForrestTomlinFactorization.cpp \
	LPElement.cpp \
	LUFactorization.cpp \
	PermutationMatrix.cpp \

all:
	./$(TEST_TARGET)

CFLAGS += \
	-DDEBUG_ON \

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(BASIS_FACTORIZATION_DIR)
vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)
vpath %.cpp $(ENGINE_DIR)
vpath %.cpp $(ENGINE_MOCK_DIR)

#
# Local Variables:
# compile-command: "make -C ../../.. "
# tags-file-name: "../../../TAGS"
# c-basic-offset: 4
# End:
#
