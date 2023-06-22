#include "stdafx.h"
#include "DeferredRenderer.h"
#include <d3d11.h>

#include "GameObjects/Model.h"
#include "GameObjects/ModelInstance.h"
#include "GameObjects/ModelData.h"

#include "CU\Utility\Timer\Timer.h"
#include "CU\Collision\Intersection.hpp"
#include <fstream>
#include "ECS\Systems\RenderCommands.h"

#include "ShadowRenderer.h"
#include "..\DX_Functions\DX_RenderFunctions.h"

#include "..\Resources\GBuffer.h"
#include "..\Resources\FullScreenTexture.h"
#include "..\Resources\ConstantBufferManager.h"
#include "..\Resources\FullScreenTexture_Factory.h"


#include "Managers\ModelManager.h"


#include "imgui\imgui.h"
#include "EngineInterface.h"
#include "..\Resources\RenderFunctions.h"
#include "..\Resources\MeshStruct.h"
#include "..\Resources\RenderStates.h"
#include "FullScreenRenderer.h"


bool Engine::DeferredRenderer::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ModelManager* aModelManager)
{
	myModelManager = aModelManager;
	GUID key = aModelManager->LoadPrimitive(PrimitiveType::PrimitiveType_Cube);
	myDecalBox = aModelManager->GetModel(key);
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;

	std::ifstream vsFile;
	vsFile.open("Content/Shaders/FullscreenVS.cso", std::ios::binary);
	std::string vsData = { std::istreambuf_iterator<char>(vsFile), std::istreambuf_iterator<char>() };
	result = myDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &myFullScreenShader);
	if (FAILED(result))
	{
		return false;
	}
	vsFile.close();

	//Read in environmentlights shader
	myDecalVS = DX::LoadVS(myDevice, "Content/Shaders/GBuffer_VS_Decals.cso");

	myBakedLightShader = DX::LoadPS(myDevice, "Content/Shaders/PS_BakedLight.cso");
	myDeferredLightShader = DX::LoadPS(myDevice, "Content/Shaders/PS_LightCalculation_PBR.cso");
	myIrradiantLightShader = DX::LoadPS(myDevice, "Content/Shaders/PS_IrradiantLightOnly.cso");
	myBakeCubeLightShader = DX::LoadPS(myDevice, "Content/Shaders/PS_LightbakeWithAmbientSky.cso");

	myStaticVS = DX::LoadVS(myDevice, "Content/Shaders/Deferred_GBuffer_VS.cso");
	myStaticPS = DX::LoadPS(myDevice, "Content/Shaders/Deferred_GBuffer_PS.cso");
	myIntermediateVertexNormals = CreateFullScreenTexture(EngineInterface::GetRenderResolution(), DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true, false);
	return true;
}

