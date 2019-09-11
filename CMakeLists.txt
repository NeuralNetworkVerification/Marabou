cmake_minimum_required (VERSION 3.12)
project(Marabou)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# TODO: Should default be ON or OFF ?
option(BUILD_PYTHON "Build Python" ON)
option(RUN_TEST "run cxxtest unit testing" ON)
option(RUN_MEMORY_TEST "run cxxtest unit testing with ASAN" OFF)

set(MARABOU_LIB MarabouHelper)
set(MARABOU_TEST_LIB MarabouHelperTest)
set(MARABOU_EXE Marabou)
set(MARABOU_PY MarabouCore)

set(TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(PYTHON_API_DIR "${PROJECT_SOURCE_DIR}/maraboupy")

set(ENGINE_DIR "${SRC_DIR}/engine")
set(PYBIND11_DIR "${TOOLS_DIR}/pybind11-2.3.0")
set(BOOST_DIR "${TOOLS_DIR}/boost_1_68_0")
set(COMMON_DIR "${SRC_DIR}/common")
set(BASIS_DIR "${SRC_DIR}/basis_factorization")

set(COMMON_REAL "${COMMON_DIR}/real")
set(COMMON_MOCK "${COMMON_DIR}/mock")
file(GLOB SRCS_COMMON_REAL "${COMMON_REAL}/*.cpp")
file(GLOB SRCS_COMMON_MOCK "${COMMON_MOCK}/*.cpp")

# We build a static library that is the core of the project, the link it to the
# API's (executable and python at the moment)
add_library(${MARABOU_LIB} ${SRCS_COMMON_REAL} )
add_executable(${MARABOU_EXE} ${ENGINE_DIR}/main.cpp)



if(MSVC)
    # TODO: TEST this
    set(COMPILE_FLAGS  /W4 /WX) 
else()
    set(COMPILE_FLAGS  -Wall -Wextra  -Werror -MMD) #-Wno-deprecated
    set(RELEASE_FLAGS -O3) #-Wno-deprecated
endif()

if (RUN_MEMORY_TEST)
    set(MEMORY_FLAGS -fsanitize=address -fno-omit-frame-pointer -O1)
endif()

# Find boost
if (NOT EXISTS ${BOOST_DIR})
    # bash file that downloads and install boost 1_68_0, the name need to match
    # BOOST_DIR variable
    execute_process(COMMAND ${TOOLS_DIR}/download_boost.sh)
endif()

# Boost
set(BOOST_ROOT "${BOOST_DIR}/installed")
set(BOOST_INCLUDES "${BOOST_DIR}/installed/include")
find_package(Boost REQUIRED COMPONENTS program_options)
set(LIBS_INCLUDES ${BOOST_INCLUDES})
set(LIBS ${Boost_LIBRARIES})

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND LIBS Threads::Threads)

# build a static library     
target_link_libraries(${MARABOU_LIB} ${LIBS})
target_include_directories(${MARABOU_LIB} PRIVATE ${LIBS_INCLUDES})
target_compile_options(${MARABOU_LIB} PRIVATE ${COMPILE_FLAGS} ${RELEASE_FLAGS})


# Build marabou executable
target_link_libraries(${MARABOU_EXE} ${MARABOU_LIB})
target_include_directories(${MARABOU_EXE} PRIVATE ${LIBS_INCLUDES})

# Build Python marabou
if (${BUILD_PYTHON})
    if (NOT EXISTS ${PYBIND11_DIR})
        message("didnt find pybind, getting it")
        execute_process(COMMAND ${TOOLS_DIR}/download_pybind11.sh)
    endif()
    add_subdirectory(${PYBIND11_DIR})
    pybind11_add_module(${MARABOU_PY} ${PYTHON_API_DIR}/MarabouCore.cpp)

    target_link_libraries(${MARABOU_PY} PRIVATE ${MARABOU_LIB})
    target_include_directories(${MARABOU_PY} PRIVATE ${LIBS_INCLUDES})

    set_target_properties(${MARABOU_PY} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PYTHON_API_DIR})
    target_compile_options(${MARABOU_LIB} PRIVATE -fPIC ${COMPILE_FLAGS}) 
endif()



add_library(${MARABOU_TEST_LIB})
if (RUN_TEST)
    set (TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/tests)
    file(MAKE_DIRECTORY ${TEST_DIR})

    set(BASIS_DIR_TEST "${BASIS_DIR}/tests")
    set(COMMON_DIR_TEST "${COMMON_DIR}/tests")
    set(ENGINE_DIR_TEST "${ENGINE_DIR}/tests")

    set(CMAKE_PREFIX_PATH ${TOOLS_DIR}/cxxtest)
    find_package(CxxTest)
    if(CXXTEST_FOUND)
        include_directories(${CXXTEST_INCLUDE_DIR})
        enable_testing()
    endif()

    target_link_libraries(${MARABOU_TEST_LIB} ${LIBS})
    target_include_directories(${MARABOU_TEST_LIB} PRIVATE ${LIBS_INCLUDES})
    target_compile_options(${MARABOU_TEST_LIB} PRIVATE ${COMPILE_FLAGS}
        ${MEMORY_FLAGS} -g)
    add_subdirectory(${COMMON_DIR_TEST})
    add_subdirectory(${ENGINE_DIR_TEST})
    add_subdirectory(${BASIS_DIR_TEST})
endif()


add_subdirectory(${SRC_DIR})
add_subdirectory(${TOOLS_DIR})


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

