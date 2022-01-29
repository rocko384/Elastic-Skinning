# CMakeList.txt : CMake project for Elastic Skinning, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

string(REPLACE " " "_" ProjectName ${PROJECT_NAME})

set(SOURCE
	"source/main.cpp"
	"include/util.h"
	"include/gfxcontext.h"
	"source/gfxcontext.cpp"
	"include/asset.h"
	"source/asset.cpp"
	"include/swapchain.h"
	"source/swapchain.cpp"
	"include/renderer.h"
	"source/renderer.cpp"
	"include/window.h"
	"source/window.cpp"
	"include/gfxpipeline.h"
	"source/gfxpipeline.cpp"
	"include/mesh.h"
	"source/mesh.cpp"
	"source/deps_impl.cpp"
	"include/renderingtypes.h"
	"include/crc.h"
	"include/model.h"
	"include/skeleton.h"
	"include/animation.h"
 "source/animation.cpp")

file(GLOB
	SHADERS
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	shaders/*
)

message(STATUS "Shaders ${SHADERS}")

# Add source to this project's executable.
add_executable (${ProjectName} ${SOURCE})

compile_shader(${ProjectName}
	FORMAT bin
	SOURCES ${SHADERS}
)

target_include_directories(
	${ProjectName}
	PUBLIC
	"include"
	${VMA_INCLUDE_DIR}
	${STB_INCLUDE_DIR}
)

target_link_libraries(
	${ProjectName}
	PUBLIC
	SDL2::SDL2main
	SDL2::SDL2
	glm::glm
	Vulkan::Vulkan
	tinygltf
)

target_compile_definitions(
	${ProjectName}
	PUBLIC
	"TINYGLTF_NO_INCLUDE_STB_IMAGE"
	"TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE"
	"GLM_FORCE_DEPTH_ZERO_TO_ONE"
)

# TODO: Add tests and install targets if needed.
