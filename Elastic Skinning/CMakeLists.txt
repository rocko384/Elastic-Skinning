# CMakeList.txt : CMake project for Elastic Skinning, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

string(REPLACE " " "_" ProjectName ${PROJECT_NAME})

set(SOURCE
	"source/main.cpp"
	"include/util.h"
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
	glm::glm
	SDL2::SDL2
	Vulkan::Vulkan
)
# TODO: Add tests and install targets if needed.
