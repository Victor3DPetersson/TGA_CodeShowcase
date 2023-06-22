#pragma once

#include "../Engine/ECS/ComponentArray.h"
#include "../Engine/ECS/Components.h"

namespace DecalSystem
{
	void RenderDecals(ComponentArray<DecalComponent>& decals, ComponentArray<TransformComponent>& someTransforms, ECS& anECS);
};

