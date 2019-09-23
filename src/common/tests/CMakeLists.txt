cmake_minimum_required (VERSION 3.10)

file(GLOB TESTS "Test_*.h")

# message("in src/common/tests")

if (RUN_TEST) 
    set(COMMON_UNIT_TEST unittest_common)
    CXXTEST_ADD_TEST(${COMMON_UNIT_TEST} common.cc ${TESTS} ${SRCS_COMMON_MOCK}) 
    target_include_directories(${COMMON_UNIT_TEST} PUBLIC "../${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${COMMON_UNIT_TEST}  ${MARABOU_TEST_LIB} ${CXXTEST_FLAGS})
    target_compile_options(${COMMON_UNIT_TEST} PRIVATE ${CXXTEST_FLAGS})

    # Add the exit 0 so the executable will always be generated, otherwise the
    # build fails (exit is not zero) and cmake removes everything
    add_custom_command(TARGET ${COMMON_UNIT_TEST} 
                   POST_BUILD
                   COMMAND ctest -C $<CONFIGURATION> --output-on-failure || exit
                   0)

    add_custom_command(TARGET ${COMMON_UNIT_TEST} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${COMMON_UNIT_TEST}> ${TEST_DIR}/${COMMON_UNIT_TEST})

endif()
