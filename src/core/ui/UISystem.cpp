#include "UISystem.h"

#include <RmlUi/Core.h>

#include <ui/UIRenderInterface.h>
#include <ui/UISystemInterface.h>
#include <utility/Logger.h>

using namespace eage::ui;
using namespace utility;

UISystem::UISystem( graphics::Renderer& renderer )
	: mRenderer( renderer )
	, mSystemInterface( std::make_unique<UISystemInterface>() )
	, mRenderInterface( std::make_unique<UIRenderInterface>( renderer ) )
{
	Rml::SetSystemInterface( mSystemInterface.get() );
	Rml::SetRenderInterface( mRenderInterface.get() );
	mInitialised = Rml::Initialise();
	if( !mInitialised )
	{
		LOG_ERROR( "UISystem: Rml::Initialise failed" );
	}
}

UISystem::~UISystem()
{
	if( mInitialised )
	{
		Rml::Shutdown();
		mInitialised = false;
	}
	mRenderInterface.reset();
	mSystemInterface.reset();
}

bool
UISystem::LoadFontFace( const std::string& path )
{
	if( !mInitialised )
	{
		return false;
	}
	return Rml::LoadFontFace( path );
}
