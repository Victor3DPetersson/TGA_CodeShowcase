#include "stdafx.h"
#include "LightSystem.h"
#include "../RenderCommands.h"

#include "../../ECS.h"

#include "RenderData.h"

void LightSystem::RenderLights
(
	ComponentArray<PointLightComponent>& somePointLights,
	ComponentArray<SpotLightComponent>& someSpotLights,
	ComponentArray<TransformComponent>& someTransforms,
	ECS& anECS
)
{
	PointLightComponent* pointLightList = somePointLights.GetComponents();
	SpotLightComponent* spotLightList = someSpotLights.GetComponents();
	TransformComponent* transformList = someTransforms.GetComponents();

	CU::BitArray<MAX_ENTITIES> activeTransforms;

	for (Entity entity = 0U; entity < MAX_ENTITIES; ++entity)
	{
		if (someTransforms.IsActive(entity))
		{
			activeTransforms.Set(entity);
		}
	}

	for (Entity entity = 0U; entity < MAX_ENTITIES; ++entity)
	{
		const bool pointLightActive = somePointLights.IsActive(entity);
		if (!activeTransforms[entity] || !pointLightActive) continue;

		PointLightRenderCommand command;
		pointLightList[entity].range = CU::Max(0.10f, pointLightList[entity].range);
		command.color = pointLightList[entity].color;
		command.color[3] = pointLightList[entity].baseIntensity;
		command.range = pointLightList[entity].range * 100.0f;
		pointLightList[entity].lightSize = CU::Max(0.0f, pointLightList[entity].lightSize);
		command.size = pointLightList[entity].lightSize;
		if (pointLightList[entity].lightSize == 0)
		{
			command.size = 1.0f;
		}
		CU::Matrix4x4f mat = transformList[entity].transform.GetMatrix(true);

		// consider calculating all entities world position in one swell foop at the start of each frame and cacheing it n stuf.
		Entity parent = anECS.GetParent(entity);
		while (parent < MAX_ENTITIES)
		{
			mat *= transformList[parent].transform.GetMatrix();

			parent = anECS.GetParent(parent);
		}

		command.position = mat.GetTranslationVector() + pointLightList[entity].offset * (m3f)mat;
		Engine::RenderPointLight(command, (uint16_t)pointLightList[entity].shadowType, (uint16_t)entity);
	}
	m4f rotationM = m4f::CreateRotationAroundX(CU::AngleToRadian(0));
	for (Entity entity = 0U; entity < MAX_ENTITIES; ++entity)
	{
		const bool spotLightActive = someSpotLights.IsActive(entity);
		if (!activeTransforms[entity] || !spotLightActive) continue;

		SpotLightRenderCommand command;
		CU::Matrix4x4f mat = transformList[entity].matrix;
		spotLightList[entity].angle = CU::Max(1.0f, spotLightList[entity].angle);
		spotLightList[entity].angle = CU::Min(90.0f, spotLightList[entity].angle);
		spotLightList[entity].range = CU::Max(1.0f, spotLightList[entity].range);
		command.angle = CU::AngleToRadian(spotLightList[entity].angle);
		command.color = spotLightList[entity].color;
		command.color[3] = spotLightList[entity].baseIntensity;
		command.range = spotLightList[entity].range * 100.0f;
		command.size = spotLightList[entity].lightSize;
		spotLightList[entity].lightSize = CU::Max(0.0f, spotLightList[entity].lightSize);
		if (spotLightList[entity].lightSize == 0)
		{
			command.size = 1.0f;
		}

		// consider calculating all entities world position in one swell foop at the start of each frame and cacheing it n stuf.
		Entity parent = anECS.GetParent(entity);
		//m4f commandTrans;
		//commandTrans.SetTranslation(transformList[entity].transform.GetPosition() + spotLightList[entity].offset);
		//commandTrans.SetForwardVector(transformList[entity].transform.GetForward());
		CU::Transform finalTrans = transformList[entity].transform;
		while (parent < MAX_ENTITIES)
		{
			CU::Transform tr1, tr2;
			tr1 = finalTrans;
			tr2 = transformList[parent].transform;
			tr2.SetScale({ 1, 1, 1 });
			finalTrans = CU::Transform(tr1.GetMatrix() * tr2.GetMatrix());

			parent = anECS.GetParent(parent);
		}
		mat *= rotationM;
		command.position = finalTrans.GetPosition();
		command.direction = finalTrans.GetForward();
		//command.direction = command.direction;
		Engine::RenderSpotLight(command, (uint16_t)spotLightList[entity].shadowType, (uint16_t)entity);
	}
}