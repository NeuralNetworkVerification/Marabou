cmake_minimum_required (VERSION 3.12)

file(GLOB TESTS "Test_*.h")


if (RUN_TEST) 
    CXXTEST_ADD_TEST(unittest_engine engine.cc ${TESTS} ${SRCS_COMMON_MOCK} ) 

    target_include_directories(unittest_engine  PRIVATE "../${CMAKE_CURRENT_SOURCE_DIR}" ${LIBS_INCLUDES} ${COMMON_DIR}/tests)
    target_link_libraries(unittest_engine PRIVATE ${MARABOU_TEST_LIB} ${DEBUG_FLAGS})

    # Add the exit 0 so the executable will always be generated, otherwise the
    # build fails (exit is not zero) and cmake removes everything
    add_custom_command(TARGET unittest_engine
                   POST_BUILD
                   COMMAND ctest -C $<CONFIGURATION> --output-on-failure || exit
                   0)

    add_custom_command(TARGET unittest_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:unittest_engine> ${TEST_DIR}/unittest_engine)
endif()
