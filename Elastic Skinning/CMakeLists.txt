# CMakeList.txt : CMake project for Elastic Skinning, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

string(REPLACE " " "_" ProjectName ${PROJECT_NAME})

# Add source to this project's executable.
add_executable (${ProjectName} "source/main.cpp" "include/util.h")

target_include_directories(
	${ProjectName}
	PUBLIC
	"include"
)

# TODO: Add tests and install targets if needed.
