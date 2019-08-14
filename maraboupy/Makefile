ROOT_DIR = ../

include ../Places.mk

SUBDIRS += \

PYBIND11_INCLUDES = $(shell python3 -m pybind11 --includes)

LOCAL_INCLUDES += \
	.. \
	$(BASIS_FACTORIZATION_DIR) \
	$(CONFIGURATION_DIR) \
	$(ENGINE_DIR) \
	$(INPUT_PARSER_DIR) \
	$(PYBIND11_INCLUDES) \

LINK_FLAGS += \

LOCAL_LIBRARIES += \

CFLAGS += \

SUFFIX = $(shell python3-config --extension-suffix)
API_NAME = $(addprefix MarabouCore, $(SUFFIX))

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
vpath %.cpp $(ENGINE_DIR)
vpath %.cpp $(ENGINE_REAL_DIR)
vpath %.cpp $(INPUT_PARSER_DIR)

SOURCES += \
	\
	MarabouCore.cpp \

TARGET = $(API_NAME)

include ../Rules.mk

COMPILE = c++ -std=c++11 -O3 -fPIC $(PYBIND11_INCLUDES)
LINK 	= c++ -std=c++11 -O3 -fPIC -shared -Wl,-undefined,dynamic_lookup $(PYBIND11_INCLUDES)

$(TARGET): $(OBJECTS)
	@echo "LD\t" $@
	@$(LINK) $(LINK_FLAGS) -o $@ $^ $(addprefix -l, $(SYSTEM_LIBRARIES)) $(addprefix -l, $(LOCAL_LIBRARIES))

#
# Local Variables:
# compile-command: "make -C .. "
# tags-file-name: "../TAGS"
# c-basic-offset: 4
# End:
#
