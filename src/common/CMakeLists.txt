file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")


target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/tests")

set (COMMON_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(common_add_unit_test name)
    marabou_add_test(${COMMON_TESTS_DIR}/Test_${name} common FALSE "unit")
endmacro()

common_add_unit_test(ConstSimpleData)
common_add_unit_test(Error)
common_add_unit_test(File)
common_add_unit_test(FloatUtils)
common_add_unit_test(HashMap)
common_add_unit_test(HashSet)
common_add_unit_test(HeapData)
common_add_unit_test(List)
common_add_unit_test(Map)
common_add_unit_test(MString)
common_add_unit_test(MStringf)
common_add_unit_test(Pair)
common_add_unit_test(Queue)
common_add_unit_test(Set)
common_add_unit_test(Stack)
common_add_unit_test(Vector)

if (${BUILD_PYTHON})
target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()




