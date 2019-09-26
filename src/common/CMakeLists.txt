cmake_minimum_required (VERSION 3.12)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")


target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/tests")

set (COMMON_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(common_add_test name)
    marabou_add_test(${COMMON_TESTS_DIR}/Test_${name} common FALSE)
endmacro()

common_add_test(ConstSimpleData common)
common_add_test(Error common)
common_add_test(File common)
common_add_test(FloatUtils common)
common_add_test(HashMap common)
common_add_test(HashSet common)
common_add_test(HeapData common)
common_add_test(List common)
common_add_test(Map common)
common_add_test(MString common)
common_add_test(MStringf common)
common_add_test(Pair common)
common_add_test(Queue common)
common_add_test(Set common)
common_add_test(Stack common)
common_add_test(Vector common)

if (${BUILD_PYTHON})
target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()




