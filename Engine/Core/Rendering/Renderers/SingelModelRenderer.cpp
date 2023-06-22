#include "stdafx.h"
#include "SingelModelRenderer.h"
#include "GameObjects\Model.h"
#include "..\Renderer.h"
#include "Managers\ModelManager.h"
#include "EngineInterface.h"
#include "Managers\Managers.h"
#include "Managers\ParticleManager.h"
#include "RenderData.h"
#include "Managers\ModelManager.h"
namespace Engine
{
	void SortMesh(MeshesToRender& meshesToFill, Model* aModelToSort, Camera& aCamera, ModelManager* aModelManager)
	{
		MeshRenderCommand command, commandPlane;
		command.model = aModelToSort;
		bool isCutout = false;
		bool isTransparent = false;
		bool isRenderTarget = false;
		Model* model = (aModelToSort);
		for (unsigned short subModel = 0; subModel < model->GetAmountOfSubModels(); subModel++)
		{
			Material* material = model->GetModelData(subModel).myMaterial;
			if (material->myMaterialType == MaterialTypes::EPBR_Transparent)
			{
				if (material->myIsCutOut)
				{
					isCutout = true;
				}
				isTransparent = true;
			}
			if (material->myMaterialType == MaterialTypes::ERenderTarget)
			{
				isRenderTarget = true;
			}
		}
		if (isRenderTarget)
		{
			meshesToFill.myRenderTargetMeshes.Add(command);
		}
		if (isTransparent)
		{
			if (isCutout)
			{
				meshesToFill.myUniqueTCutoutMeshes.Add(command);
			}
			else
			{
				meshesToFill.myUniqueForwardMeshes.Add(command);
			}
		}
		meshesToFill.myUniqueNormalMeshes.Add(command);

		commandPlane.matrix.SetTranslation(v3f(0, model->GetCollider().myMin.y, 0 ));
		commandPlane.model = aModelManager->GetModel(aModelManager->LoadPrimitive(PrimitiveType::PrimitiveType_Plane));
		meshesToFill.myUniqueStaticMeshes.Add(commandPlane);
	}
	void SortMeshAnimated(MeshesToRender& meshesToFill, ModelAnimated** aModelToSort, Camera& aCamera, m4f* aSkeleton, unsigned int aNumberOfModels, bool aRenderWithFloor, CU::Color aMeshColor)
	{
		AnimatedMeshRenderCommand command;
		CU::AABB3Df boundingVolume;
		for (unsigned int model = 0; model < aNumberOfModels; model++)
		{
			command.model = aModelToSort[model];
			command.numberOfBones = aModelToSort[model]->GetAnimationData().myIndexedSkeleton.Size();
			memcpy(&command.boneTransformsFinal[0], &aSkeleton[0], sizeof(m4f) * command.numberOfBones);
			bool isTransparent = false;
			bool isCutout = false;
			for (unsigned short subModel = 0; subModel < command.model->GetAmountOfSubModels(); subModel++)
			{
				Material* material = command.model->GetModelData(subModel).myMaterial;
				if (material->myMaterialType == MaterialTypes::EPBRTransparent_Anim)
				{
					if (material->myIsCutOut)
					{
						isCutout = true;
					}
					isTransparent = true;
				}
			}
			if (isTransparent)
			{
				if (isCutout)
				{
					meshesToFill.myUniqueTCutout_AnimMeshes.Add(command);
				}
				else
				{
					meshesToFill.myUniqueFWD_AnimMeshes.Add(command);
				}
			}
			if (command.model->GetCollider().myMin.x < boundingVolume.myMin.x) boundingVolume.myMin.x = command.model->GetCollider().myMin.x;
			if (command.model->GetCollider().myMin.y < boundingVolume.myMin.y) boundingVolume.myMin.y = command.model->GetCollider().myMin.y;
			if (command.model->GetCollider().myMin.z < boundingVolume.myMin.z) boundingVolume.myMin.z = command.model->GetCollider().myMin.z;
			if (model == 0)
			{
				command.effectData.effectColor = aMeshColor;
				command.effectData.gBufferPSEffectIndex = 2;
				command.effectData.tValue = 1.0f;
			}
			else
			{
				command.effectData.tValue = 0.f;
			}
			meshesToFill.myUniqueAnimatedNormalMeshes.Add(command);
		}
		if (aRenderWithFloor)
		{
			MeshRenderCommand commandPlane;
			commandPlane.matrix.SetTranslation(v3f(0, boundingVolume.myMin.y, 0));
			commandPlane.model = EngineInterface::GetModelManager()->GetModel(EngineInterface::GetModelManager()->LoadPrimitive(PrimitiveType::PrimitiveType_Plane));
			meshesToFill.myUniqueStaticMeshes.Add(commandPlane);
		}
	}

	void SortMeshParticle(MeshesToRender& meshesToFill, size_t aNumbOfCommands, ParticleRenderCommand* aParticle, Camera& aCamera, bool aRenderGround)
	{
		for (size_t i = 0; i < aNumbOfCommands; i++)
		{
			Material* material = aParticle[i].myMaterial;
			if (material->myMaterialType == MaterialTypes::EParticle_Default)
			{
				meshesToFill.particlesStandard.buffer[meshesToFill.particlesStandard.myNumberOfSystems++] = aParticle[i];
			}
			if (material->myMaterialType == MaterialTypes::EParticle_Glow)
			{
				meshesToFill.particlesGlowing.buffer[meshesToFill.particlesGlowing.myNumberOfSystems++] = aParticle[i];
			}
		}
		if (aRenderGround)
		{
			MeshRenderCommand commandPlane;
			commandPlane.matrix.SetTranslation(v3f(0, -100.f, 0));
			commandPlane.model = EngineInterface::GetModelManager()->GetModel(EngineInterface::GetModelManager()->LoadPrimitive(PrimitiveType::PrimitiveType_Plane));
			meshesToFill.myUniqueStaticMeshes.Add(commandPlane);
		}
	}
}