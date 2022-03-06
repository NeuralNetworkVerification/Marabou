file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (PROOF_PRODUCTION_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(proof_production_add_unit_test name)
    set(USE_MOCK_COMMON TRUE)
    set(USE_MOCK_ENGINE TRUE)
    marabou_add_test(${PROOF_PRODUCTION_TESTS_DIR}/Test_${name} proof_production USE_MOCK_COMMON USE_MOCK_ENGINE "unit")
endmacro()

proof_production_add_unit_test(BoundExplainer)
proof_production_add_unit_test(SmtLibWriter)
proof_production_add_unit_test(UnsatCertificateNode)
proof_production_add_unit_test(UnsatCertificateUtils)

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
