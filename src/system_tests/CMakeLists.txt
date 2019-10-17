set (SYSTEM_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
macro(add_system_test name)
    marabou_add_test(${SYSTEM_TESTS_DIR}/Test_${name} lp TRUE TRUE "system")
endmacro()

add_system_test(lp)
add_system_test(relu)
add_system_test(max)
add_system_test(mps)
add_system_test(acas)
file(COPY "${RESOURCES_DIR}/mps/lp_feasible_1.mps" DESTINATION ${CMAKE_BINARY_DIR}) 
file(COPY "${RESOURCES_DIR}/mps/lp_infeasible_1.mps" DESTINATION ${CMAKE_BINARY_DIR}) 
file(COPY ${ACAS_REGRESS_NET} DESTINATION ${CMAKE_BINARY_DIR}) 

add_custom_target(systemtests
  COMMAND ctest --output-on-failure -L "system" -j${CTEST_NTHREADS} $$ARGS
  DEPENDS build-tests)

