cmake_minimum_required(VERSION 3.10)

project(Vortex)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/core)

add_executable(Vortex 
	./src/AbstractScene.h
	./src/main.cpp
	./src/SceneController.h
	./src/SceneController.cpp
	./src/VortexGame.h
	./src/VortexGame.cpp
	./src/game/GameConfig.h
	./src/game/MainScene.h
	./src/game/MainScene.cpp
	./src/game/Player.h
	./src/game/Player.cpp
	./src/game/components/PlayerComponents.h
	./src/game/systems/PlayerInputSystem.h
	./src/game/systems/PlayerInputSystem.cpp
	./src/game/systems/PlayerMovementSystem.h
	./src/game/systems/PlayerMovementSystem.cpp
)

target_link_libraries(Vortex PRIVATE
	eage_core
)
