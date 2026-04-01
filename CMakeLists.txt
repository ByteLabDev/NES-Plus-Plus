cmake_minimum_required(VERSION 3.13)  # CMake version check
project(NES-Plus-Plus)                # Create project "simple_example"

set(CMAKE_CXX_STANDARD 14)            # Enable c++14 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp") # Add src files

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include)