cmake_minimum_required(VERSION 2.8.4)

project(Marabou)

set(makefilepath /cs/labs/guykatz/shirana/Marabou)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_custom_target(Marabou COMMAND make -C ${makefilepath}
        CLION_EXE_DIR=${makefilepath})
