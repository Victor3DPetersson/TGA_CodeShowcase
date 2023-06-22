#include "stdafx.h"
#include "DrawColliderSystem.h"
#include "ECS/ECS.h"
#include "EngineInterface.h"

void DrawColliderSystem::Draw(ComponentArray<BoxColliderComponent>& someBoxColliders, ComponentArray<SphereColliderComponent>& someSphereColliders,
	ComponentArray<PlaneColliderComponent>& somePlaneColliders, ComponentArray<TransformComponent>& somTransforms, ECS& anECS)
{
	for (Entity entity = 0; entity < MAX_ENTITIES; entity++)
	{
		if (!somTransforms.IsActive(entity))
			continue;

		TransformComponent& transform = somTransforms.GetComponent(entity);
		CU::Transform childTransform = transform.transform;
		Entity parent = anECS.GetParent(entity);
		while (parent < MAX_ENTITIES)
		{
			CU::Transform tr1, tr2;
			tr1 = childTransform;
			tr2 = somTransforms.GetComponent(parent).transform;
			tr2.SetScale({ 1, 1, 1 });
			childTransform = CU::Transform(tr1.GetMatrix() * tr2.GetMatrix());

			parent = anECS.GetParent(parent);
		}

		if (someBoxColliders.IsActive(entity))
		{
			BoxColliderComponent& box = someBoxColliders.GetComponent(entity);
			childTransform.SetScale({ 1, 1, 1 });
			EngineInterface::DrawBoxRotated(childTransform.GetPosition() + box.offset, box.boxSize, childTransform.GetMatrix(), 1.0f, {0, 0, 255, 255});
		}

		if (someSphereColliders.IsActive(entity))
		{
			SphereColliderComponent& sph = someSphereColliders.GetComponent(entity);
			childTransform.SetScale({ 1, 1, 1 });
			EngineInterface::DrawSphere(childTransform.GetPosition()+ sph.offset, sph.radius * 0.01f * 2.0f, {255, 0, 0, 255});
		}

		if (somePlaneColliders.IsActive(entity))
		{
			PlaneColliderComponent& plane = somePlaneColliders.GetComponent(entity);
			v2f scale = { childTransform.GetScale().x,  childTransform.GetScale().z};
			v2f size = v2f(plane.size.x, plane.size.y);
			childTransform.SetScale({ 1, 1, 1 });
			EngineInterface::DrawPlane(childTransform.GetPosition() + plane.offset, size, childTransform.GetMatrix(), { 255, 0, 0, 255 });
		}
	}
}
