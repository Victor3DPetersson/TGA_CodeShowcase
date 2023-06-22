#include "stdafx.h"
#include "RenderData.h"

#include "../Engine/GameObjects/Model.h"

#include "Core/Rendering/Renderer.h"
#include "EngineInterface.h"
#include "Managers\ModelManager.h"
#include "Managers\MaterialManager.h"
#include "ECS\Components.h"
#include "EngineInterface.h"
#include "Core\Rendering/Resources\FullScreenTexture.h"
#include <Loa/EngineLog.hpp>

namespace Engine
{
	GlobalRenderData globalRenderData;
	RenderData* GetWriteGameplayBuffer()
	{
		return &globalRenderData.renderBuffersData[globalRenderData.bufferWriteIndex];
	}
	RenderData* GetReadBuffer()
	{
		return &globalRenderData.renderBuffersData[globalRenderData.bufferReadIndex];
	}
	RenderData* GetLastReadBuffer()
	{
		return &globalRenderData.renderBuffersData[globalRenderData.bufferLastReadIndex];
	}
	void RenderMesh(MeshBuffererCommand cmd, ModelManager* aModelManager, bool isEnv, RenderTargetComponent* aRenderTargetComponent)
	{
		RenderData* buf = GetWriteGameplayBuffer();

		if (cmd.renderUnique == false || isEnv)
		{
			Model* model = aModelManager->GetModel(cmd.model);
			for (unsigned short i = 0; i < model->GetAmountOfSubModels(); i++)
			{
				unsigned short renderIndex = model->GetModelData(i).GetRenderIndex();
				if (renderIndex >= MAX_SORTED_MESHES)
				{
					LogInfo("Too many models, increase amount of Instance Mesh Types");
					continue;
				}
				unsigned short& modelCount = buf->sortedMeshes[renderIndex].numberOfModels;
				buf->sortedMeshes[renderIndex].model = cmd.model;
				buf->sortedMeshes[renderIndex].submodelIndex = i;
				buf->sortedMeshes[renderIndex].modelType = (cmd.modelType);
				buf->sortedMeshes[renderIndex].transforms[modelCount] = (cmd.matrix);
				buf->sortedMeshes[renderIndex].gameTransform[modelCount] = cmd.transform;
				buf->sortedMeshes[renderIndex].effectData[modelCount] = cmd.effectData;
				if (cmd.effectData.gBufferPSEffectIndex == 0 && model->GetModelData(i).myMaterial->myPSEffectIndex != 0)
				{
					buf->sortedMeshes[renderIndex].effectData[modelCount].gBufferPSEffectIndex = model->GetModelData(i).myMaterial->myPSEffectIndex * 2;
				}
				modelCount++;
			}
		}
		else
		{
			assert(("Oh no, I peed my pants.", buf->uniqueMeshesSize < MAX_UNIQUE_MESHES));
			if (aRenderTargetComponent && aRenderTargetComponent->CameraIndex != RenderCamera_None)
			{
				buf->renderTargetcameraFlags.Set(PO2(aRenderTargetComponent->CameraIndex - 1));
				buf->renderTargetCameras[(int)aRenderTargetComponent->CameraIndex - 1].texture = EngineInterface::GetMaterialManager()->GetRenderTarget(aRenderTargetComponent->CameraIndex - 1, aRenderTargetComponent->Resolution);
				buf->renderTargetCameras[(int)aRenderTargetComponent->CameraIndex - 1].depthTexture = EngineInterface::GetMaterialManager()->GetRenderTargetDepth(aRenderTargetComponent->Resolution);
				buf->renderTargetCameras[(int)aRenderTargetComponent->CameraIndex - 1].intermediateTexture = EngineInterface::GetMaterialManager()->GetRenderTargetIntermediate(aRenderTargetComponent->Resolution);
				buf->renderTargetCameras[(int)aRenderTargetComponent->CameraIndex - 1].gBufferTexture = EngineInterface::GetMaterialManager()->GetRenderTargetGBuffer(aRenderTargetComponent->Resolution);
				buf->renderTargetCameras[(int)aRenderTargetComponent->CameraIndex - 1].renderFlag = aRenderTargetComponent->CameraRenderFlag;
				cmd.effectData.gBufferPSEffectIndex = aRenderTargetComponent->CameraIndex - 1;
			}
			buf->uniqueMeshes[buf->uniqueMeshesSize++] = cmd;
			aModelManager->IncrementModelCounter(cmd.model);
		}
	}
	void RenderAnimatedModel(AnimatedMeshRenderCommand aCommand)
	{
		RenderData* buf = GetWriteGameplayBuffer();

		if (aCommand.renderUnique)
		{
			assert(("Crikey, I wet 'em.", buf->uniqueAnimatedMeshesSize < MAX_ANIM_UNIQUE_MESHES));
			buf->uniqueAnimatedMeshes[buf->uniqueAnimatedMeshesSize++] = aCommand;
		}
		else
		{
			for (unsigned short i = 0; i < aCommand.model->GetAmountOfSubModels(); i++)
			{
				const unsigned short renderIndex = aCommand.model->GetModelData(i).GetRenderIndex();
				unsigned short& modelCount = buf->sortedAnimMeshes[renderIndex].numberOfModels;
				memcpy(&buf->sortedAnimMeshes[renderIndex].
					boneTransforms[modelCount * 128], &aCommand.boneTransformsFinal[0], sizeof(m4f) * aCommand.numberOfBones);
				buf->sortedAnimMeshes[renderIndex].modelType = (aCommand.modelType);
				buf->sortedAnimMeshes[renderIndex].model = &aCommand.model->GetModelData(i);
				buf->sortedAnimMeshes[renderIndex].numberOfBones = aCommand.numberOfBones;
				buf->sortedAnimMeshes[renderIndex].gameTransform[modelCount] = aCommand.transform;
				buf->sortedAnimMeshes[renderIndex].effectData[modelCount] = aCommand.effectData;
				if (aCommand.effectData.gBufferPSEffectIndex == 0 && aCommand.model->GetModelData(i).myMaterial->myPSEffectIndex != 0)
				{
					buf->sortedMeshes[renderIndex].effectData[modelCount].gBufferPSEffectIndex = aCommand.model->GetModelData(i).myMaterial->myPSEffectIndex * 2;
				}
				modelCount++;
			}
		}
	}
	void RenderPointLight(PointLightRenderCommand aLightCommand, const uint16_t aLightRenderState, uint16_t aID)
	{
		RenderData* buf = GetWriteGameplayBuffer();
		assert(("Walla, I hope my pants dry before anyone notices.", buf->pointLightsSize < MAX_POINT_LIGHTS));
		buf->pointlightShadowCaster[buf->pointLightsSize] = aLightRenderState;
		buf->pointLightIDs[buf->pointLightsSize] = aID;
		buf->pointLights[buf->pointLightsSize++] = aLightCommand;
	}
	void RenderSpotLight(SpotLightRenderCommand aLightCommand, const uint16_t aLightRenderState, uint16_t aID)
	{
		RenderData* buf = GetWriteGameplayBuffer();
		assert(("Once a pantswetter, always a pantswetter.", buf->spotLightsSize < MAX_SPOT_LIGHTS));
		buf->spotlightsShadowCaster[buf->spotLightsSize] = aLightRenderState;
		buf->spotlightIDs[buf->spotLightsSize] = aID;
		buf->spotLights[buf->spotLightsSize++] = aLightCommand;
	}
	void RenderSprite(SpriteCommand aSpriteToRender)
	{
		RenderData* buf = GetWriteGameplayBuffer();
		assert(("Mum, I wet the bed again!", buf->spritesSize < MAX_SPRITES));
		buf->sprites[buf->spritesSize++] = aSpriteToRender;
		
	}
	void RenderWorldSprite(WorldSpriteCommandID aWorldSpriteToRender)
	{
		RenderData* buf = GetWriteGameplayBuffer();
		assert(("PEEEEEEEEEEEEEEEEEEEEEE", buf->wspritesSize < MAX_WSPRITES));
		buf->wsprites[buf->wspritesSize++] = WorldSpriteCommand::Get(aWorldSpriteToRender);
	}
	void RenderParticle(ParticleCommand aParticleCommand)
	{
		RenderData* buf = GetWriteGameplayBuffer();
		assert(("That wet sensation never gets old.", buf->particlesSize < MAX_PARTICLES));
		buf->particles[buf->particlesSize++] = aParticleCommand;
	}
	void RenderDecal(DecalCommand aDecalToRender)
	{
		RenderData* buf = GetWriteGameplayBuffer();
		assert(("Pee n poo me paants.", buf->decalsSize < MAX_DECALS));
		buf->decals[buf->decalsSize++] = aDecalToRender;
	}

