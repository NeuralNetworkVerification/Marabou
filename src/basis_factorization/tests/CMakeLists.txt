cmake_minimum_required (VERSION 3.12)

file(GLOB TESTS "Test_*.h")

# message("in src/basis_factorization/tests")

if (RUN_TEST) 
    CXXTEST_ADD_TEST(unittest_basis_factorization basis.cc ${TESTS} ${SRCS_COMMON_REAL}) 
    target_include_directories(unittest_basis_factorization PRIVATE "../${CMAKE_CURRENT_SOURCE_DIR}")
        # ${COMMON_DIR}/tests)
    target_link_libraries(unittest_basis_factorization PRIVATE ${MARABOU_TEST_LIB} ${MEMORY_FLAGS})
    # Add the exit 0 so the executable will always be generated, otherwise the
    # build fails (exit is not zero) and cmake removes everything
    add_custom_command(TARGET unittest_basis_factorization
                   POST_BUILD
                   COMMAND ctest -C $<CONFIGURATION> --output-on-failure || exit
                   0)

    add_custom_command(TARGET unittest_basis_factorization POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:unittest_basis_factorization> ${TEST_DIR}/unittest_basis_factorization)
endif()
