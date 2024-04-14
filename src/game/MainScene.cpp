#include "MainScene.h"

#include <utility/Logger.h>
#include <graphics/Renderer.h>
#include <graphics/Renderable.h>
#include <graphics/VulkanMesh.h>
#include <graphics/Camera.h>

using namespace vortex;
using namespace utility;

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

	mCamera = std::make_shared<graphics::OrthographicCamera>( 0.f, 1.f, 0.f, 1.f, 0.1f, 100.0f );
	mCamera->SetPosition( { 0, 0, 2.f } );
	mRenderer.SetCamera( mCamera );

	mPlayer = std::make_shared<graphics::Renderable>();

	std::vector<graphics::Vertex> rect_vertices;
	rect_vertices.resize( 4 );
	rect_vertices[0].position = {  0.5, -0.5, 0 };
	rect_vertices[1].position = {  0.5,  0.5, 0 };
	rect_vertices[2].position = { -0.5, -0.5, 0 };
	rect_vertices[3].position = { -0.5,  0.5, 0 };

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

	// player update
	mPlayer->Rotate( 0.01f, { 0, 0, 1 } );

	// enemy update

	// collision detection

	// mRenderer->AddToRenderQueue( mPlayer );
}