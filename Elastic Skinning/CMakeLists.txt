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
)

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
)

target_link_libraries(
	${ProjectName}
	PUBLIC
	SDL2::SDL2main
	SDL2::SDL2
	glm::glm
	Vulkan::Vulkan
)
# TODO: Add tests and install targets if needed.
