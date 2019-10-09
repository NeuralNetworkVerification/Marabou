cmake_minimum_required (VERSION 3.5)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")
# message("${HEADERS}")

# target_sources(COMMON_REAL PRIVATE ${HEADERS})
# target_sources(COMMON_REAL PRIVATE ${SRCS})
# target_include_directories(COMMON_REAL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")


if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
