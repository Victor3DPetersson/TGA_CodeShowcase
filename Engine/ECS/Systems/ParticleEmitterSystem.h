#pragma once

#include "../ComponentArray.h"
#include "../Components.h"
#include "../ECS.h"

namespace ParticleEmitterSystem
{
	void RenderParticles(class ComponentArray<ParticleEmitterComponent>& emitters, class ComponentArray<ParticleMultiEmitterComponent>& multiEmitters,ComponentArray<TransformComponent>& someTransforms, ECS& anECS);
};