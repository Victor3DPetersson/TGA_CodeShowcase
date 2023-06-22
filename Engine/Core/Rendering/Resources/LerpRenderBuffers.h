#pragma once
#include "RenderData.h"
#include "..\CommonUtilities\CU\Containers\GrowingArray.hpp"
#include <CU\Math\Matrix.hpp>

class Camera;
struct RenderCameraData;
struct MeshesToRender;
struct SortedModelDataForBuffers;
struct SortedAnimationDataForBuffers;
namespace Engine
{
	struct RenderData;
	struct GlobalFrameBufferData;
	class ModelManager;

	//8 JOBS
	void LerpBuffers(RenderData* aFrom, RenderData* aTo, float aT, RenderData* aResult, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aJobCounter, ModelManager* aModelManager);

	void LerpCommonBufferData(RenderData* aFrom, RenderData* aTo, float aT, RenderData* aResult, GlobalFrameBufferData& someGlobalFrameBufferData);

	void CalculateDirectionalLightCamera(RenderCameraData& aDLightCamera, const v3f aDirectionalDir, Camera& aViewCamera, Camera& aDLCamera);

	void InsertModelDataForModel(SortedModelDataForRendering& aBufferToFill, const unsigned short aIndexToFill, SortedModelDataForBuffers& aBufferToRead, const unsigned short aIndexToRead);

	void InsertModelDataForAnimatedModel(SortedAnimationDataForBuffers& aBufferToFill, const unsigned short aIndexToFill, SortedAnimationDataForBuffers& aBufferToRead, const unsigned short aIndexToRead);

	void SortModelsAndCalculateMVPs(RenderData* aBuffer, MeshesToRender& meshesToFill, Camera& aRenderCamera, std::atomic<unsigned int>& aJobCounter, ModelManager* aModelManager);

}