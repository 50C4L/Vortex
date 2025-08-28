#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanShader.h>
#include <graphics/Camera.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>
#include <graphics/Material.h>
#include <graphics/MaterialBuilder.h>
#include <imgui/imgui.h>
#include <events/InputController.h>
#include <audio/AudioMixer.h>
#include <assets/ImageLoader.h>
#include <assets/TextureAtlas.h>

#include <ecs/systems/RenderSystem.h>

#include "GameConfig.h"
#include "Player.h"

using namespace vortex;
using namespace utility;

namespace
{
	struct SceneGlobalData
	{
		alignas(64) glm::mat4 view;
		alignas(64) glm::mat4 proj;
		alignas(64) glm::mat4 view_proj;
		// padding
		float extra[16];
	};
}

MainScene::MainScene( eage::graphics::Renderer& renderer, events::InputController& input_controller, audio::AudioMixer& audio_mixer,
					  eage::ecs::ECSRegistry& ecs_registry, eage::ecs::RenderSystem& render_system )
	: mRenderer( renderer )
	, mInputController( input_controller )
	, mAudioMixer( audio_mixer )
	, mECSRegistry( ecs_registry )
	, mRenderSystem( render_system )
	, mLastUpdateTime( std::chrono::high_resolution_clock::now() )
{
}

MainScene::~MainScene()
{
}

void
MainScene::OnEnter()
{
	LOG( "MainScene::OnEnter" );

	// Set the ImGUI render function
	mRenderer.SetImGUIRenderFunction( [&](){ DrawDebugGUI(); } );

	// Prepare meshes
	PrepareMeshes();

	// Prepare materials
	PrepareMaterials();

	// Game objects - pass the sprite material resource ID to Player
	mPlayer = std::make_unique<Player>( mRenderer, mInputController, mECSRegistry, mRenderSystem );
	mPlayer->Init( mSpriteMaterialId,
				   std::make_unique<audio::SoundInstance>( mAudioMixer.CreateSound( "./resources/sounds/thruster.mp3" ) ) );

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) / 2.f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) / 2.f;
	mCamera = std::make_shared<eage::graphics::OrthographicCamera>( half_width * -1.f, half_width, half_height * -1.f, half_height, 0.1f, 100.0f );
	mCamera->SetPosition( { 0, 0, 2.f } );
}

void
MainScene::OnExit()
{
	LOG( "MainScene::OnExit" );
}

void
MainScene::Update()
{
	std::chrono::time_point<std::chrono::steady_clock> current_time = std::chrono::steady_clock::now();
	std::chrono::duration<float, std::milli> delta_time_ms = current_time - mLastUpdateTime;
	mLastUpdateTime = current_time;
	// player input

	// Update camera
	auto current_frame = mRenderer.GetCurrentFrameIndex();
	{
		SceneGlobalData scene_global_data;
		scene_global_data.view = mCamera->GetViewMatrix();
		scene_global_data.proj = mCamera->GetProjectionMatrix();
		scene_global_data.view_proj = scene_global_data.proj * scene_global_data.view;
		mRenderSystem.GetGlobalUniformBuffer()->Update( &scene_global_data, sizeof( SceneGlobalData ), sizeof( SceneGlobalData ) * current_frame );
	}
	
	// player update
	mPlayer->Update();

	// Add to render queue
	mPlayer->Draw();
}

void 
MainScene::PrepareMeshes()
{
}

void
MainScene::PrepareMaterials()
{
	mRenderSystem.CreateImageBuffer( "./resources/textures/ship/ship_texatlas.png" );

	// Create sprite material using the new MaterialBuilder and RenderSystem
	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders("./src/shaders/compiled/colored_triangle_mesh.vert.spv", 
					"./src/shaders/compiled/colored_triangle.frag.spv")
		.AddTexture(0, "./resources/textures/ship/ship_texatlas.png", 
					vk::Filter::eNearest, vk::Filter::eNearest)
		.SetAlphaBlending()
		.EnableDepthTest(true)
		.Build();

	mSpriteMaterialId = mRenderSystem.CreateMaterial(material_property);
}

void
MainScene::DrawDebugGUI()
{
	ImGuiStyle * style = &ImGui::GetStyle();

	style->WindowBorderSize = 0.0f;

	ImGui::Begin( "FPS Counter", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings );
	ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );
	ImGui::End();
}
