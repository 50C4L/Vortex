cmake_minimum_required(VERSION 3.10)

project(Vortex)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(Vortex 
	./src/main.cpp
	./src/VortexGame.h
	./src/VortexGame.cpp
	./src/core/graphics/Renderer.h
	./src/core/graphics/Renderer.cpp
	./src/core/utility/Pointers.h
	./src/core/utility/Logger.h
)

target_include_directories(Vortex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_include_directories(Vortex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include)
target_link_directories(Vortex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib)
target_link_libraries(Vortex PRIVATE
	SDL2-static.lib
	SDL2.lib
	SDL2main.lib
	vulkan-1.lib
)

if (WIN32)
target_link_libraries(Vortex PRIVATE
	imm32.lib
	version.lib
	winmm.lib
	Setupapi.lib
)
endif (WIN32)
