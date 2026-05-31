#ifndef _EAGE_EVENTS_KEYCODE_H_
#define _EAGE_EVENTS_KEYCODE_H_

#include <cstddef>
#include <functional>

namespace events
{
	///
	/// KeyCode: Backend-agnostic key identifier.
	/// Extend as new keys are required by the project.
	///
	enum class KeyCode : int
	{
		Unknown = 0,

		A, B, C, D, E, F, G, H, I, J, K, L, M,
		N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

		Key0, Key1, Key2, Key3, Key4,
		Key5, Key6, Key7, Key8, Key9,

		Space,
		Enter,
		Escape,
		Tab,
		Backspace,

		Left,
		Right,
		Up,
		Down,

		LeftShift,
		RightShift,
		LeftCtrl,
		RightCtrl,
		LeftAlt,
		RightAlt,
	};
}

template<>
struct std::hash<events::KeyCode>
{
	std::size_t operator()( events::KeyCode k ) const noexcept
	{
		return std::hash<int>{}( static_cast<int>( k ) );
	}
};

#endif // _EAGE_EVENTS_KEYCODE_H_
