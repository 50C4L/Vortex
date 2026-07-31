#ifndef _GAMECONFIG_H
#define _GAMECONFIG_H

namespace vortex
{
	namespace config
	{
		enum class SceneID : int
		{
			MENU_SCENE = 0,
			MAIN_SCENE = 1
		};

		enum class DesignResolution
		{
			WIDTH = 1280,
			HEIGHT = 720
		};

		// Low-res render target for pixel-art style. Each "pixel" here maps to
		// (ScreenResolution / VirtualResolution) screen pixels, upscaled with nearest-neighbor.
		enum class VirtualResolution
		{
			WIDTH  = 640,
			HEIGHT = 360
		};

		// Play field is the virtual resolution minus the status panel column on the right.
		// Camera and SceneRenderPass stay at full VirtualResolution; only warp/spawn use these.
		namespace layout
		{
			constexpr float DESIGN_PER_VIRTUAL =
				static_cast<float>( DesignResolution::WIDTH ) / static_cast<float>( VirtualResolution::WIDTH );

			constexpr float STATUS_PANEL_WIDTH_VIRTUAL = 64.f;
			constexpr float STATUS_PANEL_WIDTH = STATUS_PANEL_WIDTH_VIRTUAL * DESIGN_PER_VIRTUAL;	// 128

			constexpr float PLAY_FIELD_WIDTH  = static_cast<float>( DesignResolution::WIDTH ) - STATUS_PANEL_WIDTH;	// 1152
			constexpr float PLAY_FIELD_HEIGHT = static_cast<float>( DesignResolution::HEIGHT );						// 720

			constexpr float PLAY_FIELD_LEFT   = -static_cast<float>( DesignResolution::WIDTH ) * 0.5f;	// -640
			constexpr float PLAY_FIELD_RIGHT  = PLAY_FIELD_LEFT + PLAY_FIELD_WIDTH;						//  512
			constexpr float PLAY_FIELD_TOP    = PLAY_FIELD_HEIGHT * 0.5f;
			constexpr float PLAY_FIELD_BOTTOM = -PLAY_FIELD_TOP;
			constexpr float PLAY_FIELD_CENTER_X = ( PLAY_FIELD_LEFT + PLAY_FIELD_RIGHT ) * 0.5f;		//  -64
		}

		struct ScreenResolution
		{
			int width = 1920;
			int height = 1080;
		};

		inline float get_scale_factor( int screen_width, int design_width )
		{
			return static_cast<float>( screen_width ) / static_cast<float>( design_width );
		}

		enum class GameEvents : uint64_t
		{
			PLAYER_ROTATE_LEFT = 0,
			PLAYER_ROTATE_RIGHT,
			PLAYER_THRUST,
			PLAYER_SHOOT,
		};

		enum PhysicsCategoryBits : uint16_t
		{
			PHYSX_CAT_DEFAULT     = 0x0001,
			PHYSX_CAT_PLAYER      = 0x0002,
			PHYSX_CAT_SCREEN_ZONE = 0x0004,
			PHYSX_CAT_WARPABLE    = 0x0008,
			PHYSX_CAT_ENEMY       = 0x0010,
			PHYSX_CAT_BULLET      = 0x0020
		};
	}
}

#endif // _GAMECONFIG_H