# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("Elastic Skinning")

set (CMAKE_CXX_STANDARD 17)

# Add dependencies
find_package(Vulkan REQUIRED FATAL_ERROR)
add_subdirectory("Dependencies/glm")
add_subdirectory("Dependencies/SDL")

# Include sub-projects.
add_subdirectory ("Elastic Skinning")
