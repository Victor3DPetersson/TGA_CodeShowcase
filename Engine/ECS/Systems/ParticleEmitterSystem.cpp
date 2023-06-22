#include "stdafx.h"
#include "ParticleEmitterSystem.h"

#include "../Engine/ECS/ECS.h"

#include "../Engine/ECS/Systems/TransformSystem/TransformSystem.h"

#include "RenderData.h"
#include "EngineInterface.h"
#include "Managers\Managers.h"
#include "Managers\ParticleManager.h"

void ParticleEmitterSystem::RenderParticles(ComponentArray<ParticleEmitterComponent>& emitters, ComponentArray<ParticleMultiEmitterComponent>& multiEmitters, ComponentArray<TransformComponent>& someTransforms, ECS& anECS)
{
	TransformComponent* const transformList = someTransforms.GetComponents();
	Engine::ParticleManager* pm = &EngineInterface::GetManagers()->myParticleManager;
	for (Entity entity = 0; entity < MAX_ENTITIES; entity++)
	{
		const bool particleActive = emitters.IsActive(entity);
		const bool particleMultiActive = multiEmitters.IsActive(entity);
		const bool transformActive = someTransforms.IsActive(entity);



		if (transformActive == false || particleActive == false && particleMultiActive == false) continue;

		ParticleCommand command;
		Entity parent = anECS.GetParent(entity);
		command.myTransform = someTransforms.GetComponent(entity).transform;

		if (parent != INVALID_ENTITY)
		{
			while (parent < MAX_ENTITIES)
			{
				CU::Transform tr1, tr2;
				tr1 = command.myTransform;
				tr2 = transformList[parent].transform;
				tr2.SetScale({ 1, 1, 1 });
				command.myTransform = CU::Transform(tr1.GetMatrix() * tr2.GetMatrix());
				parent = anECS.GetParent(parent);
			}

		}
		const v3f prePosition = command.myTransform.GetPosition();
		if (particleActive)
		{
			ParticleEmitterComponent& particleComponent = emitters.GetComponent(entity);
			if (particleComponent.particleSystem == NIL_UUID || particleComponent.renderIndex == _UI16_MAX || particleComponent.isPlaying == false)
			{
				continue;
			}
			command.myEmitterIndex = particleComponent.renderIndex;
			command.mySystemIndex = particleComponent.particleSystem;
			command.myTransform.AddToPosition(particleComponent.offset * (m3f)command.myTransform.GetMatrix());
			command.shouldSpawn = particleComponent.shouldSpawn;
			Engine::RenderParticle(command);
			command.myTransform.SetPosition(prePosition);
		}
		if (particleMultiActive)
		{
			ParticleMultiEmitterComponent& particleMultiComp = multiEmitters.GetComponent(entity);
			if (particleMultiComp.particleSystem1 != NIL_UUID && particleMultiComp.renderIndex1 != _UI16_MAX && particleMultiComp.isPlaying1)
			{
				command.myEmitterIndex = particleMultiComp.renderIndex1;
				command.mySystemIndex = particleMultiComp.particleSystem1;
				command.myTransform.AddToPosition(particleMultiComp.offset1 * (m3f)command.myTransform.GetMatrix());
				command.shouldSpawn = particleMultiComp.shouldSpawn1;
				Engine::RenderParticle(command);
				command.myTransform.SetPosition(prePosition);
			}
			if (particleMultiComp.particleSystem2 != NIL_UUID && particleMultiComp.renderIndex2 != _UI16_MAX && particleMultiComp.isPlaying2)
			{
				command.myEmitterIndex = particleMultiComp.renderIndex2;
				command.mySystemIndex = particleMultiComp.particleSystem2;
				command.myTransform.AddToPosition(particleMultiComp.offset2 * (m3f)command.myTransform.GetMatrix());
				command.shouldSpawn = particleMultiComp.shouldSpawn2;
				Engine::RenderParticle(command);
				command.myTransform.SetPosition(prePosition);
			}
			if (particleMultiComp.particleSystem3 != NIL_UUID && particleMultiComp.renderIndex3 != _UI16_MAX && particleMultiComp.isPlaying3)
			{
				command.myEmitterIndex = particleMultiComp.renderIndex3;
				command.mySystemIndex = particleMultiComp.particleSystem3;
				command.myTransform.AddToPosition(particleMultiComp.offset3 * (m3f)command.myTransform.GetMatrix());
				command.shouldSpawn = particleMultiComp.shouldSpawn3;
				Engine::RenderParticle(command);
				command.myTransform.SetPosition(prePosition);
			}
		}
	}
}