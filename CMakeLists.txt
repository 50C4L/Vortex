cmake_minimum_required(VERSION 3.10)

project(Vortex)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(Vortex 
	./src/main.cpp
)
# target_include_directories(Vortex PUBLIC ./thirdparty/include)