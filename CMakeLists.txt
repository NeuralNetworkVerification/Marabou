cmake_minimum_required (VERSION 3.2)
project(Marabou)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_STATIC_MARABOU "Build static Marabou binary" OFF)
option(BUILD_PYTHON "Build Python" ON)
option(FORCE_PYTHON_BUILD "Build python even if there is only python32" OFF)
option(RUN_UNIT_TEST "run unit tests on build" ON)
option(RUN_REGRESS_TEST "run regression tests on build" OFF)
option(RUN_SYSTEM_TEST "run system tests on build" OFF)
option(RUN_MEMORY_TEST "run cxxtest testing with ASAN ON" ON)
option(RUN_PYTHON_TEST "run python API tests if building with python" OFF)


set(DEFAULT_PYTHON_VERSION "3" CACHE STRING "Default Python version 2/3")
set(PYTHON_VERSIONS_SUPPORTED 2 3)
list(FIND PYTHON_VERSIONS_SUPPORTED ${DEFAULT_PYTHON_VERSION} index)
if(index EQUAL -1)
    message(FATAL_ERROR "python version must be one of ${PYTHON_VERSIONS_SUPPORTED}")
endif()

set(MARABOU_LIB MarabouHelper)
set(MARABOU_TEST_LIB MarabouHelperTest)
set(MARABOU_EXE Marabou${CMAKE_EXECUTABLE_SUFFIX})
set(MARABOU_PY MarabouCore)

set(TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(PYTHON_API_DIR "${PROJECT_SOURCE_DIR}/maraboupy")
set(RESOURCES_DIR "${PROJECT_SOURCE_DIR}/resources")
set(REGRESS_DIR "${PROJECT_SOURCE_DIR}/regress")

set(ENGINE_DIR "${SRC_DIR}/engine")
set(PYBIND11_DIR "${TOOLS_DIR}/pybind11-2.3.0")
set(BOOST_DIR "${TOOLS_DIR}/boost_1_68_0")
set(COMMON_DIR "${SRC_DIR}/common")
set(BASIS_DIR "${SRC_DIR}/basis_factorization")


set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(COMMON_REAL "${COMMON_DIR}/real")
set(COMMON_MOCK "${COMMON_DIR}/mock")
file(GLOB SRCS_COMMON_REAL "${COMMON_REAL}/*.cpp")
file(GLOB SRCS_COMMON_MOCK "${COMMON_MOCK}/*.cpp")

set(ENGINE_REAL "${ENGINE_DIR}/real")
set(ENGINE_MOCK "${ENGINE_DIR}/mock")
file(GLOB SRCS_ENGINE_REAL "${ENGINE_REAL}/*.cpp")
file(GLOB SRCS_ENGINE_MOCK "${ENGINE_MOCK}/*.cpp")

set(MPS_PARSER mps)
set(ACAS_PARSER acas)
set(BERKELEY_PARSER berkeley)
set(INPUT_PARSERS_DIR input_parsers)

#-----------------------------------------------------------------------------#
# Determine number of threads available, used to configure (default) parallel
# execution of custom test targets (can be overriden with ARGS=-jN).

include(ProcessorCount)
ProcessorCount(CTEST_NTHREADS)
if(CTEST_NTHREADS EQUAL 0)
  set(CTEST_NTHREADS 1)
endif()


# --------------- set build type ----------------------------
set(BUILD_TYPES Release Debug MinSizeRel RelWithDebInfo)

# Set the default build type to Production
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release CACHE STRING "Options are: Release Debug MinSizeRel RelWithDebInfo" FORCE)
  # Provide drop down menu options in cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES})
endif()

message(STATUS "Building ${CMAKE_BUILD_TYPE} build")

# We build a static library that is the core of the project, the link it to the
# API's (executable and python at the moment)
add_library(${MARABOU_LIB} ${SRCS_COMMON_REAL} ${SRCS_ENGINE_REAL})
add_executable(${MARABOU_EXE} "${ENGINE_DIR}/main.cpp")
set(MARABOU_EXE_PATH "${BIN_DIR}/${MARABOU_EXE}")
add_custom_command(TARGET ${MARABOU_EXE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MARABOU_EXE}> ${MARABOU_EXE_PATH} )

