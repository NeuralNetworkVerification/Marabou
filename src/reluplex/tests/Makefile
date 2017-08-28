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
	Errno.cpp \
	Error.cpp \
	FloatUtils.cpp \
	MString.cpp \
	TimeUtils.cpp \
	\
	BasisFactorization.cpp \
	BlandsRule.cpp \
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
	ProjectedSteepestEdgeFactory.cpp \
	ReluConstraint.cpp \
	RowBoundTightener.cpp \
	RowBoundTightenerFactory.cpp \
	SmtCore.cpp \
	Statistics.cpp \
	Tableau.cpp \
	TableauFactory.cpp \
	TableauRow.cpp \
	TableauState.cpp \

all:
	./$(TEST_TARGET)

CFLAGS += \
	-DDEBUG_ON \

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)
vpath %.cpp $(RELUPLEX_DIR)
vpath %.cpp $(RELUPLEX_MOCK_DIR)

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
