cmake_minimum_required (VERSION 3.10)

# add_subdirectory(mock)
# add_subdirectory(real)
file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

# message("${HEADERS}")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if (RUN_TEST)
    target_sources(${MARABOU_TEST_LIB} PRIVATE ${SRCS})
    target_include_directories(${MARABOU_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()




