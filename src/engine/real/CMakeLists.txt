cmake_minimum_required (VERSION 3.12)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

# message("${HEADERS}")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
