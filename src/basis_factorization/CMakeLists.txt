cmake_minimum_required (VERSION 3.12)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
 
set (BASIS_FACTORIZATION_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(basis_factorization_add_test name)
    marabou_add_test(${BASIS_FACTORIZATION_TESTS_DIR}/Test_${name} basis_factorization TRUE)
endmacro()

basis_factorization_add_test(CSRMatrix basis_factorization)
basis_factorization_add_test(CompareFactorizations basis_factorization)
basis_factorization_add_test(ForrestTomlinFactorization basis_factorization)
basis_factorization_add_test(LUFactorization basis_factorization)
basis_factorization_add_test(LUFactors basis_factorization)
basis_factorization_add_test(PermutationMatrix basis_factorization)
basis_factorization_add_test(SparseFTFactorization basis_factorization)
basis_factorization_add_test(SparseGaussianEliminator basis_factorization)
basis_factorization_add_test(SparseLUFactorization basis_factorization)
basis_factorization_add_test(SparseLUFactors basis_factorization)
basis_factorization_add_test(SparseUnsortedArray basis_factorization)
basis_factorization_add_test(SparseUnsortedArrays basis_factorization)
basis_factorization_add_test(SparseUnsortedList basis_factorization)
basis_factorization_add_test(SparseUnsortedLists basis_factorization)

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()




