#include "stdafx.h"
#include "HandleRendererDataPackage.h"
#include "../Renderer.h"
#include "../Renderers/CubemapRenderer.h"
#include "RenderData.h"
#include "ConstantBufferManager.h"
#include "EffectResourceManager.h"
void Engine::HandleDataPackages(RenderData* aMainRenderBuffer, RenderData* aRenderBuffer, Renderer* aRenderer, ConstantBufferManager* aConstantBufferManager, MeshesToRender& meshesToFill, RenderStates* someRenderStates)
{
	unsigned int deletionCounter = (unsigned int)aRenderBuffer->numberOfDataPackages;
	for (size_t i = 0; i < aRenderBuffer->numberOfDataPackages; i++)
	{
		if (aRenderBuffer->dataPackages[i] == nullptr) { continue; }
		switch (aRenderBuffer->dataPackagesType[i])
		{
		case Engine::ELoadPackageTypes::LevelLightData:
		{	
			LevelLightExportData* data = (LevelLightExportData*)aRenderBuffer->dataPackages[i];
			LevelLightData* lightData = &data->light;
			CubemapRenderer* cubeRenderer = aRenderer->GetCubeMapRenderer();
			if (cubeRenderer->InitGPUAndRenderData())
			{
				if (lightData->numberOfGridsInLevel > 0)
				{
					for (unsigned int grid = 0; grid < lightData->numberOfGridsInLevel; grid++)
					{
						if (cubeRenderer->IsEditorInited() == false)
						{
							lightData->gridData[grid].gridRotation = m4f::GetFastInverse(lightData->gridData[grid].gridRotation);
						}
						*cubeRenderer->GetSHGridData(grid) = lightData->gridData[grid];
					}
					aConstantBufferManager->myGlobalFrameBufferData.SHGridAmount = lightData->numberOfGridsInLevel;
					if (cubeRenderer->IsEditorInited())
					{
						cubeRenderer->Debug_SetNumberOfBakeGrids(lightData->numberOfGridsInLevel);
						cubeRenderer->SetBakeDataWithGridData(lightData->SHData);
					}
					else
					{
						cubeRenderer->UpdateGridsGPUData((unsigned short)lightData->numberOfGridsInLevel);
					}
					if (lightData->SHData)
					{
						cubeRenderer->UpdateSHLightProbeData(lightData->totalNumberOfHarmonics, lightData->SHData);
						cubeRenderer->SetNumberOfLightProbesOnGPU(lightData->totalNumberOfHarmonics);
						SH* shData = (SH*)lightData->SHData;
						aConstantBufferManager->MapUnmapSHSet(lightData->SHData, lightData->totalNumberOfHarmonics);
					}
					for (unsigned int grid = 0; grid < cubeRenderer->GetNumberLightProbeGridsOnGPU(); grid++)
					{
						lightData->gridData[grid].gridRotation = m4f::GetFastInverse(lightData->gridData[grid].gridRotation);
					}
					delete[] lightData->SHData;
					lightData->SHData = nullptr;
				}
				if (lightData->rProbes.numberOfRProbes > 0)
				{
					cubeRenderer->BakeLevelReflectionProbes((unsigned int)lightData->rProbes.numberOfRProbes, lightData->rProbes.probes, false);
					aRenderer->SetWaitForBuffersToSwap(true);
				}
				else
				{
					cubeRenderer->DeInitGPUData();
				}
			}
			aConstantBufferManager->myPostProcessingData = data->post;
			aConstantBufferManager->MapUnmapPostProcessing();
			delete aRenderBuffer->dataPackages[i];
			deletionCounter--;
		}
		break;
		case Engine::ELoadPackageTypes::AddLevelLightData:
		{
			LevelLightData* lightData = (LevelLightData*)aRenderBuffer->dataPackages[i];
			CubemapRenderer* cubeRenderer = aRenderer->GetCubeMapRenderer();
			if (cubeRenderer->InitGPUAndRenderData())
			{
				if (lightData->numberOfGridsInLevel > 0)
				{
					for (unsigned int grid = cubeRenderer->GetNumberLightProbeGridsOnGPU(); grid < lightData->numberOfGridsInLevel + cubeRenderer->GetNumberLightProbeGridsOnGPU(); grid++)
					{
						lightData->gridData[grid - cubeRenderer->GetNumberLightProbeGridsOnGPU()].gridRotation = m4f::GetFastInverse(lightData->gridData[grid - cubeRenderer->GetNumberLightProbeGridsOnGPU()].gridRotation);
						*cubeRenderer->GetSHGridData(grid) = lightData->gridData[grid - cubeRenderer->GetNumberLightProbeGridsOnGPU()];
					}
					aConstantBufferManager->myGlobalFrameBufferData.SHGridAmount = cubeRenderer->GetNumberLightProbeGridsOnGPU() + lightData->numberOfGridsInLevel;
					cubeRenderer->UpdateGridsGPUData((unsigned short)(lightData->numberOfGridsInLevel + cubeRenderer->GetNumberLightProbeGridsOnGPU()));
					cubeRenderer->UpdateSHLightProbeData(lightData->totalNumberOfHarmonics + cubeRenderer->GetTotalNumberLightProbesOnGPU(), nullptr);
					cubeRenderer->SetNumberOfLightProbesOnGPU(cubeRenderer->GetTotalNumberLightProbesOnGPU() + lightData->totalNumberOfHarmonics);
					if (lightData->SHData)
					{
						SH* renderersSHData = cubeRenderer->GetSHLevelData();
						memcpy(&renderersSHData[cubeRenderer->GetTotalNumberLightProbesOnGPU() - lightData->totalNumberOfHarmonics], lightData->SHData, sizeof(SH) * lightData->totalNumberOfHarmonics);
						aConstantBufferManager->MapUnmapSHSet(renderersSHData, cubeRenderer->GetTotalNumberLightProbesOnGPU());
					}
					delete[] lightData->SHData;
					lightData->SHData = nullptr;
				}
				if (lightData->rProbes.numberOfRProbes > 0) 
				{
					cubeRenderer->BakeLevelReflectionProbes((unsigned int)lightData->rProbes.numberOfRProbes, lightData->rProbes.probes, true);
					aRenderer->SetWaitForBuffersToSwap(true);
				}
				else
				{
					cubeRenderer->DeInitGPUData();
				}
			}
			delete aRenderBuffer->dataPackages[i];
			deletionCounter--;
		}
		break;
		case Engine::ELoadPackageTypes::ReflectionProbe:
		{
			EditorReflectionProbe* reflectionProbeData = (EditorReflectionProbe*)aRenderBuffer->dataPackages[i];
			CubemapRenderer* cubeRenderer = aRenderer->GetCubeMapRenderer();
			std::atomic<unsigned int> drawCalls = 0;
			cubeRenderer->RenderCubeMapAndSH(aMainRenderBuffer, reflectionProbeData->transform.GetPosition(), meshesToFill, someRenderStates, drawCalls, true, reflectionProbeData->outerRadius * 2.0f);
			delete aRenderBuffer->dataPackages[i];
			deletionCounter--;
		}
		break;
		case Engine::ELoadPackageTypes::BakeReflectionProbes:
		{
			ReflectionProbesData* reflectionProbeData = (ReflectionProbesData*)aRenderBuffer->dataPackages[i];
			CubemapRenderer* cubeRenderer = aRenderer->GetCubeMapRenderer();
			cubeRenderer->BakeLevelReflectionProbes((unsigned int)reflectionProbeData->numberOfRProbes, reflectionProbeData->probes, false);
			delete aRenderBuffer->dataPackages[i];
			aRenderer->SetWaitForBuffersToSwap(true);
			deletionCounter--;
		}
			break;
		default:
		break;
		}
		aRenderBuffer->dataPackages[i] = nullptr;
	}
	assert(deletionCounter == 0 && "Failed to delete all the data packages");
	aRenderBuffer->numberOfDataPackages = 0;

}

