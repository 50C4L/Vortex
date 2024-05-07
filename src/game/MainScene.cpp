#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/RenderComponent.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanShader.h>
#include <graphics/Camera.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>
#include <graphics/Material.h>

#include <assets/ImageLoader.h>

#include "GameConfig.h"

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

	// @TODO: Move this to be handled inside RenderComponent
	struct RenderComponentData
	{
		alignas(64) glm::mat4 model;
		alignas(8) uint64_t vertex_buffer_address;
		// padding
		float extra[46];
	};
}

MainScene::MainScene( graphics::Renderer& renderer )
	: mRenderer( renderer )
{
}

MainScene::~MainScene()
{
}

void
MainScene::OnEnter()
{
	LOG( "MainScene::OnEnter" );

	// Descriptor set layout
	{
		graphics::DescriptorLayoutBuilder layout_builder;
		layout_builder.AddBinding( 0, vk::DescriptorType::eUniformBufferDynamic );
		mSceneGlobalDataLayout = layout_builder.Build( mRenderer.GetDevice(), vk::ShaderStageFlagBits::eVertex );
		mRenderComponentDataLayout = layout_builder.Build( mRenderer.GetDevice(), vk::ShaderStageFlagBits::eVertex );
	}

	// Material
	mSpriteMaterial = std::make_unique<SingleTextureSpriteMaterial>();
	{
		graphics::DescriptorLayoutBuilder builder;
		builder.AddBinding(0, vk::DescriptorType::eCombinedImageSampler );
		mSpriteMaterial->material_layout = builder.Build( mRenderer.GetDevice(), vk::ShaderStageFlagBits::eFragment );
	}
	mSpriteMaterial->build_pipeline( mRenderer, { mSceneGlobalDataLayout.get(), mRenderComponentDataLayout.get() } );
	mSpriteMaterial->pipeline->global_descriptor = std::make_shared<graphics::UniformDescriptor>( mRenderer, mSceneGlobalDataLayout.get() );
	const auto num_overlapping_frames = mRenderer.GetFrames().size();
	mSceneGlobalDataDynamic = graphics::ManagedBuffer::Create( 
		*mRenderer.GetMemoryAllocator().allocator.get(), sizeof( SceneGlobalData ) * num_overlapping_frames, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU );
	mSpriteMaterial->pipeline->global_descriptor->WriteDynamicBuffer( 0, vk::DescriptorType::eUniformBufferDynamic, mSceneGlobalDataDynamic->buffer, sizeof( SceneGlobalData ) );

	// Texture
	{
		assets::ImageLoader image_loader;
		auto image = image_loader.LoadImage( "./resources/textures/512_placeholder.png" );

		mSpriteMaterialResources = std::make_unique<SingleTextureSpriteMaterial::Resources>();
		mSpriteMaterialResources->color_texture = mRenderer.UploadImage( 
			image.data.data(), sizeof( unsigned char ) * image.data.size(), image.width, image.height, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor, 1 );
	}
	mSpriteMaterialResources->color_texture_sampler = mRenderer.CreateSampler( vk::Filter::eNearest, vk::Filter::eNearest );

	auto material_instance = mSpriteMaterial->Instantiate( mRenderer, *mSpriteMaterialResources );

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) / 2.f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) / 2.f;
	mCamera = std::make_shared<graphics::OrthographicCamera>( half_width * -1.f, half_width, half_height * -1.f, half_height, 0.1f, 100.0f );
	mCamera->SetPosition( { 0, 0, 2.f } );

	// Renderble
	mPlayer = std::make_shared<graphics::RenderComponent>();
	mPlayer->SetMaterial( std::move( material_instance ) );
	// RenderComponent uniform data
	auto player_descirptor = std::make_unique<graphics::UniformDescriptor>( mRenderer, mRenderComponentDataLayout.get() );
	mRenderComponentlDataDynamic = graphics::ManagedBuffer::Create( 
		*mRenderer.GetMemoryAllocator().allocator.get(), sizeof( RenderComponentData ) * num_overlapping_frames, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU );
	player_descirptor->WriteDynamicBuffer( 0, vk::DescriptorType::eUniformBufferDynamic, mRenderComponentlDataDynamic->buffer, sizeof( RenderComponentData ) );
	mPlayer->SetMeshDescriptor( std::move( player_descirptor ) );

	std::vector<graphics::Vertex> rect_vertices;
	rect_vertices.resize( 4 );
	rect_vertices[0].position = {  25, -25, 0 };
	rect_vertices[1].position = {  25,  25, 0 };
	rect_vertices[2].position = { -25, -25, 0 };
	rect_vertices[3].position = { -25,  25, 0 };

	rect_vertices[0].color = { 1, 0, 0, 1 };
	rect_vertices[1].color = { 1, 1, 0, 1 };
	rect_vertices[2].color = { 1, 0, 1, 1 };
	rect_vertices[3].color = { 0, 0, 1, 1 };

	rect_vertices[0].uv_x = 1.f;
	rect_vertices[0].uv_y = 0.f;
	rect_vertices[1].uv_x = 1.f;
	rect_vertices[1].uv_y = 1.f;
	rect_vertices[2].uv_x = 0.f;
	rect_vertices[2].uv_y = 0.f;
	rect_vertices[3].uv_x = 0.f;
	rect_vertices[3].uv_y = 1.f;

	std::vector<uint32_t> rect_indices;
	rect_indices.resize( 6 );
	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	auto mesh = mRenderer.UploadMesh( rect_indices, rect_vertices );
	mPlayer->SetMeshBuffer( std::move( mesh ), 0, 6, 0 );

	mRenderer.AddToRenderQueue( mPlayer );
}

void
MainScene::OnExit()
{
	LOG( "MainScene::OnExit" );
}

void
MainScene::Update()
{
	// player input

	// Update camera
	auto current_frame = mRenderer.GetCurrentFrameIndex();
	{
		SceneGlobalData scene_global_data;
		scene_global_data.view = mCamera->GetViewMatrix();
		scene_global_data.proj = mCamera->GetProjectionMatrix();
		scene_global_data.view_proj = scene_global_data.proj * scene_global_data.view;
		mSceneGlobalDataDynamic->Update( &scene_global_data, sizeof( SceneGlobalData ), sizeof( SceneGlobalData ) * current_frame );
	}
	
	// player update
	{
		mPlayer->Rotate( 0.01f, { 0, 0, 1 } );
		RenderComponentData render_data;
		render_data.model = mPlayer->GetModelMatrix();
		render_data.vertex_buffer_address = mPlayer->GetMeshBuffer()->vertex_buffer_address;
		mRenderComponentlDataDynamic->Update( &render_data, sizeof( RenderComponentData ), sizeof( RenderComponentData ) * current_frame );
	}
}