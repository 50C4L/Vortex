#include "StatusPanel.h"

#include <ecs/ECS.h>
#include <ui/UIDataModel.h>

#include "../components/PlayerComponents.h"

using namespace vortex;

StatusPanel::StatusPanel( eage::ecs::ECSRegistry& registry, eage::ui::UIDataModel& model )
	: mRegistry( registry )
	, mModel( model )
{
	mModel.Declare( "kill_count", 0 );
}

void
StatusPanel::Update()
{
	for( auto [ entity, player ] : mRegistry.GetComponentMap<PlayerComponent>() )
	{
		mModel.Set( "kill_count", player.kill_count );
	}
}
