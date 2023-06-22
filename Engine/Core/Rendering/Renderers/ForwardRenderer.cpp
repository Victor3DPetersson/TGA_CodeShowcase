#include "stdafx.h"
#include "ForwardRenderer.h"
#include "Core/DirectXFramework.h"
#include "CU/Math/Math.h"
#include <d3d11.h>
#include "GameObjects/Camera.h"
#include "GameObjects/Model.h"
#include "GameObjects/ModelInstance.h"
#include "GameObjects/ModelData.h"
#include "Managers\ModelManager.h"
#include "CU/Utility/Timer/Timer.h"
#include "imgui/imgui.h"
#include "Includes.h"
#include "GameObjects\Lights.h"
#include "ECS\Components.h"

#include "..\Renderer.h"
#include "..\Resources\FullScreenTexture.h"

#include "..\DX_Functions\DX_RenderFunctions.h"
#include "..\Resources\ConstantBufferManager.h"
#include "..\Resources\RenderFunctions.h"

Engine::ForwardRenderer::ForwardRenderer()
{
	myOutline_VS = nullptr;
	myOutlineAnim_VS = nullptr;
	myOutline_PS = nullptr;
	myOutline_PS_Hovered = nullptr;
	myDeviceContext = nullptr;
}

Engine::ForwardRenderer::~ForwardRenderer()
{
	SAFE_RELEASE(myOutline_VS);
	SAFE_RELEASE(myOutlineAnim_VS);
	SAFE_RELEASE(myOutline_PS);
	SAFE_RELEASE(myOutline_PS_Hovered);
	myDeviceContext = nullptr;
}

bool Engine::ForwardRenderer::Init(DirectXFramework* aFramework, ModelManager* aModelManager)
{
	if (!aFramework)
	{
		return false;
	}
	myDeviceContext = aFramework->GetDeviceContext();
	if (!myDeviceContext)
	{
		return false;
	}
	myOutline_VS = DX::LoadVS(aFramework->GetDevice(), "Content/Shaders/FWD_Outline_VS.cso");
	myOutlineAnim_VS = DX::LoadVS(aFramework->GetDevice(), "Content/Shaders/FWD_Outline_VS_A.cso");
	myOutline_PS = DX::LoadPS(aFramework->GetDevice(), "Content/Shaders/FWD_Outline_PS.cso");
	myOutline_PS_Hovered = DX::LoadPS(aFramework->GetDevice(), "Content/Shaders/FWD_Outline_PS_Hovered.cso");
	myModelmanager = aModelManager;
	return true;
}

