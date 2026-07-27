#ifndef _EAGE_UI_VALUE_H_
#define _EAGE_UI_VALUE_H_

#include <string>
#include <variant>

namespace eage::ui
{
	using UIValue = std::variant<bool, int, float, std::string>;
}

#endif // _EAGE_UI_VALUE_H_
