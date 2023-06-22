#include "stdafx.h"
#include "DecalSystem.h"
#include "ECS/Components.h"
#include "ECS/ComponentArray.h"
#include "Managers\MaterialManager.h"
#include "EngineInterface.h"
#include "RenderData.h"

void DecalSystem::RenderDecals(ComponentArray<DecalComponent>& decals, ComponentArray<TransformComponent>& someTransforms, ECS& anECS)
{
	DecalCommand command;
	for (Entity entity = 0; entity < MAX_ENTITIES; entity++)
	{
		if (decals.IsActive(entity) && someTransforms.IsActive(entity))
		{
			command.gameTransform = someTransforms.GetComponent(entity).transform;
			command.material = EngineInterface::GetMaterialManager()->GetGratPlat(decals.GetComponent(entity).material, MaterialTypes::EDecal);
			if (command.material)
			{
				Engine::RenderDecal(command);
			}
		}
	}
}
