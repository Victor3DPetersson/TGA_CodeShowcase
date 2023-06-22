#include "stdafx.h"
#include "LerpRenderBuffers.h"
#include "RenderData.h"
#include "CU\Containers\ThreadPool.h"
#include "EngineInterface.h"
#include "..\CommonUtilities\CU\Collision\Intersection.hpp"
#include "../Resources\ConstantBufferManager.h"
#include "GameObjects\Camera.h"
#include "MeshStruct.h"
#include "GameObjects\Model.h"
#include "Managers\ModelManager.h"

//#include "pix3.h"
namespace Engine
{
	void LerpBuffers(RenderData* aFrom, RenderData* aTo, float aT, RenderData* aResult, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aJobCounter, ModelManager* aModelManager)
	{
		aJobCounter = 8;
		CU::ThreadPool* tpool = EngineInterface::GetThreadPool();
		//Point Lights
		tpool->AddJob([&aFrom, &aTo, aT, &aResult, &aJobCounter]() {
			aResult->pointLightsSize = 0;
			for (size_t i = 0; i < aFrom->pointLightsSize; i++)
			{
				if (aFrom->pointLightIDs[i] == aTo->pointLightIDs[i])
				{
					aResult->pointLights[i].color.LerpColor(aFrom->pointLights[i].color, aTo->pointLights[i].color, aT);
					aResult->pointLights[i].position = v3f::ComponentWiseLerp(aFrom->pointLights[i].position, aTo->pointLights[i].position, aT);
					aResult->pointLights[i].range = CU::Lerp(aFrom->pointLights[i].range, aTo->pointLights[i].range, aT);
					aResult->pointLights[i].size = aFrom->pointLights[i].size;
					aResult->pointlightShadowCaster[i] = aFrom->pointlightShadowCaster[i];
					aResult->pointLightIDs[i] = aFrom->pointLightIDs[i];
					aResult->pointLightsSize++;
				}
				else
				{
					bool lightFound = false;
					for (size_t missingLight = i; missingLight < aTo->pointLightsSize; missingLight++)
					{
						if (aFrom->pointLightIDs[i] == aTo->pointLightIDs[missingLight])
						{
							aResult->pointLights[i].color.LerpColor(aFrom->pointLights[i].color, aTo->pointLights[missingLight].color, aT);
							aResult->pointLights[i].position = v3f::ComponentWiseLerp(aFrom->pointLights[i].position, aTo->pointLights[missingLight].position, aT);
							aResult->pointLights[i].range = CU::Lerp(aFrom->pointLights[i].range, aTo->pointLights[missingLight].range, aT);
							aResult->pointLights[i].size = aFrom->pointLights[i].size;
							aResult->pointlightShadowCaster[i] = aFrom->pointlightShadowCaster[i];
							aResult->pointLightIDs[i] = aFrom->pointLightIDs[i];
							aResult->pointLightsSize++;
							lightFound = true;
							break;
						}
					}
					if (!lightFound)
					{
						aResult->pointLights[i].color = aFrom->pointLights[i].color;
						aResult->pointLights[i].position = aFrom->pointLights[i].position;
						aResult->pointLights[i].range = aFrom->pointLights[i].range;
						aResult->pointLights[i].size = aFrom->pointLights[i].size;
						aResult->pointlightShadowCaster[i] = aFrom->pointlightShadowCaster[i];
						aResult->pointLightIDs[i] = aFrom->pointLightIDs[i];
						aResult->pointLightsSize++;
					}
				}

			}
			aJobCounter--;
			});
		//Spotlights
		tpool->AddJob([aFrom, aTo, aT, aResult, &aJobCounter]() {
			aResult->spotLightsSize = 0;
			for (size_t i = 0; i < aFrom->spotLightsSize; i++)
			{
				if (aFrom->spotlightIDs[i] == aTo->spotlightIDs[i])
				{
					aResult->spotLights[i].color.LerpColor(aFrom->spotLights[i].color, aTo->spotLights[i].color, aT);
					aResult->spotLights[i].position = v3f::ComponentWiseLerp(aFrom->spotLights[i].position, aTo->spotLights[i].position, aT);
					aResult->spotLights[i].direction = v3f::ComponentWiseLerp(aFrom->spotLights[i].direction, aTo->spotLights[i].direction, aT);
					aResult->spotLights[i].direction.Normalize();
					aResult->spotLights[i].range = CU::Lerp(aFrom->spotLights[i].range, aTo->spotLights[i].range, aT);
					aResult->spotLights[i].angle = CU::Lerp(aFrom->spotLights[i].angle, aTo->spotLights[i].angle, aT);
					aResult->spotLights[i].size = aFrom->spotLights[i].size;
					aResult->spotlightsShadowCaster[i] = aFrom->spotlightsShadowCaster[i];
					aResult->spotlightIDs[i] = aFrom->spotlightIDs[i];
					aResult->spotLightsSize++;
				}
				else
				{
					bool lightFound = false;
					for (size_t missingLight = i; missingLight < aTo->spotLightsSize; missingLight++)
					{
						if (aFrom->spotlightIDs[i] == aTo->spotlightIDs[missingLight])
						{
							aResult->spotLights[i].color.LerpColor(aFrom->spotLights[i].color, aTo->spotLights[missingLight].color, aT);
							aResult->spotLights[i].position = v3f::ComponentWiseLerp(aFrom->spotLights[i].position, aTo->spotLights[missingLight].position, aT);
							aResult->spotLights[i].direction = v3f::ComponentWiseLerp(aFrom->spotLights[i].direction, aTo->spotLights[missingLight].direction, aT);
							aResult->spotLights[i].direction.Normalize();
							aResult->spotLights[i].range = CU::Lerp(aFrom->spotLights[i].range, aTo->spotLights[missingLight].range, aT);
							aResult->spotLights[i].angle = CU::Lerp(aFrom->spotLights[i].angle, aTo->spotLights[missingLight].angle, aT);
							aResult->spotLights[i].size = aFrom->spotLights[i].size;

							aResult->spotlightsShadowCaster[i] = aFrom->spotlightsShadowCaster[i];
							aResult->spotlightIDs[i] = aFrom->spotlightIDs[i];
							aResult->spotLightsSize++;
							lightFound = true;
							break;
						}
					}
					if (!lightFound)
					{
						aResult->spotLights[i].color = aFrom->spotLights[i].color;
						aResult->spotLights[i].position = aFrom->spotLights[i].position;
						aResult->spotLights[i].range = aFrom->spotLights[i].range;
						aResult->spotLights[i].direction = aFrom->spotLights[i].direction;
						aResult->spotLights[i].range = aFrom->spotLights[i].range;
						aResult->spotLights[i].angle = aFrom->spotLights[i].angle;
						aResult->spotLights[i].size = aFrom->spotLights[i].size;

						aResult->spotlightsShadowCaster[i] = aFrom->spotlightsShadowCaster[i];
						aResult->spotlightIDs[i] = aFrom->spotlightIDs[i];
						aResult->spotLightsSize++;
					}
				}
			}
			aJobCounter--;
			});
		//Sprites
		tpool->AddJob([&aFrom, &aTo, aT, &aResult, &aJobCounter]() {
			aResult->spritesSize = 0;
			for (size_t i = 0; i < aFrom->spritesSize; i++)
			{
				//if (aFrom->sprites[i].id == aTo->sprites[i].id)
				{
					aResult->sprites[i] = aFrom->sprites[i];

					aResult->sprites[i].vtx.myColor = aFrom->sprites[i].vtx.myColor;
					//aResult->sprites[i].vtx.myColor.LerpColor(aFrom->sprites[i].vtx.myColor, aTo->sprites[i].vtx.myColor, aT);
					aResult->sprites[i].vtx.myPivotOffset = aFrom->sprites[i].vtx.myPivotOffset;
					//aResult->sprites[i].vtx.myPivotOffset = v2f::ComponentWiseLerp(aFrom->sprites[i].vtx.myPivotOffset, aTo->sprites[i].vtx.myPivotOffset, aT);
					aResult->sprites[i].vtx.myPosition = aFrom->sprites[i].vtx.myPosition;
					//aResult->sprites[i].vtx.myPosition = v2f::ComponentWiseLerp(aFrom->sprites[i].vtx.myPosition, aTo->sprites[i].vtx.myPosition, aT);
					aResult->sprites[i].vtx.mySize = aFrom->sprites[i].vtx.mySize;
					//aResult->sprites[i].vtx.mySize = v2f::ComponentWiseLerp(aFrom->sprites[i].vtx.mySize, aTo->sprites[i].vtx.mySize, aT);
					aResult->sprites[i].vtx.myUVOffsetBotR = aFrom->sprites[i].vtx.myUVOffsetBotR;
					aResult->sprites[i].vtx.myUVOffsetTopL = aFrom->sprites[i].vtx.myUVOffsetTopL;
					//aResult->sprites[i].vtx.mySize = CU::Lerp(aFrom->sprites[i].vtx.mySize, aTo->sprites[i].vtx.mySize, aT);
					aResult->sprites[i].vtx.mySize = aFrom->sprites[i].vtx.mySize;
					aResult->spritesSize++;
				}
				/*else
				{
					bool spriteFound = false;
					for (size_t missingSprite = i; missingSprite < aTo->spotLightsSize; missingSprite++)
					{
						if (aFrom->sprites[i].id == aTo->sprites[missingSprite].id)
						{
							aResult->sprites[i] = aFrom->sprites[i];

							aResult->sprites[i].vtx.myColor.LerpColor(aFrom->sprites[i].vtx.myColor, aTo->sprites[missingSprite].vtx.myColor, aT);
							aResult->sprites[i].vtx.myPivotOffset = v2f::ComponentWiseLerp(aFrom->sprites[i].vtx.myPivotOffset, aTo->sprites[missingSprite].vtx.myPivotOffset, aT);
							aResult->sprites[i].vtx.myPosition = v2f::ComponentWiseLerp(aFrom->sprites[i].vtx.myPosition, aTo->sprites[missingSprite].vtx.myPosition, aT);
							aResult->sprites[i].vtx.mySize = v2f::ComponentWiseLerp(aFrom->sprites[i].vtx.mySize, aTo->sprites[missingSprite].vtx.mySize, aT);
							aResult->sprites[i].vtx.myUVOffsetBotR = aFrom->sprites[i].vtx.myUVOffsetBotR;
							aResult->sprites[i].vtx.myUVOffsetTopL = aFrom->sprites[i].vtx.myUVOffsetTopL;
							aResult->sprites[i].vtx.mySize = CU::Lerp(aFrom->sprites[i].vtx.mySize, aTo->sprites[missingSprite].vtx.mySize, aT);
							aResult->spritesSize++;
							spriteFound = true;
							break;
						}
					}
					if (!spriteFound)
					{
						aResult->sprites[i] = aFrom->sprites[i];

						aResult->sprites[i].vtx.myColor = aFrom->sprites[i].vtx.myColor;
						aResult->sprites[i].vtx.myPivotOffset = aFrom->sprites[i].vtx.myPivotOffset;
						aResult->sprites[i].vtx.myPosition = aFrom->sprites[i].vtx.myPosition;
						aResult->sprites[i].vtx.mySize = aFrom->sprites[i].vtx.mySize;
						aResult->sprites[i].vtx.myUVOffsetBotR = aFrom->sprites[i].vtx.myUVOffsetBotR;
						aResult->sprites[i].vtx.myUVOffsetTopL = aFrom->sprites[i].vtx.myUVOffsetTopL;
						aResult->sprites[i].vtx.mySize = aFrom->sprites[i].vtx.mySize;
						aResult->spritesSize++;
					}
				}*/
			}
			aJobCounter--;
			});
		//World Sprites
		tpool->AddJob([&aFrom, &aTo, aT, &aResult, &aJobCounter]() {
			aResult->wspritesSize = aFrom->wspritesSize;
			for (size_t i = 0; i < aResult->wspritesSize; i++)
			{
				aResult->wsprites[i] = aFrom->wsprites[i];
				aResult->wsprites[i].myData.myColor.LerpColor(aFrom->wsprites[i].myData.myColor, aTo->wsprites[i].myData.myColor, aT);
				aResult->wsprites[i].myData.myPivotOffset = v2f::ComponentWiseLerp(aFrom->wsprites[i].myData.myPivotOffset, aTo->wsprites[i].myData.myPivotOffset, aT);
				aResult->wsprites[i].myData.myPosition = v3f::ComponentWiseLerp(aFrom->wsprites[i].myData.myPosition, aTo->wsprites[i].myData.myPosition, aT);
				aResult->wsprites[i].myData.mySize = v2f::ComponentWiseLerp(aFrom->wsprites[i].myData.mySize, aTo->wsprites[i].myData.mySize, aT);
				aResult->wsprites[i].myData.myUVOffsetBotR = aFrom->wsprites[i].myData.myUVOffsetBotR;
				aResult->wsprites[i].myData.myUVOffsetTopL = aFrom->wsprites[i].myData.myUVOffsetTopL;
				aResult->wsprites[i].myData.mySize = CU::Lerp(aFrom->wsprites[i].myData.mySize, aTo->wsprites[i].myData.mySize, aT);
			}
			aJobCounter--;
			});
		//Sorted Meshes 
		tpool->AddJob([&aFrom, &aTo, aT, &aResult, &aJobCounter, &meshesToFill, &aModelManager]() {
			CU::AABB3Df collider;
			v4f min, max;
			aResult->sortedMeshesSize = aFrom->sortedMeshesSize;

			meshesToFill.normalMeshListCount = (unsigned short)aResult->sortedMeshesSize;
			meshesToFill.staticMeshCount = (unsigned short)aResult->sortedMeshesSize;
			meshesToFill.forwardMeshListCount = (unsigned short)aResult->sortedMeshesSize;
			meshesToFill.transparentCutoutCount = (unsigned short)aResult->sortedMeshesSize;
			meshesToFill.outlineMeshListCount = (unsigned short)aResult->sortedMeshesSize;

			for (size_t i = 0; i < aFrom->sortedMeshesSize; i++)
			{
				SortedModelDataForBuffers& dataTo = aResult->sortedMeshes[i];
				dataTo.model = aFrom->sortedMeshes[i].model;
				dataTo.modelType = aFrom->sortedMeshes[i].modelType;
				dataTo.submodelIndex = aFrom->sortedMeshes[i].submodelIndex;
				//dataTo.numberOfModels = aFrom->sortedMeshes[i].numberOfModels;
				dataTo.numberOfModels = 0;

				Model* modelPtr = aModelManager->GetModel(dataTo.model);
				if (modelPtr == nullptr){ 
					meshesToFill.myNormalMeshes[i].model = nullptr;
					meshesToFill.myStaticMeshes[i].model = nullptr;
					meshesToFill.myForwardMeshes[i].model = nullptr;
					meshesToFill.myTransparentCutoutMeshes[i].model = nullptr;
					meshesToFill.myOutlineMeshes[i].model = nullptr;
					continue; }
				meshesToFill.myNormalMeshes[i].model = &modelPtr->GetModelData(dataTo.submodelIndex);
				meshesToFill.myStaticMeshes[i].model = &modelPtr->GetModelData(dataTo.submodelIndex);
				meshesToFill.myForwardMeshes[i].model = &modelPtr->GetModelData(dataTo.submodelIndex);
				meshesToFill.myTransparentCutoutMeshes[i].model = &modelPtr->GetModelData(dataTo.submodelIndex);
				meshesToFill.myOutlineMeshes[i].model = &modelPtr->GetModelData(dataTo.submodelIndex);

				meshesToFill.myNormalMeshes[i].modelType = dataTo.modelType;
				meshesToFill.myStaticMeshes[i].modelType = dataTo.modelType;
				meshesToFill.myForwardMeshes[i].modelType = dataTo.modelType;
				meshesToFill.myTransparentCutoutMeshes[i].modelType = dataTo.modelType;
				meshesToFill.myOutlineMeshes[i].modelType = dataTo.modelType;

				if (dataTo.model == NIL_UUID) { continue; }
				ModelData* modelData = &aModelManager->GetModel(dataTo.model)->GetModelData(dataTo.submodelIndex);

				for (size_t model = 0; model < aFrom->sortedMeshes[i].numberOfModels; model++)
				{
					if (aTo->sortedMeshes[i].effectData[model].modelIndex == aFrom->sortedMeshes[i].effectData[model].modelIndex)
					{
						dataTo.transforms[model] = CU::Transform::LerpTransforms(aFrom->sortedMeshes[i].gameTransform[model], aTo->sortedMeshes[i].gameTransform[model], aT);
						collider = modelData->myBoundingVolume;
						min = { collider.myMin, 1 };
						min = min * dataTo.transforms[model];
						max = { collider.myMax, 1 };
						max = max * dataTo.transforms[model];
						collider.myMin = { min.x, min.y, min.z };
						collider.myMax = { max.x, max.y, max.z };
						CU::AABB3D<float>::SortMinMax(collider);
						dataTo.colliders[model] = (collider);

						dataTo.effectData[model].effectColor.LerpColor(aFrom->sortedMeshes[i].effectData[model].effectColor, aTo->sortedMeshes[i].effectData[model].effectColor, aT);
						dataTo.effectData[model].outlineColor.LerpColor(aFrom->sortedMeshes[i].effectData[model].outlineColor, aTo->sortedMeshes[i].effectData[model].outlineColor, aT);
						dataTo.effectData[model].tValue = CU::Lerp(aFrom->sortedMeshes[i].effectData[model].tValue, aTo->sortedMeshes[i].effectData[model].tValue, aT);
						dataTo.effectData[model].modelIndex = aFrom->sortedMeshes[i].effectData[model].modelIndex;
						dataTo.effectData[model].gBufferPSEffectIndex = aFrom->sortedMeshes[i].effectData[model].gBufferPSEffectIndex;
						dataTo.effectData[model].gBufferVSEffectIndex = aFrom->sortedMeshes[i].effectData[model].gBufferVSEffectIndex;
						dataTo.numberOfModels++;
						continue;
					}
					bool modelFound = false;
					for (size_t missingModel = model; missingModel < aTo->sortedMeshes[i].numberOfModels; missingModel++)
					{
						if (aFrom->sortedMeshes[i].effectData[model].modelIndex == aTo->sortedMeshes[i].effectData[missingModel].modelIndex)
						{
							dataTo.transforms[model] = CU::Transform::LerpTransforms(aFrom->sortedMeshes[i].gameTransform[model], aTo->sortedMeshes[i].gameTransform[missingModel], aT);
							collider = modelData->myBoundingVolume;
							min = { collider.myMin, 1 };
							min = min * dataTo.transforms[model];
							max = { collider.myMax, 1 };
							max = max * dataTo.transforms[model];
							collider.myMin = { min.x, min.y, min.z };
							collider.myMax = { max.x, max.y, max.z };
							CU::AABB3D<float>::SortMinMax(collider);
							dataTo.colliders[missingModel] = (collider);

							dataTo.effectData[model].effectColor.LerpColor(aFrom->sortedMeshes[i].effectData[model].effectColor, aTo->sortedMeshes[i].effectData[missingModel].effectColor, aT);
							dataTo.effectData[model].outlineColor.LerpColor(aFrom->sortedMeshes[i].effectData[model].outlineColor, aTo->sortedMeshes[i].effectData[missingModel].outlineColor, aT);
							dataTo.effectData[model].tValue = CU::Lerp(aFrom->sortedMeshes[i].effectData[model].tValue, aTo->sortedMeshes[i].effectData[missingModel].tValue, aT);
							dataTo.effectData[model].modelIndex = aFrom->sortedMeshes[i].effectData[model].modelIndex;
							dataTo.effectData[model].gBufferPSEffectIndex = aFrom->sortedMeshes[i].effectData[model].gBufferPSEffectIndex;
							dataTo.effectData[model].gBufferVSEffectIndex = aFrom->sortedMeshes[i].effectData[model].gBufferVSEffectIndex;
							dataTo.numberOfModels++;
							modelFound = true;
							break;
						}
					}
					if (!modelFound)
					{
						dataTo.transforms[model] = aFrom->sortedMeshes[i].gameTransform[model].GetMatrix();
						collider = modelData->myBoundingVolume;
						min = { collider.myMin, 1 };
						min = min * dataTo.transforms[model];
						max = { collider.myMax, 1 };
						max = max * dataTo.transforms[model];
						collider.myMin = { min.x, min.y, min.z };
						collider.myMax = { max.x, max.y, max.z };
						CU::AABB3D<float>::SortMinMax(collider);
						dataTo.colliders[model] = (collider);

						dataTo.effectData[model].effectColor.LerpColor(aFrom->sortedMeshes[i].effectData[model].effectColor, aFrom->sortedMeshes[i].effectData[model].effectColor, aT);
						dataTo.effectData[model].outlineColor.LerpColor(aFrom->sortedMeshes[i].effectData[model].outlineColor, aFrom->sortedMeshes[i].effectData[model].outlineColor, aT);
						dataTo.effectData[model].tValue = CU::Lerp(aFrom->sortedMeshes[i].effectData[model].tValue, aFrom->sortedMeshes[i].effectData[model].tValue, aT);
						dataTo.effectData[model].modelIndex = aFrom->sortedMeshes[i].effectData[model].modelIndex;
						dataTo.effectData[model].gBufferPSEffectIndex = aFrom->sortedMeshes[i].effectData[model].gBufferPSEffectIndex;
						dataTo.effectData[model].gBufferVSEffectIndex = aFrom->sortedMeshes[i].effectData[model].gBufferVSEffectIndex;
						dataTo.numberOfModels++;
					}
				}
			}
			aJobCounter--;
			});
		//Unique Meshes 
		tpool->AddJob([&aFrom, &aTo, aT, &aResult, &aJobCounter]() {
			CU::AABB3Df collider;
			v4f min, max;
			aResult->uniqueMeshesSize = 0;
			size_t uniqueSize = aFrom->uniqueMeshesSize;
			for (size_t i = 0; i < uniqueSize; i++)
			{
				MeshBuffererCommand& commandResult = aResult->uniqueMeshes[i];
				if (aFrom->uniqueMeshes[i].effectData.modelIndex == aTo->uniqueMeshes[i].effectData.modelIndex)
				{
					commandResult = aFrom->uniqueMeshes[i];
					commandResult.matrix = CU::Transform::LerpTransforms(aFrom->uniqueMeshes[i].transform, aTo->uniqueMeshes[i].transform, aT);
					collider = aResult->uniqueMeshes[i].myCollider;
					min = { collider.myMin, 1 };
					min = min * aResult->uniqueMeshes[i].matrix;
					max = { collider.myMax, 1 };
					max = max * aResult->uniqueMeshes[i].matrix;
					collider.myMin = { min.x, min.y, min.z };
					collider.myMax = { max.x, max.y, max.z };
					CU::AABB3D<float>::SortMinMax(collider);
					commandResult.myCollider = collider;

					commandResult.effectData.effectColor.LerpColor(aFrom->uniqueMeshes[i].effectData.effectColor, aTo->uniqueMeshes[i].effectData.effectColor, aT);
					commandResult.effectData.outlineColor.LerpColor(aFrom->uniqueMeshes[i].effectData.outlineColor, aTo->uniqueMeshes[i].effectData.outlineColor, aT);
					commandResult.effectData.tValue = CU::Lerp(aFrom->uniqueMeshes[i].effectData.tValue, aTo->uniqueMeshes[i].effectData.tValue, aT);
					commandResult.effectData.gBufferPSEffectIndex = aFrom->uniqueMeshes[i].effectData.gBufferPSEffectIndex;
					commandResult.effectData.gBufferVSEffectIndex = aFrom->uniqueMeshes[i].effectData.gBufferVSEffectIndex;
					commandResult.effectData.modelIndex = aFrom->uniqueMeshes[i].effectData.modelIndex;
					aResult->uniqueMeshesSize++;
				}
				else
				{
					bool modelFound = false;
					for (size_t missingModel = i; missingModel < aTo->uniqueMeshesSize; missingModel++)
					{
						if (aFrom->uniqueMeshes[i].effectData.modelIndex == aTo->uniqueMeshes[missingModel].effectData.modelIndex)
						{
							commandResult = aFrom->uniqueMeshes[i];
							commandResult.matrix = CU::Transform::LerpTransforms(aFrom->uniqueMeshes[i].transform, aTo->uniqueMeshes[missingModel].transform, aT);
							collider = commandResult.myCollider;
							min = { collider.myMin, 1 };
							min = min * commandResult.matrix;
							max = { collider.myMax, 1 };
							max = max * commandResult.matrix;
							collider.myMin = { min.x, min.y, min.z };
							collider.myMax = { max.x, max.y, max.z };
							CU::AABB3D<float>::SortMinMax(collider);
							commandResult.myCollider = (collider);

							commandResult.effectData.effectColor.LerpColor(aFrom->uniqueMeshes[i].effectData.effectColor, aTo->uniqueMeshes[missingModel].effectData.effectColor, aT);
							commandResult.effectData.outlineColor.LerpColor(aFrom->uniqueMeshes[i].effectData.outlineColor, aTo->uniqueMeshes[missingModel].effectData.outlineColor, aT);
							commandResult.effectData.tValue = CU::Lerp(aFrom->uniqueMeshes[i].effectData.tValue, aTo->uniqueMeshes[missingModel].effectData.tValue, aT);
							commandResult.effectData.modelIndex = aFrom->uniqueMeshes[i].effectData.modelIndex;
							commandResult.effectData.gBufferPSEffectIndex = aFrom->uniqueMeshes[i].effectData.gBufferPSEffectIndex;
							commandResult.effectData.gBufferVSEffectIndex = aFrom->uniqueMeshes[i].effectData.gBufferVSEffectIndex;
							aResult->uniqueMeshesSize++;
							modelFound = true;
							break;
						}
					}
					if (!modelFound)
					{
						commandResult = aFrom->uniqueMeshes[i];
						commandResult.matrix = aFrom->uniqueMeshes[i].transform.GetMatrix();
						collider = commandResult.myCollider;
						min = { collider.myMin, 1 };
						min = min * commandResult.matrix;
						max = { collider.myMax, 1 };
						max = max * commandResult.matrix;
						collider.myMin = { min.x, min.y, min.z };
						collider.myMax = { max.x, max.y, max.z };
						CU::AABB3D<float>::SortMinMax(collider);
						commandResult.myCollider = (collider);

						commandResult.effectData.effectColor = aFrom->uniqueMeshes[i].effectData.effectColor;
						commandResult.effectData.outlineColor = aFrom->uniqueMeshes[i].effectData.outlineColor;
						commandResult.effectData.tValue = aFrom->uniqueMeshes[i].effectData.tValue;
						commandResult.effectData.modelIndex = aFrom->uniqueMeshes[i].effectData.modelIndex;
						commandResult.effectData.gBufferPSEffectIndex = aFrom->uniqueMeshes[i].effectData.gBufferPSEffectIndex;
						commandResult.effectData.gBufferVSEffectIndex = aFrom->uniqueMeshes[i].effectData.gBufferVSEffectIndex;
						aResult->uniqueMeshesSize++;
					}
				}
			}
			aJobCounter--;
			});
		//sorted Anim Meshes 
		tpool->AddJob([&aFrom, &aTo, aT, &aResult, &aJobCounter, &meshesToFill]() {
			CU::AABB3Df collider;
			v4f min, max;
			aResult->sortedAnimMeshesSize = aFrom->sortedAnimMeshesSize;

			meshesToFill.animNormalCount = (unsigned short)aResult->sortedAnimMeshesSize;
			meshesToFill.animFwdListCount = (unsigned short)aResult->sortedAnimMeshesSize;
			meshesToFill.animTCutoutCount = (unsigned short)aResult->sortedAnimMeshesSize;
			meshesToFill.animOutlineCount = (unsigned short)aResult->sortedAnimMeshesSize;
			for (size_t i = 0; i < aResult->sortedAnimMeshesSize; i++)
			{
				SortedAnimationDataForBuffers& dataTo = aResult->sortedAnimMeshes[i];
				dataTo.model = aFrom->sortedAnimMeshes[i].model;
				dataTo.modelType = aFrom->sortedAnimMeshes[i].modelType;
				dataTo.numberOfModels = 0;
				//dataTo.numberOfModels = aFrom->sortedAnimMeshes[i].numberOfModels;
				dataTo.numberOfBones = aFrom->sortedAnimMeshes[i].numberOfBones;

				meshesToFill.myNormalAnimMeshes[i].model = dataTo.model;
				meshesToFill.myFWD_AnimMeshes[i].model = dataTo.model;
				meshesToFill.animTCutOutMeshes[i].model = dataTo.model;
				meshesToFill.myOutlinedAnimMeshes[i].model = dataTo.model;

				meshesToFill.myNormalAnimMeshes[i].modelType = dataTo.modelType;
				meshesToFill.myFWD_AnimMeshes[i].modelType = dataTo.modelType;
				meshesToFill.animTCutOutMeshes[i].modelType = dataTo.modelType;
				meshesToFill.myOutlinedAnimMeshes[i].modelType = dataTo.modelType;

				meshesToFill.myNormalAnimMeshes[i].numberOfBones = dataTo.numberOfBones;
				meshesToFill.myFWD_AnimMeshes[i].numberOfBones = dataTo.numberOfBones;
				meshesToFill.animTCutOutMeshes[i].numberOfBones = dataTo.numberOfBones;
				meshesToFill.myOutlinedAnimMeshes[i].numberOfBones = dataTo.numberOfBones;

				for (size_t model = 0; model < aFrom->sortedAnimMeshes[i].numberOfModels; model++)
				{
					if (aTo->sortedAnimMeshes[i].effectData[model].modelIndex == aFrom->sortedAnimMeshes[i].effectData[model].modelIndex)
					{
						dataTo.transforms[model] = CU::Transform::LerpTransforms(aFrom->sortedAnimMeshes[i].gameTransform[model], aTo->sortedAnimMeshes[i].gameTransform[model], aT);
						//Collider
						collider = dataTo.model->myBoundingVolume;
						min = { collider.myMin, 1 };
						min = min * dataTo.transforms[model];
						max = { collider.myMax, 1 };
						max = max * dataTo.transforms[model];
						collider.myMin = { min.x, min.y, min.z };
						collider.myMax = { max.x, max.y, max.z };
						CU::AABB3D<float>::SortMinMax(collider);
						dataTo.colliders[model] = (collider);
						dataTo.effectData[model].effectColor.LerpColor(aFrom->sortedAnimMeshes[i].effectData[model].effectColor, aTo->sortedAnimMeshes[i].effectData[model].effectColor, aT);
						dataTo.effectData[model].outlineColor.LerpColor(aFrom->sortedAnimMeshes[i].effectData[model].outlineColor, aTo->sortedAnimMeshes[i].effectData[model].outlineColor, aT);
						dataTo.effectData[model].tValue = CU::Lerp(aFrom->sortedAnimMeshes[i].effectData[model].tValue, aTo->sortedAnimMeshes[i].effectData[model].tValue, aT);
						dataTo.effectData[model].modelIndex = aFrom->sortedAnimMeshes[i].effectData[model].modelIndex;
						dataTo.effectData[model].gBufferPSEffectIndex = aFrom->sortedAnimMeshes[i].effectData[model].gBufferPSEffectIndex;
						dataTo.effectData[model].gBufferVSEffectIndex = aFrom->sortedAnimMeshes[i].effectData[model].gBufferVSEffectIndex;

						memcpy(&dataTo.boneTransforms[model * MAX_BONECOUNT], &aFrom->sortedAnimMeshes[i].boneTransforms[model * MAX_BONECOUNT], sizeof(m4f) * MAX_BONECOUNT);
						dataTo.numberOfModels++;
					}
					else
					{
						bool modelFound = false;
						for (size_t missingModel = model; missingModel < aTo->sortedAnimMeshes[i].numberOfModels; missingModel++)
						{
							if (aFrom->sortedAnimMeshes[i].effectData[model].modelIndex == aTo->sortedAnimMeshes[i].effectData[missingModel].modelIndex)
							{
								dataTo.transforms[model] = CU::Transform::LerpTransforms(aFrom->sortedAnimMeshes[i].gameTransform[model], aTo->sortedAnimMeshes[i].gameTransform[missingModel], aT);
								collider = dataTo.model->myBoundingVolume;
								min = { collider.myMin, 1 };
								min = min * dataTo.transforms[model];
								max = { collider.myMax, 1 };
								max = max * dataTo.transforms[model];
								collider.myMin = { min.x, min.y, min.z };
								collider.myMax = { max.x, max.y, max.z };
								CU::AABB3D<float>::SortMinMax(collider);
								dataTo.colliders[missingModel] = (collider);

								dataTo.effectData[model].effectColor.LerpColor(aFrom->sortedAnimMeshes[i].effectData[model].effectColor, aTo->sortedAnimMeshes[i].effectData[missingModel].effectColor, aT);
								dataTo.effectData[model].outlineColor.LerpColor(aFrom->sortedAnimMeshes[i].effectData[model].outlineColor, aTo->sortedAnimMeshes[i].effectData[missingModel].outlineColor, aT);
								dataTo.effectData[model].tValue = CU::Lerp(aFrom->sortedAnimMeshes[i].effectData[model].tValue, aTo->sortedAnimMeshes[i].effectData[missingModel].tValue, aT);
								dataTo.effectData[model].modelIndex = aFrom->sortedAnimMeshes[i].effectData[model].modelIndex;
								dataTo.effectData[model].gBufferPSEffectIndex = aFrom->sortedAnimMeshes[i].effectData[model].gBufferPSEffectIndex;
								dataTo.effectData[model].gBufferVSEffectIndex = aFrom->sortedAnimMeshes[i].effectData[model].gBufferVSEffectIndex;
								memcpy(&dataTo.boneTransforms[model * MAX_BONECOUNT], &aFrom->sortedAnimMeshes[i].boneTransforms[model * MAX_BONECOUNT], sizeof(m4f) * MAX_BONECOUNT);
								dataTo.numberOfModels++;
								modelFound = true;
								break;
							}
						}
						if (!modelFound)
						{
							dataTo.transforms[model] = aFrom->sortedAnimMeshes[i].gameTransform[model].GetMatrix();
							collider = dataTo.model->myBoundingVolume;
							min = { collider.myMin, 1 };
							min = min * dataTo.transforms[model];
							max = { collider.myMax, 1 };
							max = max * dataTo.transforms[model];
							collider.myMin = { min.x, min.y, min.z };
							collider.myMax = { max.x, max.y, max.z };
							CU::AABB3D<float>::SortMinMax(collider);
							dataTo.colliders[model] = (collider);

							dataTo.effectData[model].effectColor = aFrom->sortedAnimMeshes[i].effectData[model].effectColor;
							dataTo.effectData[model].outlineColor = aFrom->sortedAnimMeshes[i].effectData[model].outlineColor;
							dataTo.effectData[model].tValue = aFrom->sortedAnimMeshes[i].effectData[model].tValue;
							dataTo.effectData[model].modelIndex = aFrom->sortedAnimMeshes[i].effectData[model].modelIndex;
							dataTo.effectData[model].gBufferPSEffectIndex = aFrom->sortedAnimMeshes[i].effectData[model].gBufferPSEffectIndex;
							dataTo.effectData[model].gBufferVSEffectIndex = aFrom->sortedAnimMeshes[i].effectData[model].gBufferVSEffectIndex;
							dataTo.numberOfModels++;
						}
					}
				}
			}
			aJobCounter--;
			});
		//Unique Anim Meshes 
		tpool->AddJob([&aFrom, &aTo, aT, &aResult, &aJobCounter]() {
			CU::AABB3Df collider;
			v4f min, max;
			aResult->uniqueAnimatedMeshesSize = 0;
			for (size_t i = 0; i < aFrom->uniqueAnimatedMeshesSize; i++)
			{
				AnimatedMeshRenderCommand& commandResult = aResult->uniqueAnimatedMeshes[i];
				if (aFrom->uniqueAnimatedMeshes[i].effectData.modelIndex == aTo->uniqueAnimatedMeshes[i].effectData.modelIndex)
				{
					commandResult = aFrom->uniqueAnimatedMeshes[i];
					commandResult.matrix = CU::Transform::LerpTransforms(aFrom->uniqueAnimatedMeshes[i].transform, aTo->uniqueAnimatedMeshes[i].transform, aT);
					collider = commandResult.collider;
					min = { collider.myMin, 1 };
					min = min * commandResult.matrix;
					max = { collider.myMax, 1 };
					max = max * commandResult.matrix;
					collider.myMin = { min.x, min.y, min.z };
					collider.myMax = { max.x, max.y, max.z };
					CU::AABB3D<float>::SortMinMax(collider);
					commandResult.collider = collider;

					commandResult.effectData.effectColor.LerpColor(aFrom->uniqueAnimatedMeshes[i].effectData.effectColor, aTo->uniqueAnimatedMeshes[i].effectData.effectColor, aT);
					commandResult.effectData.outlineColor.LerpColor(aFrom->uniqueAnimatedMeshes[i].effectData.outlineColor, aTo->uniqueAnimatedMeshes[i].effectData.outlineColor, aT);
					commandResult.effectData.tValue = CU::Lerp(aFrom->uniqueAnimatedMeshes[i].effectData.tValue, aTo->uniqueAnimatedMeshes[i].effectData.tValue, aT);
					aResult->uniqueAnimatedMeshesSize++;
				}
				else
				{
					bool modelFound = false;
					for (size_t missingModel = i; missingModel < aTo->uniqueAnimatedMeshesSize; missingModel++)
					{
						if (aFrom->uniqueAnimatedMeshes[i].effectData.modelIndex == aTo->uniqueAnimatedMeshes[missingModel].effectData.modelIndex)
						{
							commandResult = aFrom->uniqueAnimatedMeshes[i];
							commandResult.matrix = CU::Transform::LerpTransforms(aFrom->uniqueAnimatedMeshes[i].transform, aTo->uniqueAnimatedMeshes[missingModel].transform, aT);
							collider = commandResult.collider;
							min = { collider.myMin, 1 };
							min = min * commandResult.matrix;
							max = { collider.myMax, 1 };
							max = max * commandResult.matrix;

							collider.myMin = { min.x, min.y, min.z };
							collider.myMax = { max.x, max.y, max.z };
							CU::AABB3D<float>::SortMinMax(collider);
							commandResult.collider = (collider);

							commandResult.effectData.effectColor.LerpColor(aFrom->uniqueAnimatedMeshes[i].effectData.effectColor, aTo->uniqueAnimatedMeshes[missingModel].effectData.effectColor, aT);
							commandResult.effectData.outlineColor.LerpColor(aFrom->uniqueAnimatedMeshes[i].effectData.outlineColor, aTo->uniqueAnimatedMeshes[missingModel].effectData.outlineColor, aT);
							commandResult.effectData.tValue = CU::Lerp(aFrom->uniqueAnimatedMeshes[i].effectData.tValue, aTo->uniqueAnimatedMeshes[missingModel].effectData.tValue, aT);
							aResult->uniqueAnimatedMeshesSize++;
							modelFound = true;
							break;
						}
					}
					if (!modelFound)
					{
						commandResult = aFrom->uniqueAnimatedMeshes[i];
						commandResult.matrix = aFrom->uniqueAnimatedMeshes[i].transform.GetMatrix();
						collider = commandResult.collider;
						min = { collider.myMin, 1 };
						min = min * commandResult.matrix;
						max = { collider.myMax, 1 };
						max = max * commandResult.matrix;
						collider.myMin = { min.x, min.y, min.z };
						collider.myMax = { max.x, max.y, max.z };
						CU::AABB3D<float>::SortMinMax(collider);
						commandResult.collider = (collider);

						aResult->uniqueAnimatedMeshesSize++;
					}
				}
			}
			aJobCounter--;
			});

		//Particles 
		aResult->particlesSize = aFrom->particlesSize;
		for (size_t i = 0; i < aResult->particlesSize; i++)
		{
			aResult->particles[i].myEmitterIndex = aFrom->particles[i].myEmitterIndex;
			aResult->particles[i].mySystemIndex = aFrom->particles[i].mySystemIndex;
			aResult->particles[i].shouldSpawn = aFrom->particles[i].shouldSpawn;
			aResult->particles[i].myMatrix = aFrom->particles[i].myTransform.GetMatrix();// CU::Transform::LerpTransforms(aFrom->particles[i].myTransform, aTo->particles[i].myTransform, aT);
		}
		//Decals 
		aResult->decalsSize = aFrom->decalsSize;
		for (size_t i = 0; i < aResult->decalsSize; i++)
		{
			aResult->decals[i] = aFrom->decals[i];
			aResult->decals[i].matrix = CU::Transform::LerpTransforms(aFrom->decals[i].gameTransform, aTo->decals[i].gameTransform, aT);
		}

		//Debug lines and Geometry
	#ifndef _DISTRIBUTION
		aResult->debugLinesSize = aFrom->debugLinesSize;
		for (size_t i = 0; i < aResult->debugLinesSize; i++)
		{
			aResult->debugLines[i].myColor.LerpColor(aFrom->debugLines[i].myColor, aTo->debugLines[i].myColor, aT);
			v4f::Lerp(aResult->debugLines[i].myPosFrom.GetArrayPointer(), aFrom->debugLines[i].myPosFrom.GetArrayPointer(), aTo->debugLines[i].myPosFrom.GetArrayPointer(), aT);
			v4f::Lerp(aResult->debugLines[i].myPosTo.GetArrayPointer(), aFrom->debugLines[i].myPosTo.GetArrayPointer(), aTo->debugLines[i].myPosTo.GetArrayPointer(), aT);
			aResult->debugLines[i].mySize = CU::Lerp(aFrom->debugLines[i].mySize, aTo->debugLines[i].mySize, aT);
		}

		aResult->debugSpheresSize = aFrom->debugSpheresSize;
		for (size_t i = 0; i < aResult->debugSpheresSize; i++)
		{
			aResult->debugSpheres[i].myPosition = v3f::ComponentWiseLerp(aFrom->debugSpheres[i].myPosition, aTo->debugSpheres[i].myPosition, aT);
			aResult->debugSpheres[i].mySphereColor.LerpColor(aResult->debugSpheres[i].mySphereColor, aResult->debugSpheres[i].mySphereColor, aT);
			aResult->debugSpheres[i].myRadius = CU::Lerp(aFrom->debugSpheres[i].myRadius, aTo->debugSpheres[i].myRadius, aT);
		}
	#endif
		while (aJobCounter > 0){}
	}


