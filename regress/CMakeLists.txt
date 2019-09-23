cmake_minimum_required (VERSION 3.12)

file(GLOB SRCS "*.cpp")
# file(GLOB HEADERS "*.h")

target_sources(${MARABOU_REGRESS} PRIVATE ${SRCS})
target_include_directories(${MARABOU_REGRESS} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