void Engine::HandlePreRenderDataPackages(RenderData* aMainRenderBuffer, RenderData* aRenderBuffer, Renderer* aRenderer, ConstantBufferManager* aConstantBufferManager, MeshesToRender& meshesToFill, RenderStates* someRenderStates, EffectResourceManager* aEffectManager)
{
	bool shouldDeInitEffectManager = false;
	int counter = 0;
	for (size_t i = 0; i < aRenderBuffer->numberOfDataPackages; i++)
	{
		if (aRenderBuffer->dataPackages[i] == nullptr) { continue; }
		/*switch (aRenderBuffer->dataPackagesType[i])
		{
		case Engine::ELoadPackageTypes::PlayerPortraits:
		{
			if (shouldDeInitEffectManager == false) { aEffectManager->Icon_InitResources(); shouldDeInitEffectManager = true; }
			aEffectManager->Icon_Render(aRenderBuffer->dataPackages[i], (unsigned int)ELoadPackageTypes::PlayerPortraits);
			delete aRenderBuffer->dataPackages[i];
			counter++;
			aRenderBuffer->dataPackages[i] = nullptr;
		}
		break;
		case Engine::ELoadPackageTypes::PresetIcon:
		{
			if (shouldDeInitEffectManager == false) { aEffectManager->Icon_InitResources(); shouldDeInitEffectManager = true; }
			aEffectManager->Icon_Render(aRenderBuffer->dataPackages[i], (unsigned int)ELoadPackageTypes::PresetIcon);
			delete aRenderBuffer->dataPackages[i];
			counter++;
			aRenderBuffer->dataPackages[i] = nullptr;
		}
		break;
		case Engine::ELoadPackageTypes::AccessoryIcon:
		{
			if (shouldDeInitEffectManager == false) { aEffectManager->Icon_InitResources(); shouldDeInitEffectManager = true; }
			aEffectManager->Icon_Render(aRenderBuffer->dataPackages[i], (unsigned int)ELoadPackageTypes::AccessoryIcon);
			delete aRenderBuffer->dataPackages[i];
			counter++;
			aRenderBuffer->dataPackages[i] = nullptr;
		}
		break;
		default:
			break;
		}*/
	}
	if (counter > 0)
	{
		for (size_t i = counter; i < aRenderBuffer->numberOfDataPackages; i++)
		{
			aRenderBuffer->dataPackages[i - counter] = aRenderBuffer->dataPackages[i];
			aRenderBuffer->dataPackagesType[i - counter] = aRenderBuffer->dataPackagesType[i];
			aRenderBuffer->dataPackages[i] = nullptr;
		}
	}

	aRenderBuffer->numberOfDataPackages -= counter;
	if (shouldDeInitEffectManager) { aEffectManager->Icon_ReleaseResources(); }
}
