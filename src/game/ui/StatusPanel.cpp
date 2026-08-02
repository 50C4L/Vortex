#include "StatusPanel.h"

#include <ecs/ECS.h>
#include <ui/UIDataModel.h>

#include "../components/ExperienceComponent.h"
#include "../components/PlayerComponents.h"


using namespace vortex;

StatusPanel::StatusPanel( eage::ecs::ECSRegistry& registry, eage::ui::UIDataModel& model )
	: mRegistry( registry )
	, mModel( model )
{
	mModel.Declare( "current_level", 1 );
	mModel.Declare( "current_xp", 0 );
	mModel.Declare( "xp_to_lvl_up", 5 );
	mModel.Declare( "kill_count", 0 );
}

void
StatusPanel::Update()
{
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
