#include "stdafx.h"
#include "MeshSystem.h"
#include "../Engine/GameObjects/Model.h"
#include "../RenderCommands.h"
#include "ECS\Systems\Animation\AnimationSystem.h"
#include "../Engine/RenderData.h"
#include "../Engine/Managers/ModelManager.h"

#include <CU/Collision/Intersection.hpp>

namespace MeshSystem
{
	void RenderMeshes(ComponentArray<MeshComponent>& someMeshes, ComponentArray<TransformComponent>& someTransforms, ComponentArray<RenderTargetComponent>& someRenderTargets, ECS& anECS, v3f& aMapMax, v3f& aMapMin)
	{
		v3f min = v3f(FLT_MAX, FLT_MAX, FLT_MAX);
		v3f max = v3f(FLT_MIN, FLT_MIN, FLT_MIN);
		Engine::ModelManager* mm = EngineInterface::GetModelManager();
		MeshComponent* const meshList = someMeshes.GetComponents();
		TransformComponent* const transformList = someTransforms.GetComponents();
		RenderTargetComponent* const renderTargetList = someRenderTargets.GetComponents();
		for (Entity index = 0; index < MAX_ENTITIES; ++index)
		{
			const bool meshActive = someMeshes.IsActive(index);
			const bool transformActive = someTransforms.IsActive(index);
			const bool hasRenderTarget = someRenderTargets.IsActive(index);
			if (!meshActive || !transformActive) continue;

			TransformComponent& transform = transformList[index];
			MeshComponent& mesh = meshList[index];
			if (mesh.primitiveType != PrimitiveType_None || mm->GetModel(mesh.model) != nullptr)
			{
				MeshBuffererCommand command;
				command.renderUnique = mesh.renderUnique;
				command.effectData.effectColor = mesh.effectColor;
				command.effectData.outlineColor = mesh.outlineColor;
				command.effectData.tValue = mesh.effectTValue;
				command.effectData.modelIndex = index + 1;
				command.effectData.gBufferPSEffectIndex = mesh.psEffectIndex;
				if (mesh.primitiveType != PrimitiveType_None)
				{
					GUID key = NIL_UUID;
					key.Data4[0] = (unsigned char)mesh.primitiveType + 1;
					command.model = key;
				}
				else
				{
					command.model = mesh.model;
				}
				if(mesh.engineRenderTarget)
				{
					GUID key = NIL_UUID;
					key.Data4[0] = (unsigned char)18 + 1;
					command.model = key;
					//command.model = mm->GetRenderTarget();
				}
				Model* model = mm->GetModel(command.model);
				if (model)
				{
					// command.matrix = transform.matrix;
					command.transform = transform.transform;
					command.myCollider = model->GetCollider();

					// consider calculating all entities world position in one swell foop at the start of each frame and cacheing it.
					Entity parent = anECS.GetParent(index);
					while (parent < MAX_ENTITIES)
					{
						CU::Transform tr1, tr2;
						tr1 = command.transform;
						tr2 = transformList[parent].transform;
						tr2.SetScale({ 1, 1, 1 });
						command.transform = CU::Transform(tr1.GetMatrix() * tr2.GetMatrix());

						parent = anECS.GetParent(parent);
					}
					if (mesh.dynamic == false)
					{
						const CU::AABB3Df& modelBox = model->GetCollider();
						const v3f scale = command.transform.GetScale();

						CU::AABB3Df calculatedBox;
						calculatedBox.myMax = v3f::ComponentWiseVectorMul(scale, modelBox.myMax);
						calculatedBox.myMin = v3f::ComponentWiseVectorMul(scale, modelBox.myMin);
						calculatedBox.myMax += command.transform.GetPosition();
						calculatedBox.myMin += command.transform.GetPosition();

						if (calculatedBox.myMin.x < min.x) min.x = calculatedBox.myMin.x;
						if (calculatedBox.myMin.y < min.y) min.y = calculatedBox.myMin.y;
						if (calculatedBox.myMin.z < min.z) min.z = calculatedBox.myMin.z;

						if (calculatedBox.myMax.x > max.x) max.x = calculatedBox.myMax.x;
						if (calculatedBox.myMax.y > max.y) max.y = calculatedBox.myMax.y;
						if (calculatedBox.myMax.z > max.z) max.z = calculatedBox.myMax.z;
					}
					command.modelType = mesh.modelType;
					if (mesh.dynamic == false && (command.modelType & EModelType_STATIC) == false)
					{
						command.modelType |= EModelType_STATIC;
					}
					bool renderedRenderTarget = false;
					if (hasRenderTarget)
					{
						for (unsigned short i = 0; i < model->GetAmountOfSubModels(); i++)
						{
							if (model->GetModelData(i).myMaterial->myMaterialType == MaterialTypes::ERenderTarget)
							{
								command.renderUnique = true;
								Engine::RenderMesh(command, mm, false, &renderTargetList[index]);
								renderedRenderTarget = true;
								break;
							}
						}
					}
					if (renderedRenderTarget == false)
					{
						Engine::RenderMesh(command, mm);
					}
				}
			}
		}
		aMapMax = max;
		aMapMin = min;
	}
	void RenderAnimated(ComponentArray<AnimatedMultiMeshComponent>& someMultiMeshes, ComponentArray<AnimatedMeshComponent>& someMeshes, ComponentArray<TransformComponent>& someTransforms, ECS& anECS, float aDeltaTime)
	{
		Engine::ModelManager* mm = EngineInterface::GetModelManager();
		AnimatedMeshComponent* const meshList = someMeshes.GetComponents();
		AnimatedMultiMeshComponent* const multiMeshList = someMultiMeshes.GetComponents();
		TransformComponent* const transformList = someTransforms.GetComponents();
		unsigned int amountOfCulledEnemies = 0;
		for (Entity index = 0; index < MAX_ENTITIES; ++index)
		{
			if (someMultiMeshes.IsActive(index) && someTransforms.IsActive(index))
			{
				TransformComponent& transform = transformList[index];
				AnimatedMultiMeshComponent& mesh = multiMeshList[index];
				if (mm->GetAnimatedModel(mesh.model) != nullptr)
				{
					AnimatedMeshRenderCommand command;
					command.renderUnique = mesh.renderUnique;
					command.effectData.effectColor = mesh.effectColor;
					command.effectData.outlineColor = mesh.outlineColor;
					command.effectData.tValue = mesh.effectTValue;
					command.effectData.gBufferPSEffectIndex = mesh.psEffectIndex;
					command.model = mm->GetAnimatedModel(mesh.model);
					command.matrix = transform.matrix;
					command.transform = transform.transform;
					command.collider = command.model->GetCollider();
					command.effectData.modelIndex = index + 1;
					// consider calculating all entities world position in one swell foop at the start of each frame and cacheing it.
					Entity parent = anECS.GetParent(index);
					while (parent < MAX_ENTITIES)
					{
						command.matrix *= transformList[parent].matrix;

						parent = anECS.GetParent(parent);
					}
					v3f scale = command.matrix.GetScale();
					command.collider.myMax = v3f::ComponentWiseVectorMul(scale, command.collider.myMax);
					command.collider.myMin = v3f::ComponentWiseVectorMul(scale, command.collider.myMin);
					command.collider.myMax += command.matrix.GetTranslationVector();
					command.collider.myMin += command.matrix.GetTranslationVector();

					command.modelType = mesh.modelType;
					AnimationSystem::UpdateMultiAnimation(aDeltaTime, mesh, &command.boneTransformsFinal[0], 0);
					command.numberOfBones = command.model->GetAnimationData().myIndexedSkeleton.Size();
					Engine::RenderAnimatedModel(command);
				}

				if (mm->GetAnimatedModel(mesh.modelAccessory) != nullptr)
				{
					AnimatedMeshRenderCommand command;
					command.renderUnique = mesh.renderUnique;
					command.effectData.effectColor = {255, 255, 255, 255};
					command.effectData.outlineColor = mesh.outlineColor;
					command.effectData.tValue = mesh.effectTValue;
					command.effectData.gBufferPSEffectIndex = mesh.psEffectIndex;
					command.model = mm->GetAnimatedModel(mesh.modelAccessory);
					command.matrix = transform.matrix;
					command.transform = transform.transform;
					command.collider = command.model->GetCollider();
					command.effectData.modelIndex = index + 1;
					// consider calculating all entities world position in one swell foop at the start of each frame and cacheing it.
					Entity parent = anECS.GetParent(index);
					while (parent < MAX_ENTITIES)
					{
						command.matrix *= transformList[parent].matrix;

						parent = anECS.GetParent(parent);
					}
					v3f scale = command.matrix.GetScale();
					command.collider.myMax = v3f::ComponentWiseVectorMul(scale, command.collider.myMax);
					command.collider.myMin = v3f::ComponentWiseVectorMul(scale, command.collider.myMin);
					command.collider.myMax += command.matrix.GetTranslationVector();
					command.collider.myMin += command.matrix.GetTranslationVector();

					command.modelType = mesh.modelType;
					AnimationSystem::UpdateMultiAnimation(aDeltaTime, mesh, &command.boneTransformsFinal[0], 1);
					command.numberOfBones = command.model->GetAnimationData().myIndexedSkeleton.Size();
					Engine::RenderAnimatedModel(command);
				}
			}
			else
			{
				const bool meshActive = someMeshes.IsActive(index);
				const bool transformActive = someTransforms.IsActive(index);
				if (!meshActive || !transformActive) continue;

				TransformComponent& transform = transformList[index];
				AnimatedMeshComponent& mesh = meshList[index];
				if (mm->GetAnimatedModel(mesh.model) != nullptr)
				{
					AnimatedMeshRenderCommand command;
					command.renderUnique = mesh.renderUnique;
					command.effectData.effectColor = mesh.effectColor;
					command.effectData.outlineColor = mesh.outlineColor;
					command.effectData.tValue = mesh.effectTValue;
					command.effectData.gBufferPSEffectIndex = mesh.psEffectIndex;
					command.model = mm->GetAnimatedModel(mesh.model);
					command.matrix = transform.matrix;
					command.transform = transform.transform;
					command.collider = command.model->GetCollider();
					command.effectData.modelIndex = index + 1;
					// consider calculating all entities world position in one swell foop at the start of each frame and cacheing it.
					Entity parent = anECS.GetParent(index);
					while (parent < MAX_ENTITIES)
					{
						command.matrix *= transformList[parent].matrix;

						parent = anECS.GetParent(parent);
					}
					v3f scale = command.matrix.GetScale();
					command.collider.myMax = v3f::ComponentWiseVectorMul(scale, command.collider.myMax);
					command.collider.myMin = v3f::ComponentWiseVectorMul(scale, command.collider.myMin);
					command.collider.myMax += command.matrix.GetTranslationVector();
					command.collider.myMin += command.matrix.GetTranslationVector();

					command.modelType = mesh.modelType;
					AnimationSystem::UpdateAnimation(aDeltaTime, mesh, &command.boneTransformsFinal[0]);
					command.numberOfBones = command.model->GetAnimationData().myIndexedSkeleton.Size();
					Engine::RenderAnimatedModel(command);
				}
			}
		}
	}
}