cmake_minimum_required (VERSION 3.12)


set(MPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mps_example)
set(ACAS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/acas_example)
set(BERKELEY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/berkeley_example)

set(PARSERS_OUT_DIR ${CMAKE_BINARY_DIR}/input_parsers)

# set(MPS_PARSER mps.elf)
# set(ACAS_PARSER acas.elf)
# set(BERKELEY_PARSER berkeley.elf)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

# message("${HEADERS}")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

macro(marabou_parser name dir)
    add_executable(${name} "${dir}/main.cpp")
    target_link_libraries(${name} ${MARABOU_LIB})
    target_include_directories(${name} PRIVATE ${LIBS_INCLUDES})
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PARSERS_OUT_DIR})
endmacro()

marabou_parser(${MPS_PARSER} ${MPS_DIR})
marabou_parser(${BERKELEY_PARSER} ${BERKELEY_DIR})
marabou_parser(${ACAS_PARSER} ${ACAS_DIR})
# add_executable(${MPS_PARSER} "${MPS_DIR}/main.cpp")
# target_link_libraries(${MPS_PARSER} ${MARABOU_LIB})
# target_include_directories(${MPS_PARSER} PRIVATE ${LIBS_INCLUDES})
# set_target_properties(${MPS_PARSER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${PARSERS_OUT_DIR})