void Engine::ForwardRenderer::RenderTranslucent(MeshesToRender& meshes,
	std::atomic<unsigned int>& aDrawcallCounter, ConstantBufferManager& aCBufferManager, bool isMainRender)
{
	const unsigned short size = meshes.forwardMeshListCount;
	for (unsigned short i = 0; i < size; i++)
	{
		DX::RenderInstancedModelBatch(meshes.myForwardMeshes[i], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, false);
	}
	const unsigned short commandsSize = meshes.myUniqueForwardMeshes.Size();
	for (unsigned short m = 0; m < commandsSize; m++)
	{
		DX::RenderModel(meshes.myUniqueForwardMeshes[m], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, isMainRender, false, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();

	const unsigned short animListSize = meshes.animFwdListCount;
	for (unsigned short i = 0; i < animListSize; i++)
	{
		DX::RenderInstancedAnimatedModelBatch(meshes.myFWD_AnimMeshes[i], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, false);
	}
	const unsigned short animCommandSize = meshes.myUniqueFWD_AnimMeshes.Size();
	for (unsigned short m = 0; m < animCommandSize; ++m)
	{
		DX::RenderAnimatedModel(meshes.myUniqueFWD_AnimMeshes[m], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, false, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();
}

void Engine::ForwardRenderer::RenderTransparentCutoutMeshes(MeshesToRender& meshes, std::atomic<unsigned int>& aDrawcallCounter, ConstantBufferManager& aCBufferManager, bool /*aRenderOnlyLights*/, bool /*aRenderLights*/)
{
	//Render Models
	const unsigned short size = meshes.transparentCutoutCount;
	for (unsigned short i = 0; i < size; i++)
	{
		DX::RenderInstancedModelBatch(meshes.myTransparentCutoutMeshes[i], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, false);
	}
	const unsigned short commandsSize = meshes.myUniqueTCutoutMeshes.Size();
	for (unsigned short m = 0; m < commandsSize; m++)
	{
		DX::RenderModel(meshes.myUniqueTCutoutMeshes[m], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, false, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();

	const unsigned short animListSize = meshes.animTCutoutCount;
	for (unsigned short i = 0; i < animListSize; i++)
	{
		DX::RenderInstancedAnimatedModelBatch(meshes.animTCutOutMeshes[i], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, false);
	}
	const unsigned short animCommandSize = meshes.myUniqueTCutout_AnimMeshes.Size();
	for (unsigned short m = 0; m < animCommandSize; ++m)
	{
		DX::RenderAnimatedModel(meshes.myUniqueTCutout_AnimMeshes[m], aCBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, false, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();
}

void Engine::ForwardRenderer::RenderOutlines(MeshesToRender& meshes,
	Renderer* aRenderer, FullScreenTexture* aStencilTexture, FullScreenTexture* aDeferredTarget, std::atomic<unsigned int>& aDrawcallCounter, ConstantBufferManager& aCBufferManager)
{
	/// //////////////////////// OUTLINE MODELS HERE //////////////////
	//------------Stencil Pass----------------//
	SortedModelDataForRendering* outlineMeshes = meshes.myOutlineMeshes;
	const unsigned short outlineMeshesSize = meshes.outlineMeshListCount;
	myDeviceContext->VSSetShader(nullptr, nullptr, 0);
	myDeviceContext->GSSetShader(nullptr, nullptr, 0);
	myDeviceContext->PSSetShader(nullptr, nullptr, 0);

	for (unsigned short i = 0; i < outlineMeshesSize; i++)
	{
		DX::RenderInstancedModelBatch(outlineMeshes[i], aCBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true);
	}
	const unsigned short uniqueOutlineMeshesSize = meshes.myUniqueOutlineMeshes.Size();
	for (unsigned short m = 0; m < uniqueOutlineMeshesSize; m++)
	{
		DX::RenderModel(meshes.myUniqueOutlineMeshes[m], aCBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();

	/// //////////////////////// ANIMATED MODELS HERE //////////////////
	const unsigned short animCommandSize = meshes.animOutlineCount;
	for (unsigned short i = 0; i < animCommandSize; i++)
	{
		DX::RenderInstancedAnimatedModelBatch(meshes.myOutlinedAnimMeshes[i], aCBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true);
	}

	const unsigned short uniqueAnimSize = meshes.myUniqueOutlinedAnimMeshes.Size();
	for (unsigned short m = 0; m < uniqueAnimSize; ++m)
	{
		DX::RenderAnimatedModel(meshes.myUniqueOutlinedAnimMeshes[m], aCBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();


	aDeferredTarget->SetAsActiveTarget(aStencilTexture);
	SetDepthStencilState(aRenderer->myContext, aRenderer->myRenderStates, DepthStencilState::DEPTHSTENCILSTATE_STENCILONLY_READ);
	bool isHovered = false;

	//------------Outline Pass----------------//
	myDeviceContext->VSSetShader(myOutline_VS, nullptr, 0);
	for (unsigned short i = 0; i < outlineMeshesSize; i++)
	{
		unsigned short amountOfInstances = outlineMeshes[i].numberOfModels;
		if (amountOfInstances > 0)
		{
			if (outlineMeshes[i].modelType & EModelType_OUTLINE)
			{
				myDeviceContext->PSSetShader(myOutline_PS, nullptr, 0);
			}
			else if(outlineMeshes[i].modelType & EModelType_HOVERED)
			{
				myDeviceContext->PSSetShader(myOutline_PS_Hovered, nullptr, 0);
			}
			DX::RenderInstancedModelBatch(outlineMeshes[i], aCBufferManager, DX::EModelRenderMode::ECustomShadersNoTextures, aDrawcallCounter, true, true);
		}
	}
	for (unsigned short m = 0; m < uniqueOutlineMeshesSize; m++)
	{
		if (meshes.myUniqueOutlineMeshes[m].modelType & EModelType_OUTLINE)
		{
			myDeviceContext->PSSetShader(myOutline_PS, nullptr, 0);
		}
		else if (meshes.myUniqueOutlineMeshes[m].modelType & EModelType_HOVERED)
		{
			myDeviceContext->PSSetShader(myOutline_PS_Hovered, nullptr, 0);
		}
		DX::RenderModel(meshes.myUniqueOutlineMeshes[m], aCBufferManager, DX::EModelRenderMode::ECustomShadersNoTextures, aDrawcallCounter, true, true, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();
	/// //////////////////////// ANIMATED MODELS HERE //////////////////
	myDeviceContext->VSSetShader(myOutlineAnim_VS, nullptr, 0);
	for (unsigned short i = 0; i < animCommandSize; i++)
	{
		unsigned short amountOfInstances = meshes.myOutlinedAnimMeshes[i].numberOfModels;
		if (amountOfInstances > 0)
		{
			if (meshes.myOutlinedAnimMeshes[i].modelType & EModelType_OUTLINE)
			{
				myDeviceContext->PSSetShader(myOutline_PS, nullptr, 0);
			}
			else if (meshes.myOutlinedAnimMeshes[i].modelType & EModelType_HOVERED)
			{
				myDeviceContext->PSSetShader(myOutline_PS_Hovered, nullptr, 0);
			}
			DX::RenderInstancedAnimatedModelBatch(meshes.myOutlinedAnimMeshes[i], aCBufferManager, DX::EModelRenderMode::ECustomShadersNoTextures, aDrawcallCounter, true, true);

		}
	}
	for (unsigned short m = 0; m < uniqueAnimSize; ++m)
	{
		isHovered = false;
		if (meshes.myUniqueOutlinedAnimMeshes[m].modelType & EAnimatedModelType_PLAYER || meshes.myUniqueOutlinedAnimMeshes[m].modelType & EAnimatedModelType_OUTLINE_ENEMY)
		{
			myDeviceContext->PSSetShader(myOutline_PS, nullptr, 0);
		}
		else if (meshes.myUniqueOutlinedAnimMeshes[m].modelType & EAnimatedModelType_HOVERED)
		{
			myDeviceContext->PSSetShader(myOutline_PS_Hovered, nullptr, 0);
		}
		DX::RenderAnimatedModel(meshes.myUniqueOutlinedAnimMeshes[m], aCBufferManager, DX::EModelRenderMode::ECustomShadersNoTextures, aDrawcallCounter, true, true, true);
	}
	aCBufferManager.MapUnMapEmptyEffectBuffer();
}
