﻿# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)

project ("Elastic Skinning")

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set	(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

include(shader_build)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add dependencies
find_package(Vulkan REQUIRED FATAL_ERROR)
add_subdirectory("Dependencies/glm")
add_subdirectory("Dependencies/SDL")

set(TINYGLTF_HEADER_ONLY "True")

add_subdirectory("Dependencies/tinygltf")

set (VMA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Dependencies/VulkanMemoryAllocator/include")
set (STB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Dependencies/stb")
set (EIGEN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Dependencies/eigen")

# Include sub-projects.
add_subdirectory ("Elastic Skinning")