set(MPS_PARSER_PATH "${BIN_DIR}/${MPS_PARSER}")

# Default value false
set(PYTHON32 FALSE)
if(${BUILD_PYTHON})
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
        "import struct; print(struct.calcsize('@P'));"
    RESULT_VARIABLE _PYTHON_SUCCESS
    OUTPUT_VARIABLE PYTHON_SIZEOF_VOID_P
    ERROR_VARIABLE _PYTHON_ERROR_VALUE)
    # message("PYTHON SIZEOF VOID p ${PYTHON_SIZEOF_VOID_P}")
    if (PYTHON_SIZEOF_VOID_P EQUAL 4 AND NOT ${FORCE_PYTHON_BUILD})
        set(PYTHON32 TRUE)
        message(WARNING "Python version is 32bit, please use build_python.sh in
        maraboupy folder")
    endif()
endif()

if (NOT MSVC)
    set(COMPILE_FLAGS  -Wall -Wextra  -Werror -MMD) #-Wno-deprecated
    set(RELEASE_FLAGS ${COMPILE_FLAGS} -O3) #-Wno-deprecated
endif()

if (RUN_MEMORY_TEST)
    if(NOT MSVC)
        set(MEMORY_FLAGS -fsanitize=address -fno-omit-frame-pointer -O1)
    endif()
endif()

add_definitions(-DRESOURCES_DIR="${RESOURCES_DIR}")

if (NOT MSVC)
    set(DEBUG_FLAGS ${COMPILE_FLAGS} ${MEMORY_FLAGS} -g)
    set(CXXTEST_FLAGS ${DEBUG_FLAGS}  -Wno-ignored-qualifiers)
else()
    set(DEBUG_FLAGS ${COMPILE_FLAGS} ${MEMORY_FLAGS})
    add_definitions(-DNOMINMAX) # remove min max macros
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CXXTEST_FLAGS ${CXXTEST_FLAGS} -Wno-terminate)
endif()



# Boost
if (MSVC)
    set(BOOST_ROOT "${BOOST_DIR}/win_installed")
elseif (${CMAKE_SIZEOF_VOID_P} EQUAL 4 AND NOT MSVC)
    set(BOOST_ROOT "${BOOST_DIR}/installed32")
else()
    set(BOOST_ROOT "${BOOST_DIR}/installed")
endif()

if (MSVC)
    set(SCRIPT_EXTENSION bat)
    set(Boost_NAMESPACE libboost)
else()
    set(SCRIPT_EXTENSION sh)
endif()

if (MSVC)
    set(SCRIPT_EXTENSION bat)
    set(Boost_NAMESPACE libboost)
else()
    set(SCRIPT_EXTENSION sh)
endif()
find_package(Boost COMPONENTS program_options)
# Find boost
if (NOT ${Boost_FOUND})
    # bash file that downloads and install boost 1_68_0, the name need to match
    # BOOST_DIR variable
    execute_process(COMMAND ${TOOLS_DIR}/download_boost.${SCRIPT_EXTENSION})
    find_package(Boost REQUIRED COMPONENTS program_options)
endif()
set(LIBS_INCLUDES ${Boost_INCLUDE_DIRS})
set(LIBS ${Boost_LIBRARIES})


# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND LIBS Threads::Threads)

if (BUILD_STATIC_MARABOU)
  # build a static library
  target_link_libraries(${MARABOU_LIB} ${LIBS} -static)
else()
  target_link_libraries(${MARABOU_LIB} ${LIBS})
endif()

target_include_directories(${MARABOU_LIB} PRIVATE ${LIBS_INCLUDES})
target_compile_options(${MARABOU_LIB} PRIVATE ${RELEASE_FLAGS})


# Build marabou executable
target_link_libraries(${MARABOU_EXE} ${MARABOU_LIB})
target_include_directories(${MARABOU_EXE} PRIVATE ${LIBS_INCLUDES})


# Build Python marabou
if (${FORCE_PYTHON_BUILD})
    set(BUILD_PYTHON ON)
else()
    if (${BUILD_PYTHON} AND NOT ${PYTHON32})
        set(BUILD_PYTHON ON)
    else()
        set(BUILD_PYTHON OFF)
    endif()
endif()

