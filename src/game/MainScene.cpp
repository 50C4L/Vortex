#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/BuiltInMeshes.h>
#include <graphics/Renderer.h>
#include <graphics/BuiltInUniforms.h>
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

#include <ecs/systems/AudioSystem.h>
#include <ecs/systems/RenderSystem.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Physics.h>
#include <ecs/components/Render.h>

#include "GameConfig.h"
#include "systems/PlayerInputSystem.h"
#include "systems/PlayerGameplaySystem.h"
#include "components/PlayerComponents.h"

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

MainScene::MainScene( eage::graphics::Renderer& renderer, events::InputController& input_controller,
					  eage::ecs::ECSRegistry& ecs_registry, eage::ecs::AudioSystem& audio_system, eage::ecs::RenderSystem& render_system )
	: mRenderer( renderer )
	, mInputController( input_controller )
	, mECSRegistry( ecs_registry )
	, mAudioSystem( audio_system )
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

	PrepareMeshes();

	PrepareMaterials();

	CreateSceneRoot();

	CreatePlayerEntity();

	CreateScreenZoneEntities();

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) / 2.f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) / 2.f;
	mCamera = std::make_shared<eage::graphics::OrthographicCamera>( half_width * -1.f, half_width, half_height * -1.f, half_height, 0.1f, 100.0f );
	mCamera->SetPosition( { 0, 0, 2.f } );
}

