ROOT_DIR = ..

SUBDIRS += \

LOCAL_INCLUDES += \
	$(BASIS_FACTORIZATION_DIR) \
	$(CONFIGURATION_DIR) \
	$(ENGINE_DIR) \
	$(INPUT_PARSER_DIR) \

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
	AcasNeuralNetwork.cpp \
	AcasNnet.cpp \
	AcasParser.cpp \
	BasisFactorizationFactory.cpp \
	BlandsRule.cpp \
	ConstraintMatrixAnalyzer.cpp \
	CostFunctionManager.cpp \
	CostFunctionManagerFactory.cpp \
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
	LUFactorization.cpp \
	MaxConstraint.cpp \
	PiecewiseLinearCaseSplit.cpp \
	PiecewiseLinearConstraint.cpp \
	PrecisionRestorer.cpp \
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
	\
	main_regress.cpp \

all:
	./run_regress.elf

TARGET = run_regress.elf

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(BASIS_FACTORIZATION_DIR)
vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)
vpath %.cpp $(ENGINE_DIR)
vpath %.cpp $(ENGINE_REAL_DIR)
vpath %.cpp $(INPUT_PARSER_DIR)

#
# Local Variables:
# compile-command: "make -C .. "
# tags-file-name: "../TAGS"
# c-basic-offset: 4
# End:
#
