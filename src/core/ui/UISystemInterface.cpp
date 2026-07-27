#include "UISystemInterface.h"

#include <utility/Logger.h>

using namespace eage::ui;
using namespace utility;

UISystemInterface::UISystemInterface()
	: mStartTime( std::chrono::steady_clock::now() )
{
}

double
UISystemInterface::GetElapsedTime()
{
	using namespace std::chrono;
	return duration<double>( steady_clock::now() - mStartTime ).count();
}

bool
UISystemInterface::LogMessage( Rml::Log::Type type, const Rml::String& message )
{
	switch( type )
	{
	case Rml::Log::LT_ERROR:
	case Rml::Log::LT_ASSERT:
		LOG_ERROR( message.c_str() );
		break;
	case Rml::Log::LT_WARNING:
		LOG( LOG_LEVEL::WARNING, message.c_str() );
		break;
	default:
		LOG( message.c_str() );
		break;
	}
	return true;
}