	void LerpCommonBufferData(RenderData* aFrom, RenderData* aTo, float aT, RenderData* aResult, GlobalFrameBufferData& someGlobalFrameBufferData)
	{
		aResult->ambience.LerpColor(aFrom->ambience, aTo->ambience, aT);
		aResult->ambienceFogColor.LerpColor(aFrom->ambienceFogColor, aTo->ambienceFogColor, aT);
		aResult->dirLight.myColor.LerpColor(aFrom->dirLight.myColor, aTo->dirLight.myColor, aT);
		aResult->fogExponent = CU::Lerp(aFrom->fogExponent, aTo->fogExponent, aT);
		aResult->fogFarDistance = CU::Lerp(aFrom->fogFarDistance, aTo->fogFarDistance, aT);
		aResult->fogNearDistance = CU::Lerp(aFrom->fogNearDistance, aTo->fogNearDistance, aT);
		v4f::Lerp(aResult->dirLight.myDirection.GetArrayPointer(), aFrom->dirLight.myDirection.GetArrayPointer(), aTo->dirLight.myDirection.GetArrayPointer(), aT);
		aResult->dirLight.myDirection.Normalize();
		aResult->DoF_FocusDistance = CU::Lerp(aFrom->DoF_FocusDistance, aTo->DoF_FocusDistance, aT);

		someGlobalFrameBufferData.directionalColor = aResult->dirLight.myColor.myVector;
		someGlobalFrameBufferData.directionalDirection = v3f(aResult->dirLight.myDirection.x, aResult->dirLight.myDirection.y, aResult->dirLight.myDirection.z);
		someGlobalFrameBufferData.ambientLightColor = aResult->ambience;
		someGlobalFrameBufferData.nearFogDistance = aResult->fogNearDistance;
		someGlobalFrameBufferData.farFogDistance = aResult->fogFarDistance;
		someGlobalFrameBufferData.fogExponent = aResult->fogExponent;
		someGlobalFrameBufferData.levelMiddle = aFrom->mapMiddle;
		someGlobalFrameBufferData.levelHalfSize = aFrom->mapHalfSize;
		someGlobalFrameBufferData.pointLightCount = aResult->pointLightsSize;
		someGlobalFrameBufferData.spotLightCount = aResult->spotLightsSize;
		someGlobalFrameBufferData.DoF_focusDistance = aResult->DoF_FocusDistance;
		someGlobalFrameBufferData.ambientFogColor = aResult->ambienceFogColor;
		aResult->camera = aFrom->camera;
		aResult->camera.GetMatrix() = CU::Transform::LerpTransforms(aFrom->camera.GetTransform(), aTo->camera.GetTransform(), aT);
		aResult->camera.Update();
		//LerpBufferCamera(aResult->camera, aFrom->camera, aTo->camera, aT);
		aResult->renderTargetcameraFlags = aFrom->renderTargetcameraFlags;
		for (size_t i = 0; i < NUMBOF_RENDERTARGETS; i++)
		{
			aResult->renderTargetCameras[i].camera = aFrom->renderTargetCameras[i].camera;
			if (aResult->renderTargetcameraFlags[PO2(i)])
			{
				aResult->renderTargetCameras[i].camera.GetTransform() = CU::Transform::LerpTransforms(aFrom->renderTargetCameras[i].camera.GetTransform(), aTo->renderTargetCameras[i].camera.GetTransform(), aT);
				aResult->renderTargetCameras[i].camera.GetMatrix() = aResult->renderTargetCameras[i].camera.GetTransform().GetMatrix();
				aResult->renderTargetCameras[i].renderFlag = aFrom->renderTargetCameras[i].renderFlag;
				aResult->renderTargetCameras[i].texture = aFrom->renderTargetCameras[i].texture;
				aResult->renderTargetCameras[i].depthTexture = aFrom->renderTargetCameras[i].depthTexture;
				aResult->renderTargetCameras[i].intermediateTexture = aFrom->renderTargetCameras[i].intermediateTexture;
				aResult->renderTargetCameras[i].gBufferTexture = aFrom->renderTargetCameras[i].gBufferTexture;
				aResult->renderTargetCameras[i].camera.Update();
			}
		}
		aResult->renderNormalView = aFrom->renderNormalView;
		aResult->mainRenderFlag = aFrom->mainRenderFlag;
	}
	void CalculateDirectionalLightCamera(RenderCameraData& aDLightCamera, const v3f aDirectionalDir, Camera& aViewCamera, Camera& aDLCamera)
	{
		aDLCamera.RecalculateProjectionMatrix(90, v2f({ 2048, 2048 }) * 6.0f, false, 100.0f, 10000.0f, false);
		v3f forwardVector = aViewCamera.GetMatrix().GetForwardVector() * 1723.0f;
		forwardVector.y = forwardVector.y + 1000.f;
		v3f worldPositionToLookAt = aViewCamera.GetMatrix().GetTranslationVector() + forwardVector;
		v3f lightPosition = worldPositionToLookAt + (aDirectionalDir * -1.0f) * 3000.0f;

		aDLightCamera.toCTransform.LookAt(worldPositionToLookAt, lightPosition);
		aDLightCamera.CProjection = aDLCamera.GetProjection();
		aDLightCamera.fromCamera = m4f::GetFastInverse(aDLightCamera.toCTransform);
		const CameraData data = aViewCamera.GetCameraData();
		aDLightCamera.myFar = data.myFar;
		aDLightCamera.myNear = data.myNear;
	}
	void InsertModelDataForModel(SortedModelDataForRendering& aBufferToFill, const unsigned short aIndexToFill, SortedModelDataForBuffers& aBufferToRead, const unsigned short aIndexToRead)
	{
		memcpy(&aBufferToFill.colliders[aIndexToFill], &aBufferToRead.colliders[aIndexToRead], sizeof(CU::AABB3Df));
		memcpy(&aBufferToFill.transforms[aIndexToFill], &aBufferToRead.transforms[aIndexToRead], sizeof(m4f));
		memcpy(&aBufferToFill.effectData[aIndexToFill], &aBufferToRead.effectData[aIndexToRead], sizeof(ObjectEffectData));
	}

