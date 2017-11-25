ROOT_DIR = ../../..

SUBDIRS += \

LOCAL_INCLUDES += \
    .. \
    $(CONFIGURATION_DIR) \
    $(ENGINE_DIR) \
    $(TENSORFLOW_DIR)/include_tf \

LINK_FLAGS += \
    -Llib_tf \

LOCAL_LIBRARIES += \
    tensorflow_cc \

CFLAGS += \
    -Wno-unused-parameter \
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
    \
    main.cpp \
    ../TfParser.cpp \
    ../TensorflowNeuralNetwork.cpp \
    ../TfNnet.cpp \

TARGET = tensorflow_example.elf

include $(ROOT_DIR)/Rules.mk

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
