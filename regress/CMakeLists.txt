cmake_minimum_required (VERSION 3.5)

file(GLOB SRCS "*.cpp")
# file(GLOB HEADERS "*.h")

target_sources(${MARABOU_REGRESS} PRIVATE ${SRCS})
target_include_directories(${MARABOU_REGRESS} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# The Regression exe is dependent on those files, copy them to the build dir
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)
file(COPY ${ACAS_REGRESS_NET} DESTINATION ${CMAKE_BINARY_DIR}) 
file(COPY "${REGRESS_DIR}/lp_feasible_1.mps" DESTINATION ${CMAKE_BINARY_DIR}) 
file(COPY "${REGRESS_DIR}/lp_infeasible_1.mps" DESTINATION ${CMAKE_BINARY_DIR}) 
