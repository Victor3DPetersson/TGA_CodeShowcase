#pragma once

#include "../../Components.h"
#include "../../ComponentArray.h"

namespace MeshSystem
{
	void RenderMeshes(ComponentArray<MeshComponent>& someMeshes, ComponentArray<TransformComponent>& someTransforms, ComponentArray<RenderTargetComponent>& someRenderTargets, ECS& anECS, v3f& aMapMax, v3f& aMapMin);
	void RenderAnimated(ComponentArray<AnimatedMultiMeshComponent>& someMultiMeshes, ComponentArray<AnimatedMeshComponent>& someMeshes, ComponentArray<TransformComponent>& someTransforms, ECS& anECS, float aDeltaTime);
}