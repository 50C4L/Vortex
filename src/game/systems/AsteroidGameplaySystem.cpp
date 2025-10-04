#include "AsteroidGameplaySystem.h"

#include <ecs/ECS.h>
#include <ecs/components/Basics.h>
#include <ecs/components/Render.h>
#include <ecs/systems/RenderSystem.h>
#include <graphics/BuiltInMeshes.h>
#include <graphics/BuiltInUniforms.h>
#include <graphics/MaterialBuilder.h>
#include <graphics/Renderer.h>
#include <assets/TextureAtlas.h>

using namespace vortex;

AsteroidGameplaySystem::AsteroidGameplaySystem( eage::ecs::ECSRegistry& registry, eage::ecs::RenderSystem& render_system, 
												eage::graphics::Renderer& renderer )
	: mECSRegistry( registry )
	, mRenderSystem( render_system )
	, mRenderer( renderer )
{
}

AsteroidGameplaySystem::~AsteroidGameplaySystem()
{
}

void AsteroidGameplaySystem::PrepareAsteroids( int count, uint64_t root_entity )
{
	// Create asteroid material
	mRenderSystem.CreateImageBuffer( "./resources/textures/asteroid/asteroid.png" );

	// Create sprite material using the new MaterialBuilder and RenderSystem
	auto material_property = eage::graphics::MaterialBuilder()
		.SetShaders("./src/shaders/compiled/colored_triangle_mesh.vert.spv", 
					"./src/shaders/compiled/colored_triangle.frag.spv")
		.AddTexture(0, "./resources/textures/asteroid/asteroid.png", 
					vk::Filter::eNearest, vk::Filter::eNearest)
		.SetAlphaBlending()
		.EnableDepthTest(true)
		.Build();

	mAsteroidMaterialId = mRenderSystem.CreateMaterial( material_property );

	// Load texture
	assets::TextureAtlas texture_atlas( "./resources/textures/asteroid/asteroid.json" );
	texture_atlas.Flip();
	const auto& asteroid_tex = texture_atlas.GetSubTexture( "asteroid_L_0.png" );

	// Create mesh once - THIS CAN BE REUSED
	auto rect = eage::graphics::made_rect_vertices({0, 0, 0}, 50, 50);
	rect.vertices[0].uv_x = asteroid_tex.uv_max.x;
	rect.vertices[0].uv_y = asteroid_tex.uv_min.y;
	rect.vertices[1].uv_x = asteroid_tex.uv_max.x;
	rect.vertices[1].uv_y = asteroid_tex.uv_max.y;
	rect.vertices[2].uv_x = asteroid_tex.uv_min.x;
	rect.vertices[2].uv_y = asteroid_tex.uv_min.y;
	rect.vertices[3].uv_x = asteroid_tex.uv_min.x;
	rect.vertices[3].uv_y = asteroid_tex.uv_max.y;

	// Create shared mesh - ALL ASTEROIDS CAN USE THIS
	mAsteroidMeshId = mRenderSystem.CreateMeshBuffer(rect.indices, rect.vertices, 0, 6, 0);

	// Create given number of asteroids
	for( int i = 0; i < count; ++i )
	{
		auto asteroid = mECSRegistry.CreateEntity();
		mAvailableAsteroids.push_back( asteroid );

		// Set parent-child relationship with scene root
		auto& root = mECSRegistry.GetComponent<eage::ecs::SceneGraphComponment>( root_entity );
		root.children_entities.push_back( asteroid );
		eage::ecs::SceneGraphComponment relationship;
		relationship.parent_entity = root_entity;
		mECSRegistry.AddComponent( asteroid, std::move( relationship ) );
		
		// Transform component
		eage::ecs::TransformComponent transform;
		transform.SetPosition( glm::vec3( rand() % 800 - 400, rand() % 600 - 300, 0.0f ) );
		transform.SetScale( glm::vec3( 1.0f + (rand() % 100) / 100.f ) ); // Random scale between 1.0 and 2.0
		mECSRegistry.AddComponent( asteroid, std::move( transform ) );

		// Render component
		auto asteroid_uniform_buffer_id = mRenderSystem.CreateDynamicUniformBuffer( sizeof(eage::graphics::MeshUniformData) );
		auto asteroid_descriptor_id = mRenderSystem.CreateDynamicDescriptorSet( mRenderer.GetBuiltInDescriptorSetLayouts().per_object.get() );
		
		// Set up the descriptor binding for the asteroid (this should be done once at creation time)
		mRenderSystem.GetDescriptorSet( asteroid_descriptor_id )->WriteBuffer(
			0, // binding
			vk::DescriptorType::eUniformBufferDynamic,
			mRenderSystem.GetUniformBuffer( asteroid_uniform_buffer_id )->buffer,
			sizeof(eage::graphics::MeshUniformData) );

		// Add ECS RenderComponent with the new resource IDs
		mECSRegistry.AddComponent( asteroid, eage::ecs::RenderComponent{ 
			mAsteroidMeshId, 
			mAsteroidMaterialId, 
			asteroid_uniform_buffer_id, 
			asteroid_descriptor_id 
		} );
	}
}

void AsteroidGameplaySystem::Update()
{
}