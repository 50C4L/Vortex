#include "LevelingSystem.h"

#include <cmath>

#include <ecs/ECS.h>

#include "../components/ExperienceComponent.h"

using namespace vortex;

LevelingSystem::LevelingSystem( eage::ecs::ECSRegistry& registry )
	: mRegistry( registry )
{
}

void
LevelingSystem::Update()
{
	if( IsPaused() )
	{
		return;
	}

	for( auto [ entity, experience ] : mRegistry.GetComponentMap<ExperienceComponent>() )
	{
		(void)entity;

		experience.current_xp += experience.pending_xp;
		experience.pending_xp = 0;

		while( experience.current_xp >= experience.xp_to_lvl_up )
		{
			experience.current_xp -= experience.xp_to_lvl_up;
			++experience.current_level;
			experience.xp_to_lvl_up = static_cast<int>( std::ceil( experience.xp_to_lvl_up * 1.5 ) );
		}
	}
}
