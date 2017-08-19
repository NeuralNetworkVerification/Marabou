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
	MString.cpp \
	TimeUtils.cpp \
	\
	BasisFactorization.cpp \
	BlandsRule.cpp \
	BoundTightener.cpp \
	DantzigsRule.cpp \
	DegradationChecker.cpp \
	Engine.cpp \
	EngineState.cpp \
	EntrySelectionStrategy.cpp \
	Equation.cpp \
	EtaMatrix.cpp \
	FreshVariables.cpp \
	InputQuery.cpp \
	LPElement.cpp \
	MaxConstraint.cpp \
	NestedDantzigsRule.cpp \
	PiecewiseLinearCaseSplit.cpp \
	PiecewiseLinearConstraint.cpp \
	Preprocessor.cpp \
	ProjectedSteepestEdge.cpp \
	ReluConstraint.cpp \
	SmtCore.cpp \
	Statistics.cpp \
	SteepestEdge.cpp \
	Tableau.cpp \
	TableauFactory.cpp \
	TableauRow.cpp \
	TableauState.cpp \
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
