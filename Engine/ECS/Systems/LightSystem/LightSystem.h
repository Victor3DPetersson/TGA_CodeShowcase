#pragma once

#include "../../ComponentArray.h"
#include "../../Components.h"


namespace LightSystem
{
	void RenderLights
	(
		ComponentArray<PointLightComponent>& somePointLights,
		ComponentArray<SpotLightComponent>& someSpotLights,
		ComponentArray<TransformComponent>& someTransforms,
		ECS& anECS
	);
}