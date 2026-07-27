#ifndef _EAGE_UI_DATA_MODEL_H_
#define _EAGE_UI_DATA_MODEL_H_

#include <memory>
#include <string>

#include <ui/UIValue.h>

namespace eage::ui
{
	class UIDataModel
	{
	public:
		UIDataModel( const UIDataModel& ) = delete;
		UIDataModel& operator=( const UIDataModel& ) = delete;
		~UIDataModel();

		/// Must be called before LoadDocument(); RmlUi resolves data-* at parse time.
		void Declare( const std::string& key, UIValue initial );

		/// Dirties the document only when the value actually differs.
		void Set( const std::string& key, UIValue value );

	private:
		friend class UIView;

		struct Impl;
		explicit UIDataModel( std::unique_ptr<Impl> impl );

		/// rml_context is an opaque Rml::Context*; kept void* so this header stays Rml-free.
		static std::unique_ptr<UIDataModel> Create( void* rml_context, const std::string& model_name );

		std::unique_ptr<Impl> mImpl;
	};
}

#endif // _EAGE_UI_DATA_MODEL_H_
