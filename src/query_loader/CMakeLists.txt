file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (QUERY_LOADER_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(query_loader_add_unit_test name)
    marabou_add_test(${QUERY_LOADER_TESTS_DIR}/Test_${name} query_loader FALSE FALSE "unit")
endmacro()

query_loader_add_unit_test(QueryLoader)

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
