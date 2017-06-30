ROOT_DIR = ../..

SUBDIRS += \

LOCAL_INCLUDES += \
	$(CONFIGURATION_DIR) \
	$(RELUPLEX_DIR) \

LINK_FLAGS += \

LOCAL_LIBRARIES += \

CFLAGS += \
	-DDEBUG_ON \

SOURCES += \
	GlobalConfiguration.cpp \
	\
	Errno.cpp \
	Error.cpp \
	FloatUtils.cpp \
	String.cpp \
	\
	BasisFactorization.cpp \
	BlandsRule.cpp \
	DantzigsRule.cpp \
	Engine.cpp \
	Equation.cpp \
	EtaMatrix.cpp \
	FreshVariables.cpp \
	InputQuery.cpp \
	PiecewiseLinearCaseSplit.cpp \
	ReluConstraint.cpp \
	Tableau.cpp \
	TableauFactory.cpp \
	TableauRow.cpp \
	\
	main_regress.cpp \

all:
	./$(TARGET)

TARGET = run_regress.elf

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)
vpath %.cpp $(RELUPLEX_DIR)
vpath %.cpp $(RELUPLEX_REAL_DIR)

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
