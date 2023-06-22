#pragma once
#include "../Engine/ECS/ComponentArray.h"
#include "../Engine/ECS/Components.h"

namespace DrawColliderSystem
{
	void Draw(ComponentArray<BoxColliderComponent>& someBoxColliders, ComponentArray<SphereColliderComponent>& someSphereColliderComponents,
		ComponentArray<PlaneColliderComponent>& somePlaneColliders, ComponentArray<TransformComponent>& somTransforms, ECS& anECS);
}