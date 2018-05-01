ROOT_DIR = ../

SUBDIRS += \

PYBIND11_INCLUDES = $(shell python3 -m pybind11 --includes)

LOCAL_INCLUDES += \
	.. \
	$(BASIS_FACTORIZATION_DIR) \
	$(CONFIGURATION_DIR) \
	$(ENGINE_DIR) \
	$(PYBIND11_INCLUDES) \

LINK_FLAGS += \

LOCAL_LIBRARIES += \

CFLAGS += \
	-DDEBUG_ON \

SUFFIX = $(shell python3-config --extension-suffix)
API_NAME = $(addprefix MarabouCore, $(SUFFIX))

SOURCES += \
	GlobalConfiguration.cpp \
	\
	Errno.cpp \
	Error.cpp \
	FloatUtils.cpp \
	MString.cpp \
	TimeUtils.cpp \
	\
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
	ForrestTomlinFactorization.cpp \
	FreshVariables.cpp \
	InputQuery.cpp \
	LPElement.cpp \
	LUFactorization.cpp \
	MarabouCore.cpp \
	MaxConstraint.cpp \
	PermutationMatrix.cpp \
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
	

TARGET = $(API_NAME)

include ../Rules.mk

COMPILE = c++ -std=c++11 -O3 -fPIC $(PYBIND11_INCLUDES)
LINK 	= c++ -std=c++11 -O3 -fPIC -shared -Wl,-undefined,dynamic_lookup $(PYBIND11_INCLUDES)

$(TARGET): $(OBJECTS)
	@echo "LD\t" $@
	@$(LINK) $(LINK_FLAGS) -o $@ $^ $(addprefix -l, $(SYSTEM_LIBRARIES)) $(addprefix -l, $(LOCAL_LIBRARIES))

vpath %.cpp $(BASIS_FACTORIZATION_DIR)
vpath %.cpp $(CONFIGURATION_DIR)
vpath %.cpp $(ENGINE_DIR)
vpath %.cpp $(ENGINE_REAL_DIR)
vpath %.cpp $(COMMON_DIR)
vpath %.cpp $(COMMON_REAL_DIR)

#
# Local Variables:
# compile-command: "make -C ../../.. "
# tags-file-name: "../../../TAGS"
# c-basic-offset: 4
# End:
#