void Engine::DeferredRenderer::GenerateGBuffer(MeshesToRender& meshes, GBuffer* aGBuffer, FullScreenTexture* aDepthTexture,
	RenderData& someDecals,  ConstantBufferManager& aConstantBufferManager, Camera& aRenderCamera, RenderStates* someRenderStates, FullScreenRenderer* aFullscreenRenderer, std::atomic<unsigned int>& aDrawcallCounter)
{
	GUID key = myModelManager->LoadPrimitive(PrimitiveType::PrimitiveType_Cube);
	myDecalBox = myModelManager->GetModel(key);
	SortedModelDataForRendering* staticMeshes = meshes.myStaticMeshes;
	//Here I set the static VS and PS

	myContext->PSSetShader(myStaticPS, nullptr, 0);
	myContext->VSSetShader(myStaticVS, nullptr, 0);

	aConstantBufferManager.MapUnMapEmptyEffectBuffer();
	unsigned short size = (unsigned short)meshes.staticMeshCount;
	for (unsigned short i = 0; i < size; i++)
	{
		DX::RenderInstancedModelBatch(meshes.myStaticMeshes[i], aConstantBufferManager, DX::EModelRenderMode::EOnlyTextures, aDrawcallCounter, myModelManager, true);
	}
	unsigned short uniqueStaticSize = meshes.myUniqueStaticMeshes.Size();
	for (unsigned short i = 0; i < uniqueStaticSize; i++)
	{
		DX::RenderModel(meshes.myUniqueStaticMeshes[i], aConstantBufferManager, DX::EModelRenderMode::EOnlyTextures, aDrawcallCounter, myModelManager, true, true);
	}
	aGBuffer->SetAsActiveTarget(EGBufferTexture::DEPTH, aDepthTexture);

	unsigned short uniqueNormalSize = meshes.myUniqueNormalMeshes.Size();
	for (unsigned short i = 0; i < uniqueNormalSize; i++)
	{
		DX::RenderModel(meshes.myUniqueNormalMeshes[i], aConstantBufferManager, DX::EModelRenderMode::EOnlyTextures, aDrawcallCounter, myModelManager, true, true);
	}
	aConstantBufferManager.MapUnMapEmptyEffectBuffer();
	SortedModelDataForRendering* normalMeshes = meshes.myNormalMeshes;
	size = meshes.normalMeshListCount;
	for (unsigned short i = 0; i < size; i++)
	{
		DX::RenderInstancedModelBatch(normalMeshes[i], aConstantBufferManager, DX::EModelRenderMode::EOnlyTextures, aDrawcallCounter, myModelManager, true);
	}
	//ID3D11BlendState* aAlphaBlendState, ID3D11BlendState* aNormalBlendState,
	//ID3D11DepthStencilState* aReadOnlyDepthState, ID3D11DepthStencilState* aNormalDepthState,
	//Add CutOutmeshes here

	//Here we draw Decals
	if (someDecals.decalsSize > 0)
	{
		UINT sampleMask = 0xffffffff;
		float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		myContext->OMSetBlendState(someRenderStates->blendStates[BlendState::BLENDSTATE_ALPHABLEND_DECALS], blendFactor, sampleMask);
		myContext->OMSetDepthStencilState(someRenderStates->depthStencilStates[DEPTHSTENCILSTATE_READONLY], 0xFF);
		myIntermediateVertexNormals->SetAsActiveTarget();
		aGBuffer->SetAsResourceOnSlot(EGBufferTexture::VERTEXNORMAL_ROUGHNESS, 8);
		aFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

		aGBuffer->SetDecalTargetsAsTarget(aDepthTexture);
		aGBuffer->SetAsResourceOnSlot(EGBufferTexture::POSITION_NMSTRENGTH, 21);
		myIntermediateVertexNormals->SetAsResourceOnSlot(24);
		aGBuffer->SetAsResourceOnSlot(EGBufferTexture::DEPTH, 26);

		myContext->VSSetShader(myDecalVS, nullptr, 0);
		DX::RenderDecals(myDecalBox, someDecals, aConstantBufferManager, aDrawcallCounter, aRenderCamera, someRenderStates);

		DX::ClearShaderResources(myContext, 21);
		DX::ClearShaderResources(myContext, 24);
		DX::ClearShaderResources(myContext, 26);
		myContext->OMSetDepthStencilState(someRenderStates->depthStencilStates[DEPTHSTENCILSTATE_DEFAULT], 0xFF);
		aGBuffer->SetAsActiveTarget(EGBufferTexture::DEPTH, aDepthTexture);
		myContext->OMSetBlendState(someRenderStates->blendStates[BlendState::BLENDSTATE_DISABLE], blendFactor, sampleMask);
	}

	myContext->PSSetShader(myStaticPS, nullptr, 0);
	myContext->VSSetShader(myStaticVS, nullptr, 0);
	for (size_t mParticles = 0; mParticles < meshes.particleMeshCount; mParticles++)
	{
		DX::RenderParticleMeshBatch(myContext, &meshes.particlesMesh[mParticles], aConstantBufferManager, DX::EModelRenderMode::EOnlyTextures, aDrawcallCounter);
	}


	SortedAnimationDataForBuffers* normalAnimMehses = meshes.myNormalAnimMeshes;
	unsigned int animSize = (unsigned int)meshes.animNormalCount;
	for (unsigned short i = 0; i < animSize; i++)
	{
		DX::RenderInstancedAnimatedModelBatch(normalAnimMehses[i], aConstantBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, true);
	}
	unsigned short uniqueAnimSize = meshes.myUniqueAnimatedNormalMeshes.Size();
	for (unsigned short i = 0; i < uniqueAnimSize; i++)
	{
		DX::RenderAnimatedModel(meshes.myUniqueAnimatedNormalMeshes[i], aConstantBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, true, true);
	}
	aConstantBufferManager.MapUnMapEmptyEffectBuffer();
	for (unsigned int i = 8; i < 20; i++)
	{
		DX::ClearShaderResources(myContext, i);
	}
	for (unsigned int i = 0; i < 6; i++)
	{
		DX::ClearShaderResources(myContext, i + 21);
	}
}

