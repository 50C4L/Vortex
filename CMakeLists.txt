cmake_minimum_required(VERSION 3.10)

project(Vortex)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/core)

add_executable(Vortex 
	./src/AbstractScene.h
	./src/EngineContext.h
	./src/main.cpp
	./src/SceneController.h
	./src/SceneController.cpp
	./src/VortexGame.h
	./src/VortexGame.cpp
	./src/game/GameConfig.h
	./src/game/MainScene.h
	./src/game/MainScene.cpp
	./src/game/components/RewardComponent.h
	./src/game/components/ExperienceComponent.h
	./src/game/components/GameGenericComponents.h
	./src/game/components/PlayerComponents.h
	./src/game/components/EnemyComponent.h
	./src/game/components/DriftComponent.h
	./src/game/data/EnemyDefinition.h
	./src/game/data/EnemyDefinition.cpp
	./src/game/data/WaveTypes.h
	./src/game/data/WaveStore.h
	./src/game/data/WaveStore.cpp
	./src/game/systems/EnemySystem.h
	./src/game/systems/EnemySystem.cpp
	./src/game/systems/WaveSystem.h
	./src/game/systems/WaveSystem.cpp
	./src/game/systems/BulletSystem.h
	./src/game/systems/BulletSystem.cpp
	./src/game/systems/LevelingSystem.h
	./src/game/systems/LevelingSystem.cpp
	./src/game/systems/PlayerInputSystem.h
	./src/game/systems/PlayerInputSystem.cpp
	./src/game/systems/PlayerGameplaySystem.h
	./src/game/systems/PlayerGameplaySystem.cpp
	./src/game/systems/WarpSystem.h
	./src/game/systems/WarpSystem.cpp
	./src/game/ui/StatusPanel.h
	./src/game/ui/StatusPanel.cpp
)

target_link_libraries(Vortex PRIVATE
	eage_core
)

add_executable(AnimTool
	./src/tools/animtool/main.cpp
	./src/tools/animtool/AnimToolApp.h
	./src/tools/animtool/AnimToolApp.cpp
	./src/tools/animtool/FileDialog.h
	./src/tools/animtool/FileDialog.cpp
	./src/tools/animtool/FrameThumbnail.h
	./src/tools/animtool/FrameThumbnail.cpp
	./src/tools/animtool/FrameSequence.h
	./src/tools/animtool/FrameSequence.cpp
	./src/tools/animtool/AnimationExporter.h
	./src/tools/animtool/AnimationExporter.cpp
	./src/tools/animtool/ToolUIStyle.h
)

target_link_directories(AnimTool PRIVATE
	$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/debug/lib>
	$<$<NOT:$<CONFIG:Debug>>:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/release/lib>
)

target_link_libraries(AnimTool PRIVATE
	eage_core
	nfd.lib
)

if(WIN32)
target_link_libraries(AnimTool PRIVATE
	ole32.lib
	uuid.lib
	shell32.lib
)
endif()

function( vortex_copy_runtime_assets target_name )
	add_custom_command( TARGET ${target_name} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/resources
			$<TARGET_FILE_DIR:${target_name}>/resources
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/src/shaders
			$<TARGET_FILE_DIR:${target_name}>/src/shaders
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/src/core/ui/shaders
			$<TARGET_FILE_DIR:${target_name}>/src/core/ui/shaders
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/src/core/graphics/shaders
			$<TARGET_FILE_DIR:${target_name}>/src/core/graphics/shaders
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/src/game/ui/rml
			$<TARGET_FILE_DIR:${target_name}>/src/game/ui/rml
	)
endfunction()

vortex_copy_runtime_assets( Vortex )
vortex_copy_runtime_assets( AnimTool )
