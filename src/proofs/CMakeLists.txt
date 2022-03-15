file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (PROOFS_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(proofs_add_unit_test name)
    set(USE_MOCK_COMMON TRUE)
    set(USE_MOCK_ENGINE TRUE)
    marabou_add_test(${PROOFS_TESTS_DIR}/Test_${name} proofs USE_MOCK_COMMON USE_MOCK_ENGINE "unit")
endmacro()

proofs_add_unit_test(BoundExplainer)
proofs_add_unit_test(Checker)
proofs_add_unit_test(SmtLibWriter)
proofs_add_unit_test(UnsatCertificateNode)
proofs_add_unit_test(UnsatCertificateUtils)

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
