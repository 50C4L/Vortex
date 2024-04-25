#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/Renderable.h>
#include <graphics/VulkanDescriptor.h>
#include <graphics/VulkanMesh.h>
#include <graphics/VulkanPipeline.h>
#include <graphics/VulkanShader.h>
#include <graphics/Camera.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/VMAWrapper.h>

#include "GameConfig.h"

using namespace vortex;
using namespace utility;

namespace
{
	struct SceneGlobalData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 view_proj;
		// padding
		glm::vec4 extra[16];
	};

	struct RenderableData
	{
		glm::mat4 model;
		uint64_t vertex_buffer_address;
		// padding
		glm::vec4 extra[8];
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
		layout_builder.AddBinding( 0, vk::DescriptorType::eUniformBuffer );
		mSceneGlobalDataLayout = layout_builder.Build( mRenderer.GetDevice(), vk::ShaderStageFlagBits::eVertex );
		mRenderableDataLayout = layout_builder.Build( mRenderer.GetDevice(), vk::ShaderStageFlagBits::eVertex );
	}

	// Scene global uniform data
	mSceneGlobalDescriptor = std::make_shared<graphics::UniformDescriptor>( mRenderer, mSceneGlobalDataLayout.get() );
	mSceneGlobalData.resize( mRenderer.GetFrames().size() );
	for( size_t i = 0; i < mSceneGlobalData.size(); ++i )
	{
		mSceneGlobalData[i] = graphics::ManagedBuffer::Create( 
			*mRenderer.GetMemoryAllocator().allocator.get(), sizeof( SceneGlobalData ), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU );
		mSceneGlobalDescriptor->WriteBuffer( i, 0, vk::DescriptorType::eUniformBuffer, mSceneGlobalData[i]->buffer, 0, sizeof( SceneGlobalData ) );
	}

	// Render pipeline
	mGeneralPipeline = std::make_unique<graphics::RenderPipeline>();
	mGeneralPipeline->global_descriptor = mSceneGlobalDescriptor;
	std::vector<vk::DescriptorSetLayout> descriptor_layouts = { mSceneGlobalDataLayout.get(), mRenderableDataLayout.get() };
	vk::PipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>( descriptor_layouts.size() );
	pipeline_layout_info.pSetLayouts = descriptor_layouts.data();
	mGeneralPipeline->layout = mRenderer.GetDevice().createPipelineLayoutUnique( pipeline_layout_info );

	auto vertex_shader = graphics::create_shader_module_from_file( mRenderer.GetDevice(), "./src/shaders/compiled/colored_triangle_mesh.vert.spv" );
	if( !vertex_shader.has_value() )
	{
		LOG_ERROR( "Failed to create vertex shader module." );
		return;
	}

	auto fragment_shader = graphics::create_shader_module_from_file( mRenderer.GetDevice(), "./src/shaders/compiled/colored_triangle.frag.spv" );
	if( !fragment_shader.has_value() )
	{
		LOG_ERROR( "Failed to create fragment shader module." );
		return;
	}

	graphics::VulkanPipelineBuilder pipeline_builder;
	mGeneralPipeline->pipeline = pipeline_builder
		.SetPipelineLayout( mGeneralPipeline->layout.get() )
		.SetShaders( vertex_shader.value().get(), fragment_shader.value().get() )
		.SetInputTopology( vk::PrimitiveTopology::eTriangleList )
		.SetPolygonMode( vk::PolygonMode::eFill )
		.SetCullMode( vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise )
		.SetMultisampling()
		.SetBlendMode( vk::Bool32( VK_TRUE ), vk::BlendFactor::eOne, vk::BlendFactor::eDstAlpha, vk::BlendOp::eAdd )
		.SetColorAttachmentFormat( mRenderer.GetColorFormat() )
		.SetDepthTest( vk::Bool32( VK_TRUE ), vk::Bool32( VK_TRUE ), vk::CompareOp::eGreaterOrEqual )
		.SetDepthFormat( mRenderer.GetDepthFormat() )
		.Build( mRenderer.GetDevice() );

	float half_width = static_cast<float>( config::DesignResolution::WIDTH ) / 2.f;
	float half_height = static_cast<float>( config::DesignResolution::HEIGHT ) / 2.f;
	mCamera = std::make_shared<graphics::OrthographicCamera>( half_width * -1.f, half_width, half_height * -1.f, half_height, 0.1f, 100.0f );
	mCamera->SetPosition( { 0, 0, 2.f } );

	// Renderble
	mPlayer = std::make_shared<graphics::Renderable>( *mGeneralPipeline );
	// renderable uniform data
	auto player_descirptor = std::make_unique<graphics::UniformDescriptor>( mRenderer, mRenderableDataLayout.get() );
	mRenderablelData.resize( mRenderer.GetFrames().size() );
	for( size_t i = 0; i < mRenderablelData.size(); ++i )
	{
		mRenderablelData[i] = graphics::ManagedBuffer::Create( 
			*mRenderer.GetMemoryAllocator().allocator.get(), sizeof( RenderableData ), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU );
		player_descirptor->WriteBuffer( i, 0, vk::DescriptorType::eUniformBuffer, mRenderablelData[i]->buffer, 0, sizeof( RenderableData ) );
	}
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

	std::vector<uint32_t> rect_indices;
	rect_indices.resize( 6 );
	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	auto mesh = mRenderer.UploadMesh( rect_indices, rect_vertices );
	mPlayer->SetMeshBuffer( std::move( mesh ) );
	mPlayer->SetDrawIndexInfo( { 0, 6, 0 } );

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
	SceneGlobalData* scene_global_data = static_cast<SceneGlobalData*>( mSceneGlobalData[ current_frame ]->allocation_info.pMappedData );
	scene_global_data->view = mCamera->GetViewMatrix();
	scene_global_data->proj = mCamera->GetProjectionMatrix();
	scene_global_data->view_proj = scene_global_data->proj * scene_global_data->view;

	// player update
	mPlayer->Rotate( 0.01f, { 0, 0, 1 } );
	RenderableData* renderable_data = static_cast<RenderableData*>( mRenderablelData[ current_frame ]->allocation_info.pMappedData );
	renderable_data->model = mPlayer->GetModelMatrix();
	renderable_data->vertex_buffer_address = mPlayer->GetMeshBuffer()->vertex_buffer_address;
}