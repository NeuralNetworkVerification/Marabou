cmake_minimum_required (VERSION 3.12)

set (SYSTEM_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
macro(add_system_test name)
    marabou_add_test(${SYSTEM_TESTS_DIR}/Test_${name} lp TRUE TRUE "system")
endmacro()

add_system_test(lp)
