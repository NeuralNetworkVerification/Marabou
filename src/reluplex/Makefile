ROOT_DIR = ..

SUBDIRS += \
	tests \
	regress \

LOCAL_INCLUDES += \
	$(CONFIGURATION_DIR) \

LINK_FLAGS += \

LOCAL_LIBRARIES += \

SOURCES += \
	GlobalConfiguration.cpp \
	\
	Errno.cpp \
	Error.cpp \
	FloatUtils.cpp \
	\
	main.cpp \

TARGET = run_reluplex.elf

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)

#
# Local Variables:
# compile-command: "make -C .. "
# tags-file-name: "../TAGS"
# c-basic-offset: 4
# End:
#
