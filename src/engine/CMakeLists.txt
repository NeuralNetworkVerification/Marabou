cmake_minimum_required (VERSION 3.12)

add_subdirectory(real)
add_subdirectory(mock)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")


target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (ENGINE_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(engine_add_test name)
    marabou_add_test(${ENGINE_TESTS_DIR}/Test_${name} engine FALSE)
endmacro()

engine_add_test(SmtCore engine)
engine_add_test(BlandsRule engine)
engine_add_test(ConstraintBoundTightener engine)
engine_add_test(ConstraintMatrixAnalyzer engine)
engine_add_test(CostFunctionManager engine)
engine_add_test(DantzigsRule engine)
engine_add_test(DegradationChecker engine)
engine_add_test(Engine engine)
engine_add_test(InputQuery engine)
engine_add_test(LargestIntervalDivider engine)
engine_add_test(MaxConstraint engine)
engine_add_test(NetworkLevelReasoner engine)
engine_add_test(Preprocessor engine)
engine_add_test(ProjectedSteepestEdge engine)
engine_add_test(ReluConstraint engine)
engine_add_test(RowBoundTightener engine)
engine_add_test(SymbolicBoundTightener engine)
engine_add_test(Tableau engine)

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
