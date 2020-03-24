file(GLOB SRCS "base/*.cpp")
file(GLOB SRCS "context/*.cpp")

file(GLOB HEADERS "base/*.h")
file(GLOB HEADERS "context/*.h")
file(GLOB HEADERS "include/*.h")

target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOUCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include")

if ($${BUILD_PYTHON})
  target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
