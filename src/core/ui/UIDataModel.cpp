#include "UIDataModel.h"

#include <type_traits>
#include <unordered_map>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Variant.h>
#include <utility/Logger.h>

using namespace eage::ui;
using namespace utility;

struct UIDataModel::Impl
{
	Rml::Context* context = nullptr;
	std::string model_name;
	Rml::DataModelConstructor constructor;
	Rml::DataModelHandle handle;
	std::unordered_map<std::string, UIValue> values;
};

namespace
{
	void assign_variant( Rml::Variant& variant, const UIValue& value )
	{
		std::visit(
			[&]( const auto& v )
			{
				using T = std::decay_t<decltype( v )>;
				if constexpr( std::is_same_v<T, bool> )
				{
					variant = v;
				}
				else if constexpr( std::is_same_v<T, int> )
				{
					variant = v;
				}
				else if constexpr( std::is_same_v<T, float> )
				{
					variant = v;
				}
				else if constexpr( std::is_same_v<T, std::string> )
				{
					variant = Rml::String( v.c_str() );
				}
			},
			value );
	}
}

UIDataModel::UIDataModel( std::unique_ptr<Impl> impl )
	: mImpl( std::move( impl ) )
{
}

UIDataModel::~UIDataModel() = default;

std::unique_ptr<UIDataModel>
UIDataModel::Create( void* rml_context, const std::string& model_name )
{
	auto* context = static_cast<Rml::Context*>( rml_context );
	auto impl = std::make_unique<Impl>();
	impl->context = context;
	impl->model_name = model_name;
	impl->constructor = context->CreateDataModel( model_name );
	if( !impl->constructor )
	{
		LOG_ERROR( "UIDataModel: failed to create data model '" + model_name + "'" );
		return nullptr;
	}
	impl->handle = impl->constructor.GetModelHandle();
	return std::unique_ptr<UIDataModel>( new UIDataModel( std::move( impl ) ) );
}

void
UIDataModel::Declare( const std::string& key, UIValue initial )
{
	if( !mImpl || !mImpl->constructor )
	{
		LOG_ERROR( "UIDataModel::Declare called without a valid model" );
		return;
	}

	mImpl->values[ key ] = std::move( initial );

	const bool ok = mImpl->constructor.BindFunc(
		key,
		[ this, key ]( Rml::Variant& variant )
		{
			auto it = mImpl->values.find( key );
			if( it != mImpl->values.end() )
			{
				assign_variant( variant, it->second );
			}
		} );

	if( !ok )
	{
		LOG_ERROR( "UIDataModel: failed to bind key '" + key + "'" );
	}
}

void
UIDataModel::Set( const std::string& key, UIValue value )
{
	if( !mImpl )
	{
		return;
	}

	auto it = mImpl->values.find( key );
	if( it == mImpl->values.end() )
	{
		LOG_ERROR( "UIDataModel::Set unknown key '" + key + "' (Declare it first)" );
		return;
	}

	if( it->second == value )
	{
		return;
	}

	it->second = std::move( value );
	mImpl->handle.DirtyVariable( key );
}
