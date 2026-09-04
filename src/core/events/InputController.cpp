#include "InputController.h"

#include <SDL2/SDL.h>
#include <utility/Logger.h>

using namespace events;
using namespace utility;

namespace
{
	KeyCode sdl_keycode_to_keycode( SDL_Keycode sdl_key )
	{
		switch( sdl_key )
		{
			case SDLK_a: return KeyCode::A;
			case SDLK_b: return KeyCode::B;
			case SDLK_c: return KeyCode::C;
			case SDLK_d: return KeyCode::D;
			case SDLK_e: return KeyCode::E;
			case SDLK_f: return KeyCode::F;
			case SDLK_g: return KeyCode::G;
			case SDLK_h: return KeyCode::H;
			case SDLK_i: return KeyCode::I;
			case SDLK_j: return KeyCode::J;
			case SDLK_k: return KeyCode::K;
			case SDLK_l: return KeyCode::L;
			case SDLK_m: return KeyCode::M;
			case SDLK_n: return KeyCode::N;
			case SDLK_o: return KeyCode::O;
			case SDLK_p: return KeyCode::P;
			case SDLK_q: return KeyCode::Q;
			case SDLK_r: return KeyCode::R;
			case SDLK_s: return KeyCode::S;
			case SDLK_t: return KeyCode::T;
			case SDLK_u: return KeyCode::U;
			case SDLK_v: return KeyCode::V;
			case SDLK_w: return KeyCode::W;
			case SDLK_x: return KeyCode::X;
			case SDLK_y: return KeyCode::Y;
			case SDLK_z: return KeyCode::Z;
			case SDLK_0: return KeyCode::Key0;
			case SDLK_1: return KeyCode::Key1;
			case SDLK_2: return KeyCode::Key2;
			case SDLK_3: return KeyCode::Key3;
			case SDLK_4: return KeyCode::Key4;
			case SDLK_5: return KeyCode::Key5;
			case SDLK_6: return KeyCode::Key6;
			case SDLK_7: return KeyCode::Key7;
			case SDLK_8: return KeyCode::Key8;
			case SDLK_9: return KeyCode::Key9;
			case SDLK_SPACE:     return KeyCode::Space;
			case SDLK_RETURN:    return KeyCode::Enter;
			case SDLK_ESCAPE:    return KeyCode::Escape;
			case SDLK_TAB:       return KeyCode::Tab;
			case SDLK_BACKSPACE: return KeyCode::Backspace;
			case SDLK_LEFT:      return KeyCode::Left;
			case SDLK_RIGHT:     return KeyCode::Right;
			case SDLK_UP:        return KeyCode::Up;
			case SDLK_DOWN:      return KeyCode::Down;
			case SDLK_LSHIFT:    return KeyCode::LeftShift;
			case SDLK_RSHIFT:    return KeyCode::RightShift;
			case SDLK_LCTRL:     return KeyCode::LeftCtrl;
			case SDLK_RCTRL:     return KeyCode::RightCtrl;
			case SDLK_LALT:      return KeyCode::LeftAlt;
			case SDLK_RALT:      return KeyCode::RightAlt;
			default:             return KeyCode::Unknown;
		}
	}
}

InputController::InputController( std::unordered_map<KeyCode, uint64_t>&& keys_to_event_ids )
	: mKeysToEventIds( std::move( keys_to_event_ids ) )
{
}

InputController::~InputController()
{
}

void
InputController::Handle( SDL_Event& event )
{
	if( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP )
	{
		if( event.key.repeat != 0 )
		{
			return;
		}
		bool is_down = event.type == SDL_KEYDOWN;
		KeyCode key = sdl_keycode_to_keycode( event.key.keysym.sym );
		auto it = mKeysToEventIds.find( key );
		if( it != mKeysToEventIds.end() )
		{
			// Send event
			uint64_t event_id = it->second;
			auto obs_it = mEventObservers.find( event_id );
			if( obs_it != mEventObservers.end() )
			{
				for( auto observer : obs_it->second )
				{
					observer->OnInputEvent( event_id, is_down );
				}
			}
		}
	}
}

void
InputController::Subscribe( uint64_t event_id, Observer* observer )
{
	auto it = mEventObservers.find( event_id );
	if( it == mEventObservers.end() )
	{
		mEventObservers[event_id] = std::vector<Observer*>{ observer };
		return;
	}
	mEventObservers[event_id].push_back( observer );
}

void
InputController::Unsubscribe( uint64_t event_id, Observer* observer )
{
	auto it = mEventObservers.find( event_id );
	if( it != mEventObservers.end() )
	{
		auto& observers = it->second;
		observers.erase( std::remove( observers.begin(), observers.end(), observer ), observers.end() );
		if( observers.empty() )
		{
			mEventObservers.erase( it );
		}
	}
}
