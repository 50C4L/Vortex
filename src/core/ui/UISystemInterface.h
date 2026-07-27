#ifndef _EAGE_UI_SYSTEM_INTERFACE_H_
#define _EAGE_UI_SYSTEM_INTERFACE_H_

#include <chrono>

#include <RmlUi/Core/SystemInterface.h>

namespace eage::ui
{
	class UISystemInterface final : public Rml::SystemInterface
	{
	public:
		UISystemInterface();

		double GetElapsedTime() override;
		bool LogMessage( Rml::Log::Type type, const Rml::String& message ) override;

	private:
		std::chrono::steady_clock::time_point mStartTime;
	};
}

#endif // _EAGE_UI_SYSTEM_INTERFACE_H_