uint64_t
MainScene::GetSceneRoot()
{
	return mSceneRootEntity;
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
	mPlayerGameplaySystem->Update( delta_time_ms.count() / 1000.f );

	// Update camera
	auto current_frame = mRenderer.GetCurrentFrameIndex();
	{
		SceneGlobalData scene_global_data;
		scene_global_data.view = mCamera->GetViewMatrix();
		scene_global_data.proj = mCamera->GetProjectionMatrix();
		scene_global_data.view_proj = scene_global_data.proj * scene_global_data.view;
		mRenderSystem.GetGlobalUniformBuffer()->Update( &scene_global_data, sizeof( SceneGlobalData ), sizeof( SceneGlobalData ) * current_frame );
	}
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

void 
MainScene::CreateSceneRoot()
{
	mSceneRootEntity = mECSRegistry.CreateEntity();

	// Root identity transform
	mECSRegistry.AddComponent( mSceneRootEntity, eage::ecs::TransformComponent{} );

	// Root relationship component
	mECSRegistry.AddComponent( mSceneRootEntity, eage::ecs::SceneGraphComponment{} );
}

void
MainScene::CreatePlayerEntity()
{
	mPlayerInputSystem = std::make_unique<PlayerInputSystem>( mECSRegistry, mInputController );
	mPlayerGameplaySystem = std::make_unique<PlayerGameplaySystem>( mECSRegistry );

	mPlayerEntity = mECSRegistry.CreateEntity();

	// Set parent-child relationship with scene root
	auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( mSceneRootEntity );
	root.children_entities.push_back( mPlayerEntity );
	eage::ecs::SceneGraphComponment player_relationship;
	player_relationship.parent_entity = mSceneRootEntity;
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player_relationship ) );

	// Player component with all player-specific data
	PlayerComponent player;
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player ) );
	
	// Velocity component
	mECSRegistry.AddComponent( mPlayerEntity, eage::ecs::Velocity2DComponent{} );
	
	// Transform component
	mECSRegistry.AddComponent( mPlayerEntity, eage::ecs::TransformComponent{} );

	// Physics component
	eage::ecs::PhysicsComponent player_physics;
	player_physics.body_type = eage::ecs::PhysicsComponent::BodyType::DYNAMIC;
	player_physics.is_sensor = true;
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player_physics ) );

	eage::ecs::CircleColliderComponent player_collider;
	player_collider.radius = 25.f; // Approximate radius of the ship
	mECSRegistry.AddComponent( mPlayerEntity, std::move( player_collider ) );

	// Render component
	assets::TextureAtlas texture_atlas( "./resources/textures/ship/ship_texatlas.json" );
	texture_atlas.Flip();
	const auto& ship_tex = texture_atlas.GetSubTexture( "player_ship.png" ); 

	auto rect = eage::graphics::made_rect_vertices( { 0, 0, 0 }, 50, 50 );
	rect.vertices[0].uv_x = ship_tex.uv_max.x;
	rect.vertices[0].uv_y = ship_tex.uv_min.y;
	rect.vertices[1].uv_x = ship_tex.uv_max.x;
	rect.vertices[1].uv_y = ship_tex.uv_max.y;
	rect.vertices[2].uv_x = ship_tex.uv_min.x;
	rect.vertices[2].uv_y = ship_tex.uv_min.y;
	rect.vertices[3].uv_x = ship_tex.uv_min.x;
	rect.vertices[3].uv_y = ship_tex.uv_max.y;

	// Create ship mesh through RenderSystem
	auto ship_mesh_id = mRenderSystem.CreateMeshBuffer( rect.indices, rect.vertices, 0, 6, 0 );

	// Create uniform buffer and descriptor for mesh data
	auto ship_uniform_buffer_id = mRenderSystem.CreateDynamicUniformBuffer( sizeof(eage::graphics::MeshUniformData) );
	auto ship_descriptor_id = mRenderSystem.CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
	
	// Set up the descriptor binding for the ship (this should be done once at creation time)
	mRenderSystem.GetDescriptorSet( ship_descriptor_id )->WriteBuffer(
		0, // binding
		vk::DescriptorType::eUniformBufferDynamic,
		mRenderSystem.GetUniformBuffer( ship_uniform_buffer_id )->buffer,
		sizeof(eage::graphics::MeshUniformData) );

	// Add ECS RenderComponent with the new resource IDs
	mECSRegistry.AddComponent( mPlayerEntity, eage::ecs::RenderComponent{ 
		ship_mesh_id, 
		mSpriteMaterialId, 
		ship_uniform_buffer_id, 
		ship_descriptor_id 
	} );

	// Audio components
	AudioSourceComponent thrust_audio;
	thrust_audio.sound_path = "./resources/sounds/thruster.mp3";
	thrust_audio.sound_resource_id = mAudioSystem.LoadSound( thrust_audio.sound_path );
	thrust_audio.should_loop = true;
	mECSRegistry.AddComponent( mPlayerEntity, std::move(thrust_audio) );
	mECSRegistry.AddComponent( mPlayerEntity, AudioEventComponent{}) ;

	// Create Thruster entity - child of ship
	auto thruster_entity = mECSRegistry.CreateEntity();

	// Set parent-child relationship with player entity
	auto& player_scene = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( mPlayerEntity );
	player_scene.children_entities.push_back( thruster_entity );
	auto& player_cmp = mECSRegistry.GetComponent<PlayerComponent>( mPlayerEntity );
	player_cmp.thruster_fx_entity = thruster_entity;

	// Thruster transform component
	eage::ecs::TransformComponent thruster_transform;
	thruster_transform.SetPosition( glm::vec3(0.0f, -30.f, 0.0f) ); // Behind ship in local space
	thruster_transform.SetScale( glm::vec3( 1 / 5.f ) );
	mECSRegistry.AddComponent( thruster_entity, std::move(thruster_transform) );

	const auto& thrust_tex = texture_atlas.GetSubTexture( "ship_thrust_fx.png" );
	rect.vertices[0].uv_x = thrust_tex.uv_max.x;
	rect.vertices[0].uv_y = thrust_tex.uv_min.y;
	rect.vertices[1].uv_x = thrust_tex.uv_max.x;
	rect.vertices[1].uv_y = thrust_tex.uv_max.y;
	rect.vertices[2].uv_x = thrust_tex.uv_min.x;
	rect.vertices[2].uv_y = thrust_tex.uv_min.y;
	rect.vertices[3].uv_x = thrust_tex.uv_min.x;
	rect.vertices[3].uv_y = thrust_tex.uv_max.y;

	// Create thruster mesh through RenderSystem
	auto thrust_mesh_id = mRenderSystem.CreateMeshBuffer( rect.indices, rect.vertices, 0, 6, 0 );
	auto thrust_uniform_buffer_id = mRenderSystem.CreateDynamicUniformBuffer( sizeof(eage::graphics::MeshUniformData) );
	auto thrust_descriptor_id = mRenderSystem.CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
	mRenderSystem.GetDescriptorSet( thrust_descriptor_id )->WriteBuffer(
		0, // binding
		vk::DescriptorType::eUniformBufferDynamic,
		mRenderSystem.GetUniformBuffer( thrust_uniform_buffer_id )->buffer,
		sizeof(eage::graphics::MeshUniformData) );

	// Add ECS RenderComponent with the new resource IDs
	mECSRegistry.AddComponent( thruster_entity, eage::ecs::RenderComponent{ 
		thrust_mesh_id, 
		mSpriteMaterialId, 
		thrust_uniform_buffer_id, 
		thrust_descriptor_id 
	} );
}

void 
MainScene::CreateScreenZoneEntities()
{
	mOnScreenZoneEntity = mECSRegistry.CreateEntity();

	// Set parent-child relationship with scene root
	auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( mSceneRootEntity );
	root.children_entities.push_back( mOnScreenZoneEntity );
	eage::ecs::SceneGraphComponment screen_zone_relationship;
	screen_zone_relationship.parent_entity = mSceneRootEntity;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( screen_zone_relationship ) );

	// Add Transform component
	mECSRegistry.AddComponent( mOnScreenZoneEntity, eage::ecs::TransformComponent{} );

	// Collision component
	eage::ecs::PhysicsComponent physics;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( physics ) );

	// Box collider component
	eage::ecs::BoxColliderComponent box_collider;
	// box_collider.width = static_cast<float>( config::DesignResolution::WIDTH );
	// box_collider.height = static_cast<float>( config::DesignResolution::HEIGHT );
	box_collider.width = 100.0f;
	box_collider.height = 100.0f;
	mECSRegistry.AddComponent( mOnScreenZoneEntity, std::move( box_collider ) );
}