void Engine::DeferredRenderer::RenderLights(RenderData* someSceneData, std::atomic<unsigned int>& aDrawcallCounter, bool aRenderWithharmonics)
{
	myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myContext->IASetInputLayout(nullptr);
	myContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	myContext->VSSetShader(myFullScreenShader, nullptr, 0);
	if (aRenderWithharmonics)
	{
		myContext->PSSetShader(myDeferredLightShader, nullptr, 0);
	}
	else
	{
		myContext->PSSetShader(myBakedLightShader, nullptr, 0);
	}
	aDrawcallCounter++;
	myContext->Draw(3, 0);
}

void Engine::DeferredRenderer::RenderBakedLights(RenderData* someSceneData, std::atomic<unsigned int>& aDrawcallCounter)
{
	myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myContext->IASetInputLayout(nullptr);
	myContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	myContext->VSSetShader(myFullScreenShader, nullptr, 0);
	myContext->PSSetShader(myBakeCubeLightShader, nullptr, 0);

	aDrawcallCounter++;
	myContext->Draw(3, 0);
}


void Engine::DeferredRenderer::RenderIrradiantLight(RenderData* someSceneData, std::atomic<unsigned int>& aDrawcallCounter)
{
	myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myContext->IASetInputLayout(nullptr);
	myContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	myContext->VSSetShader(myFullScreenShader, nullptr, 0);
	myContext->PSSetShader(myIrradiantLightShader, nullptr, 0);

	aDrawcallCounter++;
	myContext->Draw(3, 0);
}

void Engine::DeferredRenderer::RenderStaticMap(Camera& aCameraToRenderThrough, CU::GrowingArray<SortedModelDataForBuffers> someStaticMeshes)
{
	m4f VP = m4f::GetFastInverse(aCameraToRenderThrough.GetMatrix()) * aCameraToRenderThrough.GetProjection();
	//myFrameBufferData.myEnvironmentLightMipCount = 0;
	//myFrameBufferData.myRenderCamera = aCameraToRenderThrough.GetCameraData();
	//myFrameBufferData.myCameraPosition = aCameraToRenderThrough.GetMatrix().GetTranslationVector();
	//DX::MapUnmapConstantBuffer(myContext, myFrameBuffer, &myFrameBufferData, sizeof(FrameBufferData), 2, UINT_MAX, 2);

	//CU::GrowingArray<SortedModelDataForBuffers>& staticMeshes = someStaticMeshes;
	//unsigned int size = staticMeshes.Size();
	//myContext->PSSetShader(myRenderStaticMapShader, nullptr, 0);

	//for (unsigned short i = 0; i < size; i++)
	//{
	//	ModelData& modelData = *staticMeshes[i].model;
	//	
	//	if (staticMeshes[i].numberOfModels > 0 && modelData.myMaterial->myMaterialType != MaterialTypes::EPBR_Transparent)
	//	{
	//		for (unsigned short t = 0; t < staticMeshes[i].numberOfModels; t++)
	//		{
	//			staticMeshes[i].MVP_transforms[t] = staticMeshes[i].transforms[t] * VP;
	//		}

	//		DX::MapUnmapDynamicBuffer(myContext, myTransformStructuredBufferSRV, myTransformStructuredBuffer, &staticMeshes[i].transforms[0], sizeof(m4f), staticMeshes[i].numberOfModels, 0, true, false, false);
	//		DX::MapUnmapDynamicBuffer(myContext, myMVPStructuredBufferSRV, myMVPStructuredBuffer, &staticMeshes[i].MVP_transforms[0], sizeof(m4f), staticMeshes[i].numberOfModels, 1, true, false, false);
	//		myContext->IASetPrimitiveTopology(modelData.myMaterial->myPrimitiveTopology);
	//		myContext->IASetInputLayout(modelData.myMaterial->myInputLayout);
	//		myContext->IASetVertexBuffers(0, 1, &modelData.myVertexBuffer, &modelData.myStride, &modelData.myOffset);
	//		myContext->IASetIndexBuffer(modelData.myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	//		myContext->VSSetShader(modelData.myMaterial->myVertexShader, nullptr, 0);
	//		myContext->PSSetShaderResources(1, 3, modelData.myMaterial->myTextures);

	//		myContext->DrawIndexedInstanced(modelData.myNumberOfIndices, (unsigned int)staticMeshes[i].numberOfModels, 0, 0, 0);
	//	}
	//}
}
