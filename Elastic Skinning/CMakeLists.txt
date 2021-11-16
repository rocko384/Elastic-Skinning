# CMakeList.txt : CMake project for Elastic Skinning, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

string(REPLACE " " "_" ProjectName ${PROJECT_NAME})

set(SOURCE
	"source/main.cpp"
	"include/util.h"
	"include/gfxcontext.h"
	"source/gfxcontext.cpp"
)

# Add source to this project's executable.
add_executable (${ProjectName} ${SOURCE})

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
