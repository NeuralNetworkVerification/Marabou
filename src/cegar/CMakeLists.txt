file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")


target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (CEGAR_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(cegar_add_unit_test name)
    set(USE_REAK_COMMON TRUE)
    set(USE_REAL_ENGINE TRUE)
    marabou_add_test(${CEGAR_TESTS_DIR}/Test_${name} cegar USE_REAL_COMMON
        USE_REAL_ENGINE "unit")
endmacro()

cegar_add_unit_test(IncrementalLinearization)

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
