#include "StatusPanel.h"

#include <string>

#include <ecs/ECS.h>
#include <ui/UIDataModel.h>

#include "../components/ExperienceComponent.h"
#include "../components/PlayerComponents.h"
#include "../systems/WaveSystem.h"


using namespace vortex;

StatusPanel::StatusPanel( eage::ecs::ECSRegistry& registry, eage::ui::UIDataModel& model, WaveSystem& wave_system )
	: mRegistry( registry )
	, mModel( model )
	, mWaveSystem( wave_system )
{
	mModel.Declare( "current_level", 1 );
	mModel.Declare( "current_xp", 0 );
	mModel.Declare( "xp_to_lvl_up", 5 );
	mModel.Declare( "kill_count", 0 );
	mModel.Declare( "wave_time_remaining", 0 );
	mModel.Declare( "wave_banner_text", std::string() );
	mModel.Declare( "wave_banner_visible", false );
}

void
StatusPanel::Update()
{
	const WaveHudState hud = mWaveSystem.GetHudState();
	mModel.Set( "wave_time_remaining", hud.time_remaining_sec );
	mModel.Set( "wave_banner_text", hud.banner_text );
	mModel.Set( "wave_banner_visible", hud.banner_visible );

	for( auto [ entity, player ] : mRegistry.GetComponentMap<PlayerComponent>() )
	{
		mModel.Set( "kill_count", player.kill_count );

		if( mRegistry.HasComponent<ExperienceComponent>( entity ) )
		{
			const auto& experience = mRegistry.GetComponent<ExperienceComponent>( entity );
			mModel.Set( "current_level", experience.current_level );
			mModel.Set( "current_xp", experience.current_xp );
			mModel.Set( "xp_to_lvl_up", experience.xp_to_lvl_up );
		}
	}
}