if (${BUILD_PYTHON})

    # This is suppose to set the PYTHON_EXECUTABLE variable
    # First try to find the default python version
    find_package(PythonInterp ${DEFAULT_PYTHON_VERSION})
    if (NOT EXISTS ${PYTHON_EXECUTABLE})
        # If the default didn't work just find any python version
        find_package(PythonInterp REQUIRED)
    endif()

    if (NOT EXISTS ${PYBIND11_DIR})
        message("didnt find pybind, getting it")
	execute_process(COMMAND ${TOOLS_DIR}/download_pybind11.${SCRIPT_EXTENSION})
    endif()
    add_subdirectory(${PYBIND11_DIR})
    pybind11_add_module(${MARABOU_PY} ${PYTHON_API_DIR}/MarabouCore.cpp)

    target_link_libraries(${MARABOU_PY} PRIVATE ${MARABOU_LIB})
    target_include_directories(${MARABOU_PY} PRIVATE ${LIBS_INCLUDES})

    set_target_properties(${MARABOU_PY} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PYTHON_API_DIR})
    if(NOT MSVC)
        target_compile_options(${MARABOU_LIB} PRIVATE -fPIC ${RELEASE_FLAGS})
    endif()
endif()



add_library(${MARABOU_TEST_LIB})
set (TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests")
file(MAKE_DIRECTORY ${TEST_DIR})

set(CMAKE_PREFIX_PATH "${TOOLS_DIR}/cxxtest")
set(CXXTEST_USE_PYTHON FALSE)
find_package(CxxTest)
if(CXXTEST_FOUND)
    include_directories(${CXXTEST_INCLUDE_DIR})
    enable_testing()
endif()

target_link_libraries(${MARABOU_TEST_LIB} ${LIBS})
target_include_directories(${MARABOU_TEST_LIB} PRIVATE ${LIBS_INCLUDES})
target_compile_options(${MARABOU_TEST_LIB} PRIVATE ${CXXTEST_FLAGS})



add_custom_target(build-tests ALL)

add_custom_target(check
      COMMAND ctest --output-on-failure -j${CTEST_NTHREADS} $$ARGS
      DEPENDS build-tests build_input_parsers ${MARABOU_EXE})


# Decide which tests to run and execute
set(TESTS_TO_RUN "")
# ctest uses regex, so create the string to look: (unit|system) ...
macro(append_tests_to_run new_val)
    if ("${TESTS_TO_RUN}" STREQUAL "")
        set(TESTS_TO_RUN ${new_val})
    else()
        set(TESTS_TO_RUN "${TESTS_TO_RUN}|${new_val}")
    endif()
endmacro()

if (${RUN_UNIT_TEST})
    append_tests_to_run("unit")
endif()
if (${RUN_REGRESS_TEST})
    append_tests_to_run("regress[0-5]")
endif()
if (${RUN_SYSTEM_TEST})
    append_tests_to_run("system")
endif()
if (NOT ${TESTS_TO_RUN} STREQUAL "")
    # make ctest verbose
    set(CTEST_OUTPUT_ON_FAILURE 1)
    add_custom_command(
        TARGET build-tests
        POST_BUILD
        COMMAND ctest --output-on-failure  -L "\"(${TESTS_TO_RUN})\"" -j${CTEST_NTHREADS} $$ARGS
    )
endif()

if (${BUILD_PYTHON} AND ${RUN_PYTHON_TEST})
    if (MSVC)
        add_custom_command(
            TARGET build-tests
            POST_BUILD
            COMMAND cp ${PYTHON_API_DIR}/Release/* ${PYTHON_API_DIR}
        )
    endif()

    add_custom_command(
        TARGET build-tests
        POST_BUILD
        COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PYTHON_API_DIR}/test
    )
endif()    

# Add the input parsers
add_custom_target(build_input_parsers)
add_dependencies(build_input_parsers ${MPS_PARSER} ${ACAS_PARSER}
    ${BERKELEY_PARSER})

add_subdirectory(${SRC_DIR})
add_subdirectory(${TOOLS_DIR})
add_subdirectory(${REGRESS_DIR})

execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Marabou Version
set(MARABOU_VERSION 1.0.+)
add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")
add_definitions("-DGIT_BRANCH=\"${GIT_BRANCH}\"")
add_definitions("-DMARABOU_VERSION=\"${MARABOU_VERSION}\"")
