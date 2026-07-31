#include "UIView.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <ui/UIRenderInterface.h>
#include <ui/UIRenderPass.h>
#include <ui/UISystem.h>
#include <utility/Logger.h>
#include <graphics/AbstractRenderPass.h>
#include <graphics/ManagedVulkanResources.h>

using namespace eage::ui;
using namespace utility;

struct UIView::Impl
{
	UISystem& system;
	std::string context_name;
	Rml::Context* context = nullptr;
	std::unique_ptr<UIDataModel> data_model;
	std::unique_ptr<UIRenderPass> render_pass;
	Rml::ElementDocument* document = nullptr;
};

UIView::UIView(
	UISystem& system,
	const std::string& name,
	uint32_t width,
	uint32_t height,
	const std::string& model_name )
	: mImpl( std::make_unique<Impl>( Impl{ system, name } ) )
{
	mImpl->context = Rml::CreateContext(
		name,
		Rml::Vector2i{ static_cast<int>( width ), static_cast<int>( height ) },
		system.mRenderInterface.get() );

	if( !mImpl->context )
	{
		LOG_ERROR( "UIView: failed to create context '" + name + "'" );
		return;
	}

	mImpl->data_model = UIDataModel::Create( mImpl->context, model_name );
	if( !mImpl->data_model )
	{
		LOG_ERROR( "UIView: failed to create data model '" + model_name + "'" );
		return;
	}

	mImpl->render_pass = std::make_unique<UIRenderPass>(
		system.GetRenderer(),
		*system.mRenderInterface,
		*mImpl->context,
		width,
		height );
}

UIView::~UIView()
{
	if( mImpl->document )
	{
		mImpl->document->Close();
		mImpl->document = nullptr;
	}

	mImpl->render_pass.reset();
	mImpl->data_model.reset();

	if( mImpl->context )
	{
		Rml::RemoveContext( mImpl->context_name );
		mImpl->context = nullptr;
	}
}

UIDataModel&
UIView::GetDataModel()
{
	return *mImpl->data_model;
}

void
UIView::BindImage( const std::string& name, eage::graphics::ManagedImage& image )
{
	mImpl->system.mRenderInterface->BindExternalImage( name, image );
}

bool
UIView::LoadDocument( const std::string& rml_path )
{
	if( !mImpl->context )
	{
		return false;
	}

	mImpl->document = mImpl->context->LoadDocument( rml_path );
	if( !mImpl->document )
	{
		LOG_ERROR( "UIView: failed to load document " + rml_path );
		return false;
	}

	mImpl->document->Show();
	return true;
}

eage::graphics::AbstractRenderPass&
UIView::GetRenderPass()
{
	return *mImpl->render_pass;
}

eage::graphics::ManagedImage*
UIView::GetOutput()
{
	if( !mImpl->render_pass )
	{
		return nullptr;
	}
	return mImpl->render_pass->GetColorTarget();
}