	void AddDataPackageToRenderer(ELoadPackageTypes aType, void* data)
	{
		RenderData* buf = GetWriteGameplayBuffer();
		buf->dataPackages[buf->numberOfDataPackages] = data;
		buf->dataPackagesType[buf->numberOfDataPackages++] = aType;
	}

	void UpdateRenderTargetCameras()
	{
		v2f res = EngineInterface::GetRenderResolution();

		RenderData* buf = GetWriteGameplayBuffer();
		for (size_t i = 0; i < NUMBOF_RENDERTARGETS; i++)
		{
			if (buf->renderTargetcameraFlags[PO2(i)])
			{
				buf->renderTargetCameras[i].camera.RecalculateProjectionMatrix((*buf->renderTargetCameras[i].texture)->GetResolution());
			}
		}
	}
	void SortMeshBuffer()
	{
		ModelManager* mm = EngineInterface::GetModelManager();
		mm->SortAllMeshes();
		RenderData* buf = GetWriteGameplayBuffer();
		buf->sortedMeshesSize = mm->GetAmountOfModels();
		memset(&buf->sortedMeshes, 0, buf->sortedMeshesSize * sizeof(SortedModelDataForBuffers));
		buf->sortedAnimMeshesSize = mm->GetAmountOfAnimModels();
		memset(&buf->sortedAnimMeshes, 0, buf->sortedAnimMeshesSize * sizeof(SortedAnimationDataForBuffers));
		globalRenderData.gameplayFlags &= ~GlobalRenderData::GameplayFlags_ModelBuffersAreAdjusted;
	}
	bool SwapAndClearGameplayBuffers()
	{
		if (globalRenderData.renderThreadIsCopying.test_and_set())
		{
			return false;
		}
		globalRenderData.renderThreadIsCopying.clear();
		++globalRenderData.step;
		//RenderData* buf = GetLastReadBuffer();
		if (GetWriteGameplayBuffer()->loadedLevel)
		{
			for (size_t i = 0; i < NUMBOF_RENDERTARGETS; i++)
			{
				globalRenderData.renderBuffersData[globalRenderData.bufferReadIndex].renderTargetCameras[i] = globalRenderData.renderBuffersData[globalRenderData.bufferWriteIndex].renderTargetCameras[i];
				globalRenderData.renderBuffersData[globalRenderData.bufferLastReadIndex].renderTargetCameras[i] = globalRenderData.renderBuffersData[globalRenderData.bufferWriteIndex].renderTargetCameras[i];
			}
			globalRenderData.renderBuffersData[globalRenderData.bufferWriteIndex].loadedLevel = false;
		}

		ModelManager* mm = EngineInterface::GetModelManager();
		RenderData* lastReadBuff = GetLastReadBuffer();

		for (size_t i = 0; i < lastReadBuff->sortedMeshesSize; i++)
		{
			if (lastReadBuff->sortedMeshes[i].numberOfModels > 0)
			{
				mm->DecrementModelCounter(lastReadBuff->sortedMeshes[i].model);
			}
		}
		for (size_t i = 0; i < lastReadBuff->uniqueMeshesSize; i++)
		{
			mm->DecrementModelCounter(lastReadBuff->uniqueMeshes[i].model);
		}

		size_t lastRead = globalRenderData.bufferLastReadIndex;
		globalRenderData.bufferLastReadIndex = globalRenderData.bufferReadIndex;
		globalRenderData.bufferReadIndex = globalRenderData.bufferWriteIndex;
		globalRenderData.bufferWriteIndex = lastRead;

		RenderData* readBuf = GetReadBuffer();
		for (size_t i = 0; i < readBuf->sortedMeshesSize; i++)
		{
			if (readBuf->sortedMeshes[i].numberOfModels > 0)
			{
				mm->IncrementModelCounter(readBuf->sortedMeshes[i].model);
			}
		}
		//if (buf->resolutionChanged)
		{
		//	GetReadGameplayBuffer()->resolutionChanged = true;
		}

		return true;
	}

}