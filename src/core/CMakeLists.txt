cmake_minimum_required(VERSION 3.10)

project(eage_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(THIRDPARTY_DEBUG_PATH ${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/debug)
set(THIRDPARTY_RELEASE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/release)

# IMGUI static library
set(IMGUI_PATH ${CMAKE_CURRENT_LIST_DIR}/imgui)

add_library(imgui STATIC
	${IMGUI_PATH}/imgui.h
	${IMGUI_PATH}/imgui.cpp
	${IMGUI_PATH}/imconfig.h
	${IMGUI_PATH}/imgui_demo.cpp
	${IMGUI_PATH}/imgui_draw.cpp
	${IMGUI_PATH}/imgui_internal.h
	${IMGUI_PATH}/imstb_rectpack.h
	${IMGUI_PATH}/imstb_textedit.h
	${IMGUI_PATH}/imstb_truetype.h
	${IMGUI_PATH}/imgui_tables.cpp
	${IMGUI_PATH}/imgui_widgets.cpp
	${IMGUI_PATH}/imgui_impl_sdl2.h
	${IMGUI_PATH}/imgui_impl_sdl2.cpp
	${IMGUI_PATH}/imgui_impl_vulkan.h
	${IMGUI_PATH}/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC 
	debug ${THIRDPARTY_DEBUG_PATH}/include
	optimized ${THIRDPARTY_RELEASE_PATH}/include
)
target_link_directories(imgui PUBLIC 
	debug ${THIRDPARTY_DEBUG_PATH}/lib
	optimized ${THIRDPARTY_RELEASE_PATH}/lib
)

# Thirdparty
target_link_libraries(imgui PUBLIC
	SDL2-static.lib
	SDL2.lib
	SDL2main.lib
	vulkan-1.lib
)

if (WIN32)
target_link_libraries(imgui PUBLIC
	imm32.lib
	version.lib
	winmm.lib
	Setupapi.lib
)
endif (WIN32)

# CORE
add_library(eage_core STATIC
	./graphics/BuiltInMeshes.h
	./graphics/BuiltInMeshes.cpp
	./graphics/ImageUtilities.h
	./graphics/ImageUtilities.cpp
	./graphics/ImGUILifetime.h
	./graphics/ImGUILifetime.cpp
	./graphics/Camera.h
	./graphics/Camera.cpp
	./graphics/ManagedVulkanResources.h
	./graphics/ManagedVulkanResources.cpp
	./graphics/Material.h
	./graphics/MaterialBuilder.h
	./graphics/MaterialBuilder.cpp
	./graphics/MaterialProperty.h
	./graphics/Renderer.h
	./graphics/Renderer.cpp
	./graphics/VMAWrapper.h
	./graphics/VMAWrapper.cpp
	./graphics/VulkanDescriptor.h
	./graphics/VulkanDescriptor.cpp
	./graphics/VulkanCommandContext.h
	./graphics/VulkanCommandContext.cpp
	./graphics/VulkanMesh.h
	./graphics/VulkanPipeline.h
	./graphics/VulkanPipeline.cpp
	./graphics/VulkanShader.h
	./graphics/VulkanShader.cpp
	./graphics/VulkanSwapchain.h
	./graphics/VulkanSwapchain.cpp
	./graphics/VulkanContext.h
	./graphics/VulkanContext.cpp

	./assets/ImageLoader.h
	./assets/ImageLoader.cpp
	./assets/TextureAtlas.h
	./assets/TextureAtlas.cpp

	./events/InputController.h
	./events/InputController.cpp

	./audio/AudioMixer.h
	./audio/AudioMixer.cpp

	./ecs/ECS.h
	./ecs/ResourceManager.h
	./ecs/components/Audio.h
	./ecs/components/Basics.h
	./ecs/components/Physics.h
	./ecs/components/Render.h
	./ecs/systems/AudioSystem.h
	./ecs/systems/AudioSystem.cpp
	./ecs/systems/PhysicsSystem.h
	./ecs/systems/PhysicsSystem.cpp
	./ecs/systems/RenderSystem.h
	./ecs/systems/RenderSystem.cpp
	./ecs/systems/SceneGraphSystem.h
	./ecs/systems/SceneGraphSystem.cpp

	./physics/PhysicsEngine.h
	./physics/PhysicsEngine.cpp
	./physics/PhysicsEventListener.h

	./profiling/PerformanceTracker.h
	./profiling/PerformanceTracker.cpp

	./utility/Filesystem.h
	./utility/Pointers.h
	./utility/Logger.h
	./utility/JsonParser.h
)

add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

target_include_directories(eage_core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(eage_core PUBLIC
	imgui
	VulkanMemoryAllocator.lib
	box2
)
