cmake_minimum_required(VERSION 3.10)

project(eage_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(THIRDPARTY_DEBUG_PATH ${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/debug)
set(THIRDPARTY_RELEASE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/release)
set(THIRDPARTY_INCLUDE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/include)
set(THIRDPARTY_SRC_PATH ${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/src)

# IMGUI static library (vendored Dear ImGui)
set(IMGUI_PATH ${CMAKE_CURRENT_LIST_DIR}/imgui/vendor)

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
	${THIRDPARTY_INCLUDE_PATH}
)
target_link_directories(imgui PUBLIC
	$<$<CONFIG:Debug>:${THIRDPARTY_DEBUG_PATH}/lib>
	$<$<NOT:$<CONFIG:Debug>>:${THIRDPARTY_RELEASE_PATH}/lib>
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
	./graphics/AbstractRenderPass.h
	./graphics/CommandBuffer.h
	./graphics/SceneRenderPass.h
	./graphics/SceneRenderPass.cpp
	./graphics/PresentPass.h
	./graphics/PresentPass.cpp
	./graphics/CompositePass.h
	./graphics/CompositePass.cpp
	./graphics/ImageUtilities.h
	./graphics/ImageUtilities.cpp
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
	./graphics/ShaderReflection.h
	./graphics/ShaderReflection.cpp
	./graphics/VulkanShader.h
	./graphics/VulkanShader.cpp
	./graphics/VulkanSwapchain.h
	./graphics/VulkanSwapchain.cpp
	./graphics/VulkanContext.h
	./graphics/VulkanContext.cpp

	./assets/ImageLoader.h
	./assets/ImageLoader.cpp
	./assets/ImageWriter.h
	./assets/ImageWriter.cpp
	./assets/AnimationClip.h
	./assets/AnimationClip.cpp
	./assets/SceneResourceLoader.h
	./assets/SceneResourceLoader.cpp

	./events/KeyCode.h
	./events/InputController.h
	./events/InputController.cpp

	./audio/AudioMixer.h
	./audio/AudioMixer.cpp

	./ecs/ComponentPool.h
	./ecs/ECS.h
	./ecs/ResourceStore.h
	./ecs/components/Audio.h
	./ecs/components/Animation.h
	./ecs/components/Basics.h
	./ecs/components/Effect.h
	./ecs/components/Hud.h
	./ecs/components/Physics.h
	./ecs/components/Render.h
	./ecs/systems/AudioSystem.h
	./ecs/systems/AudioSystem.cpp
	./ecs/systems/AnimationSystem.h
	./ecs/systems/AnimationSystem.cpp
	./ecs/systems/EffectSystem.h
	./ecs/systems/EffectSystem.cpp
	./ecs/systems/PhysicsSystem.h
	./ecs/systems/PhysicsSystem.cpp
	./ecs/systems/RenderSystem.h
	./ecs/systems/RenderSystem.cpp
	./ecs/systems/SceneGraphSystem.h
	./ecs/systems/SceneGraphSystem.cpp
	./ecs/loaders/TextureLoadDelegate.h
	./ecs/loaders/TextureLoadDelegate.cpp
	./ecs/loaders/AnimationLoadDelegate.h
	./ecs/loaders/AnimationLoadDelegate.cpp
	./ecs/loaders/AudioLoadDelegate.h
	./ecs/loaders/AudioLoadDelegate.cpp

	./imgui/ImGuiRenderPass.h
	./imgui/ImGuiRenderPass.cpp
	./imgui/ImGUILifetime.h
	./imgui/ImGUILifetime.cpp

	./ui/UIValue.h
	./ui/UIDataModel.h
	./ui/UIDataModel.cpp
	./ui/UISystemInterface.h
	./ui/UISystemInterface.cpp
	./ui/UIRenderInterface.h
	./ui/UIRenderInterface.cpp
	./ui/UIRenderPass.h
	./ui/UIRenderPass.cpp
	./ui/UISystem.h
	./ui/UISystem.cpp
	./ui/UIView.h
	./ui/UIView.cpp
	./ui/FontLoadDelegate.h
	./ui/FontLoadDelegate.cpp

	./physics/PhysicsEngine.h
	./physics/PhysicsEngine.cpp
	./physics/PhysicsEventListener.h

	./profiling/AbstractGPUTimingProvider.h
	./profiling/PerformanceTracker.h
	./profiling/PerformanceTracker.cpp

	./utility/Filesystem.h
	./utility/Filesystem.cpp
	./utility/Pointers.h
	./utility/Logger.h
	./utility/JsonParser.h

	${THIRDPARTY_SRC_PATH}/spirv-reflect/spirv_reflect.h
	${THIRDPARTY_SRC_PATH}/spirv-reflect/spirv_reflect.c
)

add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
add_definitions(-DSPIRV_REFLECT_USE_SYSTEM_SPIRV_H)

target_include_directories(eage_core PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${THIRDPARTY_SRC_PATH}/spirv-reflect/include
)

# Required when linking RmlUi as a static library (avoids dllimport on MSVC).
target_compile_definitions(eage_core PUBLIC
	RMLUI_STATIC_LIB
)

target_link_libraries(eage_core PUBLIC
	imgui
	VulkanMemoryAllocator.lib
	box2
	rmlui.lib
	rmlui_debugger.lib
	freetype.lib
)