	void InsertModelDataForAnimatedModel(SortedAnimationDataForBuffers& aBufferToFill, const unsigned short aIndexToFill, SortedAnimationDataForBuffers& aBufferToRead, const unsigned short aIndexToRead)
	{
		memcpy(&aBufferToFill.colliders[aIndexToFill], &aBufferToRead.colliders[aIndexToRead], sizeof(CU::AABB3Df));
		memcpy(&aBufferToFill.transforms[aIndexToFill], &aBufferToRead.transforms[aIndexToRead], sizeof(m4f));
		memcpy(&aBufferToFill.effectData[aIndexToFill], &aBufferToRead.effectData[aIndexToRead], sizeof(ObjectEffectData));
		memcpy(&aBufferToFill.boneTransforms[aIndexToFill * MAX_BONECOUNT], &aBufferToRead.boneTransforms[aIndexToRead * MAX_BONECOUNT], sizeof(m4f) * aBufferToFill.numberOfBones);

	}
	struct BufferPointer
	{
		RenderData* pointer = nullptr;
	};
	void SortSortedModels(BufferPointer aBufferPointer, MeshesToRender& meshesToFill, Camera& aRenderCamera, std::atomic<unsigned int>& aJobCounter, unsigned short aStartIndex, unsigned short aEndIndex)
	{
		CU::ThreadPool* tpool = EngineInterface::GetThreadPool();
		tpool->AddJob([aBufferPointer, &meshesToFill, &aRenderCamera, &aJobCounter, aStartIndex, aEndIndex]() {
			RenderData* aBuffer = aBufferPointer.pointer;
			const v3f cameraPos = aRenderCamera.GetMatrix().GetTranslationVector();
			for (unsigned short i = aStartIndex; i < aEndIndex; i++)
			{
				meshesToFill.myTransparentCutoutMeshes[i].numberOfModels = 0;
				meshesToFill.myForwardMeshes[i].numberOfModels = 0;
				meshesToFill.myNormalMeshes[i].numberOfModels = 0;
				meshesToFill.myStaticMeshes[i].numberOfModels = 0;
				meshesToFill.myOutlineMeshes[i].numberOfModels = 0;
				if (meshesToFill.myStaticMeshes[i].model == nullptr) { continue; }
				ModelData* modelData = meshesToFill.myStaticMeshes[i].model;
				for (unsigned short model = 0; model < aBuffer->sortedMeshes[i].numberOfModels; model++)
				{
					if (aRenderCamera.IsPointInsideFarRadius(aBuffer->sortedMeshes[i].colliders[model].ClosestPointInAABB(cameraPos)))
					{
						if (modelData->myMaterial->myMaterialType == MaterialTypes::EPBR_Transparent)
						{
							if (modelData->myMaterial->myIsCutOut)
							{
								InsertModelDataForModel(meshesToFill.myTransparentCutoutMeshes[i], meshesToFill.myTransparentCutoutMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
								meshesToFill.myTransparentCutoutMeshes[i].effectData[meshesToFill.myTransparentCutoutMeshes[i].numberOfModels - 1].gBufferPSEffectIndex = 2;
							}
							else
							{
								InsertModelDataForModel(meshesToFill.myForwardMeshes[i], meshesToFill.myForwardMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
							if ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_OUTLINE)
							{
								InsertModelDataForModel(meshesToFill.myOutlineMeshes[i], meshesToFill.myOutlineMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
							if ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_HOVERED)
							{
								InsertModelDataForModel(meshesToFill.myOutlineMeshes[i], meshesToFill.myOutlineMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
							if ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_STATIC)
							{
								InsertModelDataForModel(meshesToFill.myStaticMeshes[i], meshesToFill.myStaticMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
						}
						else
						{
							if ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_NORMAL && ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_STATIC) == false)
							{
								InsertModelDataForModel(meshesToFill.myNormalMeshes[i], meshesToFill.myOutlineMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
							if ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_OUTLINE)
							{
								InsertModelDataForModel(meshesToFill.myOutlineMeshes[i], meshesToFill.myOutlineMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
							if ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_HOVERED)
							{
								InsertModelDataForModel(meshesToFill.myOutlineMeshes[i], meshesToFill.myOutlineMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
							if ((EModelType_)aBuffer->sortedMeshes[i].modelType & EModelType_STATIC)
							{
								InsertModelDataForModel(meshesToFill.myStaticMeshes[i], meshesToFill.myStaticMeshes[i].numberOfModels++, aBuffer->sortedMeshes[i], model);
							}
						}

					}
				}
			}
			aJobCounter--;
			});
	}

	void SortModelsAndCalculateMVPs(RenderData* aBuffer, MeshesToRender& meshesToFill, Camera& aRenderCamera, std::atomic<unsigned int>& aJobCounter, ModelManager* aModelManager)
	{
		aJobCounter = 9;
		//PIXBeginEvent(PIX_COLOR_INDEX(14), "Start of SortAndCalc");

		CU::ThreadPool* tpool = EngineInterface::GetThreadPool();

		const unsigned short sortedMeshSize = (unsigned short)aBuffer->sortedMeshesSize;
		const unsigned short sortedAnimMeshSize = (unsigned short)aBuffer->sortedAnimMeshesSize;

		meshesToFill.normalMeshListCount = sortedMeshSize;
		meshesToFill.staticMeshCount = sortedMeshSize;
		meshesToFill.forwardMeshListCount = sortedMeshSize;
		meshesToFill.transparentCutoutCount = sortedMeshSize;
		meshesToFill.outlineMeshListCount = sortedMeshSize;

		meshesToFill.animNormalCount = sortedAnimMeshSize;
		meshesToFill.animFwdListCount = sortedAnimMeshSize;
		meshesToFill.animTCutoutCount = sortedAnimMeshSize;
		meshesToFill.animOutlineCount = sortedAnimMeshSize;
		BufferPointer pntr;
		pntr.pointer = aBuffer;

		const unsigned short aSortAmount = sortedMeshSize / 8;

		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, 0, aSortAmount);
		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, aSortAmount, aSortAmount * 2);
		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, aSortAmount * 2, aSortAmount * 3);
		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, aSortAmount * 3, aSortAmount * 4);
		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, aSortAmount * 4, aSortAmount * 5);
		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, aSortAmount * 5, aSortAmount * 6);
		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, aSortAmount * 6, aSortAmount * 7);
		SortSortedModels(pntr, meshesToFill, aRenderCamera, aJobCounter, aSortAmount * 7, sortedMeshSize);
		MeshRenderCommand commandToFill;

		for (unsigned short i = 0; i < aBuffer->uniqueMeshesSize; i++)
		{
			bool isTransparent = false;
			bool isCutout = false;
			bool isRenderTarget = false;
			if (aRenderCamera.IsPointInsideFarRadius(aBuffer->uniqueMeshes[i].matrix.GetTranslationVector()))
			{
				Model* model = aModelManager->GetModel(aBuffer->uniqueMeshes[i].model);
				if (!model) { continue; }
				commandToFill.model = model;
				commandToFill.matrix = aBuffer->uniqueMeshes[i].matrix;
				commandToFill.myCollider = aBuffer->uniqueMeshes[i].myCollider;
				commandToFill.modelType = aBuffer->uniqueMeshes[i].modelType;
				commandToFill.effectData = aBuffer->uniqueMeshes[i].effectData;
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
					meshesToFill.myRenderTargetMeshes.Add(commandToFill);
					if (model->GetAmountOfSubModels() == 1)
					{
						continue;
					}
				}
				if (isTransparent)
				{
					if (isCutout)
					{
						aBuffer->uniqueMeshes[i].effectData.gBufferPSEffectIndex = 2;
						meshesToFill.myUniqueTCutoutMeshes.Add(commandToFill);
					}
					else
					{
						meshesToFill.myUniqueForwardMeshes.Add(commandToFill);
					}
					if ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_OUTLINE)
					{
						meshesToFill.myUniqueOutlineMeshes.Add(commandToFill);
					}
					if ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_HOVERED)
					{
						meshesToFill.myUniqueOutlineMeshes.Add(commandToFill);
					}
					if ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_STATIC)
					{
						meshesToFill.myUniqueStaticMeshes.Add(commandToFill);
					}
				}
				else
				{
					if ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_NORMAL && ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_STATIC) == false)
					{
						meshesToFill.myUniqueNormalMeshes.Add(commandToFill);
					}
					if ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_OUTLINE)
					{
						meshesToFill.myUniqueOutlineMeshes.Add(commandToFill);
					}
					if ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_HOVERED)
					{
						meshesToFill.myUniqueOutlineMeshes.Add(commandToFill);
					}
					if ((EModelType_)aBuffer->uniqueMeshes[i].modelType & EModelType_STATIC)
					{
						meshesToFill.myUniqueStaticMeshes.Add(commandToFill);
					}
				}
			}
		}
		tpool->AddJob([&aBuffer, &meshesToFill, &aRenderCamera, &aJobCounter]() {
			//sorted Anim Meshes 
			v3f cameraPos = aRenderCamera.GetMatrix().GetTranslationVector();
			for (unsigned short i = 0; i < aBuffer->sortedAnimMeshesSize; i++)
			{
				meshesToFill.animTCutOutMeshes[i].numberOfModels = 0;
				meshesToFill.myFWD_AnimMeshes[i].numberOfModels = 0;
				meshesToFill.myNormalAnimMeshes[i].numberOfModels = 0;
				meshesToFill.myOutlinedAnimMeshes[i].numberOfModels = 0;
				for (unsigned short model = 0; model < aBuffer->sortedAnimMeshes[i].numberOfModels; model++)
				{
					if (aRenderCamera.IsPointInsideFarRadius(aBuffer->sortedAnimMeshes[i].colliders[model].ClosestPointInAABB(cameraPos)))
					{
						if (aBuffer->sortedAnimMeshes[i].model->myMaterial->myMaterialType == MaterialTypes::EPBRTransparent_Anim)
						{
							if (aBuffer->sortedAnimMeshes[i].model->myMaterial->myIsCutOut)
							{
								InsertModelDataForAnimatedModel(meshesToFill.animTCutOutMeshes[i], meshesToFill.animTCutOutMeshes[i].numberOfModels++, aBuffer->sortedAnimMeshes[i], model);
								meshesToFill.animTCutOutMeshes[i].effectData[meshesToFill.animTCutOutMeshes[i].numberOfModels - 1].gBufferPSEffectIndex = 2;
							}
							else
							{
								InsertModelDataForAnimatedModel(meshesToFill.myFWD_AnimMeshes[i], meshesToFill.myFWD_AnimMeshes[i].numberOfModels++, aBuffer->sortedAnimMeshes[i], model);
							}
						}
						else
						{
							InsertModelDataForAnimatedModel(meshesToFill.myNormalAnimMeshes[i], meshesToFill.myNormalAnimMeshes[i].numberOfModels++, aBuffer->sortedAnimMeshes[i], model);
						}
						if (aBuffer->sortedAnimMeshes[i].modelType != EAnimatedModelType_NORMAL)
						{
							InsertModelDataForAnimatedModel(meshesToFill.myOutlinedAnimMeshes[i], meshesToFill.myOutlinedAnimMeshes[i].numberOfModels++, aBuffer->sortedAnimMeshes[i], model);
						}
					}
				}
			}
			aJobCounter--;
		});
		//Unique Anim Meshes 
		for (unsigned short i = 0; i < aBuffer->uniqueAnimatedMeshesSize; i++)
		{
			bool isTransparent = false;
			bool isCutout = false;
			bool hasNormalMaterial = false;
			for (unsigned short subModel = 0; subModel < aBuffer->uniqueAnimatedMeshes[i].model->GetAmountOfSubModels(); subModel++)
			{
				Material* material = aBuffer->uniqueAnimatedMeshes[i].model->GetModelData(subModel).myMaterial;
				if (material->myMaterialType == MaterialTypes::EPBRTransparent_Anim)
				{
					if (material->myIsCutOut)
					{
						isCutout = true;
					}
					isTransparent = true;
				}
				else
				{
					hasNormalMaterial = true;
				}
			}
			if (isTransparent)
			{
				if (isCutout)
				{
					meshesToFill.myUniqueTCutout_AnimMeshes.Add(aBuffer->uniqueAnimatedMeshes[i]);
				}
				else
				{
					meshesToFill.myUniqueFWD_AnimMeshes.Add(aBuffer->uniqueAnimatedMeshes[i]);
				}
				if ((EAnimatedModelType_)aBuffer->uniqueAnimatedMeshes[i].modelType == EAnimatedModelType_OUTLINE_ENEMY ||
					(EAnimatedModelType_)aBuffer->uniqueAnimatedMeshes[i].modelType == EAnimatedModelType_HOVERED
					)
				{
					meshesToFill.myUniqueOutlinedAnimMeshes.Add(aBuffer->uniqueAnimatedMeshes[i]);

				}
			}
			if (hasNormalMaterial)
			{
				meshesToFill.myUniqueAnimatedNormalMeshes.Add(aBuffer->uniqueAnimatedMeshes[i]);
				if ((EAnimatedModelType_)aBuffer->uniqueAnimatedMeshes[i].modelType != EAnimatedModelType_NORMAL)
				{
					meshesToFill.myUniqueOutlinedAnimMeshes.Add(aBuffer->uniqueAnimatedMeshes[i]);
				}
			}
		}
		while (aJobCounter > 0)
		{
		}	
		//PIXEndEvent();
	}
}