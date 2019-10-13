cmake_minimum_required (VERSION 3.12)
project(Marabou)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_PYTHON "Build Python" ON)
option(FORCE_PYTHON_BUILD "Build python even if there is only python32" OFF)
option(RUN_UNIT_TEST "run cxxtest unit testing" ON)
option(RUN_MEMORY_TEST "run cxxtest unit testing with ASAN" ON)
option(RUN_REGRESS_TEST "run regression tests" OFF)

set(MARABOU_LIB MarabouHelper)
set(MARABOU_TEST_LIB MarabouHelperTest)
set(MARABOU_EXE Marabou)
set(MARABOU_REGRESS Regression)
set(MARABOU_PY MarabouCore)

set(TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(PYTHON_API_DIR "${PROJECT_SOURCE_DIR}/maraboupy")
set(REGRESS_DIR "${PROJECT_SOURCE_DIR}/regress")

set(ENGINE_DIR "${SRC_DIR}/engine")
set(PYBIND11_DIR "${TOOLS_DIR}/pybind11-2.3.0")
set(BOOST_DIR "${TOOLS_DIR}/boost_1_68_0")
set(COMMON_DIR "${SRC_DIR}/common")
set(BASIS_DIR "${SRC_DIR}/basis_factorization")
set(ACAS_REGRESS_NET "${REGRESS_DIR}/acas_nnet")


set(COMMON_REAL "${COMMON_DIR}/real")
set(COMMON_MOCK "${COMMON_DIR}/mock")
file(GLOB SRCS_COMMON_REAL "${COMMON_REAL}/*.cpp")
file(GLOB SRCS_COMMON_MOCK "${COMMON_MOCK}/*.cpp")

# We build a static library that is the core of the project, the link it to the
# API's (executable and python at the moment)
add_library(${MARABOU_LIB} ${SRCS_COMMON_REAL} )
add_executable(${MARABOU_EXE} "${ENGINE_DIR}/main.cpp")
add_executable(${MARABOU_REGRESS} "${REGRESS_DIR}/main_regress.cpp")


# Build regress 
target_link_libraries(${MARABOU_REGRESS} PRIVATE ${MARABOU_LIB})

target_include_directories(${MARABOU_REGRESS} PRIVATE ${LIBS_INCLUDES})
if (RUN_REGRESS_TEST)
    add_custom_command(TARGET ${MARABOU_REGRESS} 
                   POST_BUILD
                   COMMAND Regress --output-on-failure || exit
                   0)
endif()

# Default value false
set(PYTHON32 FALSE)
if(${BUILD_PYTHON})
    execute_process(COMMAND "${Python_EXECUTABLE}" "-c"
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

if(MSVC)
    # TODO: TEST this
    set(COMPILE_FLAGS)
else()
    set(COMPILE_FLAGS  -Wall -Wextra  -Werror -MMD) #-Wno-deprecated
    set(RELEASE_FLAGS -O3) #-Wno-deprecated
endif()

if (RUN_MEMORY_TEST)
    if(MSVC)
        set(MEMORY_FLAGS)
    else()
        set(MEMORY_FLAGS -fsanitize=address -fno-omit-frame-pointer -O1)
    endif()
endif()

if(MSVC)
    set(CXXTEST_FLAGS ${DEBUG_FLAGS} )
    set(DEBUG_FLAGS ${COMPILE_FLAGS} ${MEMORY_FLAGS})
else() 
    set(DEBUG_FLAGS ${COMPILE_FLAGS} ${MEMORY_FLAGS} -g -DDEBUG_ON) 
    set(CXXTEST_FLAGS ${DEBUG_FLAGS}  -Wno-ignored-qualifiers)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CXXTEST_FLAGS -Wno-terminate)
endif()
if(MSVC)
    set(RELEASE_FLAGS  ${COMPILE_FLAGS} )
else()
    set(RELEASE_FLAGS  ${COMPILE_FLAGS} -O3)
endif()


# Boost
if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(BOOST_ROOT "${BOOST_DIR}/installed32")
    # set(BOOST_INCLUDES "${BOOST_DIR}/installed32/include")
else()
    set(BOOST_ROOT "${BOOST_DIR}/installed")
    # set(BOOST_INCLUDES "${BOOST_DIR}/installed/include")
endif()
set(Boost_NAMESPACE libboost)
find_package(Boost COMPONENTS program_options)
# Find boost
if (NOT ${Boost_FOUND})
    # bash file that downloads and install boost 1_68_0, the name need to match
    # BOOST_DIR variable
    execute_process(COMMAND ${TOOLS_DIR}/download_boost.sh)
    find_package(Boost REQUIRED COMPONENTS program_options)
endif()
set(LIBS_INCLUDES ${Boost_INCLUDE_DIRS})
set(LIBS ${Boost_LIBRARIES})


# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND LIBS Threads::Threads)

# build a static library     
target_link_libraries(${MARABOU_LIB} ${LIBS})
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

# message("BUILD_PYTHON: ${BUILD_PYTHON}")
if (${BUILD_PYTHON})
    # This is suppose to set the PYTHON_EXECUTABLE variable
    find_package(Python REQUIRED)
    
    if (NOT EXISTS ${PYBIND11_DIR})
        message("didnt find pybind, getting it")
        if(MSVC)
            execute_process(COMMAND ${TOOLS_DIR}/download_pybind11.bat)
        else()
            execute_process(COMMAND ${TOOLS_DIR}/download_pybind11.sh)
        endif()
    endif()
    add_subdirectory(${PYBIND11_DIR})
    pybind11_add_module(${MARABOU_PY} ${PYTHON_API_DIR}/MarabouCore.cpp)

    target_link_libraries(${MARABOU_PY} PRIVATE ${MARABOU_LIB})
    target_include_directories(${MARABOU_PY} PRIVATE ${LIBS_INCLUDES})

    set_target_properties(${MARABOU_PY} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PYTHON_API_DIR})
    if(MSVC)
        target_compile_options(${MARABOU_LIB} PRIVATE ${RELEASE_FLAGS}) 
    else()
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
      DEPENDS build-tests)

if (${RUN_UNIT_TEST})
    # make ctest verbose 
    set(CTEST_OUTPUT_ON_FAILURE 1)
    add_custom_command(
        TARGET build-tests
        POST_BUILD
        COMMAND ctest --output-on-failure  -L unit
    )

endif()

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

