cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(PGD_Linf_Rand)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the CMake prefix path to include the new LibTorch location
list(APPEND CMAKE_PREFIX_PATH "~/libtorch/libtorch/share/cmake/Torch")

# Unset any environment variables that might influence the build
unset(ENV{CXXFLAGS})
unset(ENV{LDFLAGS})

# Find the required packages
find_package(Torch REQUIRED)

# Explicitly clear any MKL-related flags
set(CMAKE_CXX_FLAGS "")
set(CMAKE_SHARED_LINKER_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_REQUIRED_LIBRARIES "")

# Include directories
include_directories(${TORCH_INCLUDE_DIRS})
include_directories(${TORCH_INCLUDE_DIRS}/torch/csrc/api/include)

# Add the executable
add_executable(PGD_Linf_Rand main.cpp read_net.cpp pgd_attack.cpp)

# Link the libraries
target_link_libraries(PGD_Linf_Rand "${TORCH_LIBRARIES}")

# Ensure you have -D_GLIBCXX_USE_CXX11_ABI=0 for compatibility
set_property(TARGET PGD_Linf_Rand PROPERTY CXX_STANDARD 17)
set_property(TARGET PGD_Linf_Rand PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET PGD_Linf_Rand PROPERTY CXX_EXTENSIONS OFF)
