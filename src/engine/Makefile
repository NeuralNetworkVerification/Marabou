ROOT_DIR = ../..

include $(ROOT_DIR)/Places.mk

SUBDIRS += \
	tests \

LOCAL_INCLUDES += \
	.. \
	$(BASIS_FACTORIZATION_DIR) \
	$(CONFIGURATION_DIR) \
	$(INPUT_PARSER_DIR) \
	$(BOOST_INCLUDES)

LINK_FLAGS += \

LIBRARY_DIR += \
	$(BOOST_LIBS) \

LIBRARIES += \
	:libboost_program_options.a \

CFLAGS = \
	-MMD \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-deprecated \
	-std=c++0x \
	-O3 \

include $(BASIS_FACTORIZATION_DIR)/Sources.mk
include $(COMMON_DIR)/Sources.mk
include $(COMMON_REAL_DIR)/Sources.mk
include $(CONFIGURATION_DIR)/Sources.mk
include $(ENGINE_DIR)/Sources.mk
include $(ENGINE_REAL_DIR)/Sources.mk
include $(INPUT_PARSER_DIR)/Sources.mk

vpath %.cpp $(BASIS_FACTORIZATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)
vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(ENGINE_REAL_DIR)
vpath %.cpp $(INPUT_PARSER_DIR)

SOURCES += \
	Marabou.cpp \
	Options.cpp \
	main.cpp \

TARGET = marabou.elf

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
