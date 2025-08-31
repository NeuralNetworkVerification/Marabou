file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (CDCL_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(cdcl_add_unit_test name)
    set(USE_MOCK_COMMON TRUE)
    set(USE_MOCK_ENGINE TRUE)
    marabou_add_test(${CDCL_TESTS_DIR}/Test_${name} cdcl USE_MOCK_COMMON USE_MOCK_ENGINE "unit")
endmacro()

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
