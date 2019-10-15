file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
 
set (BASIS_FACTORIZATION_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(basis_factorization_add_unit_test name)
    marabou_add_test(${BASIS_FACTORIZATION_TESTS_DIR}/Test_${name} basis_factorization TRUE "unit")
endmacro()

basis_factorization_add_unit_test(CSRMatrix)
basis_factorization_add_unit_test(CompareFactorizations)
basis_factorization_add_unit_test(ForrestTomlinFactorization)
basis_factorization_add_unit_test(LUFactorization)
basis_factorization_add_unit_test(LUFactors)
basis_factorization_add_unit_test(PermutationMatrix)
basis_factorization_add_unit_test(SparseFTFactorization)
basis_factorization_add_unit_test(SparseGaussianEliminator)
basis_factorization_add_unit_test(SparseLUFactorization)
basis_factorization_add_unit_test(SparseLUFactors)
basis_factorization_add_unit_test(SparseUnsortedArray)
basis_factorization_add_unit_test(SparseUnsortedArrays)
basis_factorization_add_unit_test(SparseUnsortedList)
basis_factorization_add_unit_test(SparseUnsortedLists)

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()




