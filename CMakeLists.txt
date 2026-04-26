cmake_minimum_required(VERSION 3.20)
project(N3DS_Emulator VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(CPU_SOURCES
    src/cpu/arm11.cpp
    src/cpu/arm9.cpp
)

set(MEMORY_SOURCES
    src/memory/memory_stub.cpp
)

set(GPU_SOURCES
    src/gpu/gpu_stub.cpp
)

set(OS_SOURCES
    src/os/kernel_stub.cpp
)

set(IO_SOURCES
    src/io/io_stub.cpp
)

set(LOADER_SOURCES
    src/loader/loader_stub.cpp
)

# Create core emulator library
add_library(n3ds_core
    ${CPU_SOURCES}
    ${MEMORY_SOURCES}
    ${GPU_SOURCES}
    ${OS_SOURCES}
    ${IO_SOURCES}
    ${LOADER_SOURCES}
)

target_include_directories(n3ds_core PUBLIC include)

# Test executable
add_executable(test_cpu tests/test_cpu.cpp)
target_link_libraries(test_cpu n3ds_core)

# Enable testing
enable_testing()
add_test(NAME CPUTest COMMAND test_cpu)
