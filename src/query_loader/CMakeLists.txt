cmake_minimum_required (VERSION 3.10)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")
file(GLOB SRC_EXCLUDE "main.cpp")

list(REMOVE_ITEM SRCS "${SRC_EXCLUDE}")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

