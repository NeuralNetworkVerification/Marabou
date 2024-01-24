
###############
## Inclusion ##
###############
# Includes the parser code in the main Marabou executable.

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

##################
## Parser tests ##
##################

set (PARSER_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

macro(add_parser_unit_test name)
    set(USE_REAL_COMMON TRUE)
    set(USE_REAL_ENGINE TRUE)
    marabou_add_test(
        ${PARSER_TESTS_DIR}/Test_${name}
        parser
        USE_REAL_COMMON
        USE_REAL_ENGINE
        "unit"
    )
endmacro()

add_parser_unit_test(OnnxParser)
add_parser_unit_test(VnnLibParser)

########################
## Parser executables ##
########################
# We also build executables for some of the individual parsers. This is a little
# weird to me, as the MPS parser seems to be an executable that takes arbitrary arguments
# and runs it. Whereas the Berkeley and ACAS executables are just tests (that never seem
# to be run).

## Set source and output location variables
set(MPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mps_example)
set(ACAS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/acas_example)
set(BERKELEY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/berkeley_example)
set(PARSERS_OUT_DIR ${CMAKE_BINARY_DIR}/input_parsers)

# Macro for building the parser executables
macro(marabou_parser name dir)
    add_executable(${name} "${dir}/main.cpp")
    target_link_libraries(${name} ${MARABOU_LIB})
    target_include_directories(${name} PRIVATE ${LIBS_INCLUDES})
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PARSERS_OUT_DIR})
endmacro()

# Register the parsers
marabou_parser(${BERKELEY_PARSER} ${BERKELEY_DIR})
marabou_parser(${ACAS_PARSER} ${ACAS_DIR})
marabou_parser(${MPS_PARSER} ${MPS_DIR})

# Copy the MPS parser executable to the requested path (used by Python code)
add_custom_command(TARGET ${MPS_PARSER} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MPS_PARSER}> ${MPS_PARSER_PATH} )

