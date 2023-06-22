#include "stdafx.h"
#include "CubemapRenderer.h"

#include <d3d11.h>

#include "../Resources/ConstantBufferManager.h"
#include "../Resources/FullScreenTexture.h"
#include "../Resources/GBuffer.h"
#include "../Resources/FullScreenTexture_Factory.h"
#include "..\Resources\MeshStruct.h"
#include "../Resources/LerpRenderBuffers.h"
#include "RenderData.h"
#include "../Resources/RenderFunctions.h"
#include "../DX_Functions/DX_RenderFunctions.h"
#include "../Resources/RenderStates.h"

#include "../Resources/CameraClusterCreation.h"

#include "DeferredRenderer.h"
#include "FullScreenRenderer.h"
#include "ShadowRenderer.h"

#include "EngineInterface.h"
#include "Managers\ModelManager.h"
#include "Managers\TextureManager.h"

#include "../DX_Functions/SHMath/DirectXSH.h"
#include "imgui\imgui.h"

bool Engine::CubemapRenderer::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ConstantBufferManager* aConstantBufferManager, FullScreenRenderer* aFullscreenRenderer, DeferredRenderer* aDeferredRenderer, ShadowRenderer* aShadowRenderer, unsigned int& aNumberOfMips)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myCBufferManager = aConstantBufferManager;
	myFullscreenRenderer = aFullscreenRenderer;
	myDeferredRenderer = aDeferredRenderer;
	myShadowRenderer = aShadowRenderer;

	unsigned int mipCounter = 1;
	unsigned int dividedResolution = CUBEMAP_TEXTURESIZE;
	while (dividedResolution != 1)
	{
		dividedResolution = dividedResolution / 2;
		mipCounter++;
	}
	aNumberOfMips = mipCounter;
	myModelManager = EngineInterface::GetModelManager();
	return true;
}
bool Engine::CubemapRenderer::InitGPUAndRenderData()
{
	if (myRenderData) { return true; }
	myRenderData = new CubeRenderData();
	myMemoryNewCounter++;
	myRenderData->cubeMapTextureSize = { CUBEMAP_TEXTURESIZE,CUBEMAP_TEXTURESIZE };
	bool failedInit = false;
	for (unsigned int c = 0; c < 6; c++)
	{
		switch (c)
		{
		case 0:
			//RIGHT
			myRenderData->cubeRotationMatrixes[c].SetRotation({ CU::AngleToRadian(0), CU::AngleToRadian(90), CU::AngleToRadian(0) });
			break;
		case 1:
			//LEFT
			myRenderData->cubeRotationMatrixes[c].SetRotation({ CU::AngleToRadian(0), CU::AngleToRadian(-90), CU::AngleToRadian(0) });
			break;
		case 2:
			//UP
			myRenderData->cubeRotationMatrixes[c].SetRotation({ CU::AngleToRadian(-90), CU::AngleToRadian(0), CU::AngleToRadian(0) });
			break;
		case 3:
			//DOWN
			myRenderData->cubeRotationMatrixes[c].SetRotation({ CU::AngleToRadian(90), CU::AngleToRadian(0), CU::AngleToRadian(0) });
			break;
		case 4:
			//FRONT is an Identity matrix
			break;
		case 5:
			//BACK
			myRenderData->cubeRotationMatrixes[c].SetRotation({ CU::AngleToRadian(0), CU::AngleToRadian(180), CU::AngleToRadian(0) });
			break;
		default:
			break;
		}
	}
	if (!myRenderData->cubeMap.Init(myDevice, myContext, { 128, 128 }, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false))
	{
		assert(false && "Failed to create cubemap for cubemap generation");
		failedInit = true;
	}
	for (unsigned int i = 0; i < 6; i++)
	{
		myRenderData->cubeRenderTargets[i] = CreateFullScreenTexture({ 128, 128 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false);
	}
	unsigned int resolution = 256;
	for (unsigned short mips = 0; mips < 8; mips++)
	{
		resolution = (unsigned int)((float)resolution / 2.f);
		myRenderData->cubeDownSampledTargets_Lods[mips] = CreateFullScreenTextureArray({ resolution, resolution }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, 1, 6, false, false, false, true);
	}

	myRenderData->GBufferTarget = CreateGBuffer(myRenderData->cubeMapTextureSize, myDevice, myContext);
	myRenderData->cubeDepth = CreateDepthTexture(myRenderData->cubeMapTextureSize, DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE, EDepthStencilFlag::DEPTH_ONLY);
	myRenderData->SSAOtarget = CreateFullScreenTexture(myRenderData->cubeMapTextureSize, DXGI_FORMAT_R16_FLOAT, myDevice, myContext, true);
	myRenderData->SSAOBlurredtarget = CreateFullScreenTexture(myRenderData->cubeMapTextureSize, DXGI_FORMAT_R16_FLOAT, myDevice, myContext, true);
	myRenderData->renderCam.RecalculateProjectionMatrix(90, myRenderData->cubeMapTextureSize, true, 50.f, 2000.f);

	HRESULT result;
	D3D11_BUFFER_DESC constantBufferDescription = { 0 };
	constantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	constantBufferDescription.ByteWidth = sizeof(v4f);
	result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myRenderData->cubemapRenderSizeBuffer);
	if (FAILED(result)) { failedInit = true; }

	if (!myRenderData->filteredCubeMap.InitWithFullMip(myDevice, myContext, myRenderData->cubeMapTextureSize, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false))
	{
		assert(false && "Failed to create cubemap with Mips");
		failedInit = true;
	}

	myRenderData->preBlurShader = DX::LoadCS(myDevice, "Content/Shaders/CS_FilterEnvMap.cso");
	if (myRenderData->preBlurShader == nullptr)
	{
		failedInit = true;
	}

	if (!DX::CreateStructuredBuffer(myDevice, &myRenderData->levelReflectionProbeBufferSRV, &myRenderData->levelReflectionProbeBuffer, MAX_NUMBOF_REFLECTIONPROBES, sizeof(ReflectionProbe))) { failedInit = true; }

	D3D11_TEXTURE2D_DESC texArrayDesc;
	myRenderData->filteredCubeMap.GetTexture()->GetDesc(&texArrayDesc);
	if (failedInit)
	{
		DeInitGPUData();
	}
	return true;
}

void Engine::CubemapRenderer::EditorInit()
{
	if (myEditorData) { return; }
	myEditorData = new CubeRendererEditorData();

	myEditorData->spherePS = DX::LoadPS(myDevice, "Content/Shaders/PS_Debug_CubeMapViewer.cso");
	if (myEditorData->spherePS == nullptr) { assert(false && "Failed to load cube PS"); }

	myEditorData->gridPS = DX::LoadPS(myDevice, "Content/Shaders/PS_Debug_SHGrid.cso");
	if (myEditorData->gridPS == nullptr){ assert(false && "Failed to create Grid PS"); }

	myEditorData->debugSphere.model = myModelManager->GetModel(myModelManager->LoadPrimitive(PrimitiveType::PrimitiveType_Sphere));
	myEditorData->debugSphere.matrix.SetScale({ 0.4f, 0.4f, 0.4f });

	myEditorData->meshBuffer = new SortedModelDataForRendering();
	myEditorData->meshBuffer->model = &myEditorData->debugSphere.model->GetModelData();


	HRESULT result;
	D3D11_BUFFER_DESC constantBufferDescription = { 0 };
	constantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDescription.ByteWidth = sizeof(SHGridData);
	result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myEditorData->SHGridBuffer);
	if (FAILED(result)) { assert(false && "Failed to create constant buffer in Cubemap Editor Init"); return; }

	InitGPUAndRenderData();
}

void Engine::CubemapRenderer::DeInitGPUData()
{
	if (myEditorData)
	{
		return;
	}
	if (myRenderData == nullptr) { return; }
	for (size_t mips = 0; mips < 8; mips++)
	{
		myRenderData->cubeDownSampledTargets_Lods[mips]->ReleaseResources();
		SAFE_DELETE(myRenderData->cubeDownSampledTargets_Lods[mips]);
	}
	for (size_t target = 0; target < 6; target++)
	{
		myRenderData->cubeRenderTargets[target]->ReleaseResources();
		SAFE_DELETE(myRenderData->cubeRenderTargets[target]);
	}
	myRenderData->cubeMap.ReleaseResources();
	myRenderData->cubeDepth->ReleaseResources();
	SAFE_DELETE(myRenderData->cubeDepth);
	myRenderData->SSAOtarget->ReleaseResources();
	SAFE_DELETE(myRenderData->SSAOtarget);
	myRenderData->SSAOBlurredtarget->ReleaseResources();
	SAFE_DELETE(myRenderData->SSAOBlurredtarget);
	myRenderData->GBufferTarget->ReleaseResources();
	SAFE_DELETE(myRenderData->GBufferTarget);

	myRenderData->preBlurShader->Release();
	myRenderData->filteredCubeMap.ReleaseResources();
	myRenderData->levelReflectionProbes.ReleaseResources();
	SAFE_RELEASE(myRenderData->cubemapRenderSizeBuffer);
	SAFE_RELEASE(myRenderData->levelReflectionProbeBuffer);
	SAFE_RELEASE(myRenderData->levelReflectionProbeBufferSRV);
	SAFE_DELETE(myRenderData);
	myMemoryNewCounter--;
	assert(myMemoryNewCounter < 2 && "Did not return enough memory, check memory management");
}

void Engine::CubemapRenderer::RenderCubeMapAndSH(RenderData* aRenderBuffer, v3f aCubeMapPosition, MeshesToRender& meshesToFill, RenderStates* someRenderStates, std::atomic<unsigned int>& aDrawcallCounter, bool aGeneratePrefiltered, float aOuterRadius)
{
	if (myRenderData == nullptr) { assert(false && "Cube Render Data needs to be inited"); return; }
	myRenderData->renderCam.RecalculateProjectionMatrix(90, myRenderData->cubeMapTextureSize, true, 10.f, aOuterRadius);
	myRenderData->cubeMapPos = aCubeMapPosition;
	//Render all the models and light them properly
	myCBufferManager->MapUnMapScreenBuffer(myRenderData->cubeMapTextureSize, myRenderData->cubeMapTextureSize);
	SetDepthStencilState(myContext, *someRenderStates, DEPTHSTENCILSTATE_DEFAULT);
	ID3D11ShaderResourceView* envTexForBakedLight = EngineInterface::GetTextureManager()->GetTextureObject("", ETextureTypes::eEnvironment);
	myContext->PSSetShaderResources(20, 1, &envTexForBakedLight);

	std::atomic<unsigned int> jobCounter = 0;
	for (unsigned int side = 0; side < 6; side++)
	{
		myRenderData->renderCam.GetMatrix().SetTranslation(myRenderData->cubeMapPos);
		myRenderData->renderCam.GetMatrix().SetRotation(myRenderData->cubeRotationMatrixes[side]);
		myShadowRenderer->SortForSpecificCamera(aRenderBuffer, myRenderData->renderCam);
		myShadowRenderer->RenderShadowMaps(aRenderBuffer, meshesToFill, aDrawcallCounter, *myFullscreenRenderer);
		DX::ClearShaderResources(myContext, 8);
		for (unsigned int i = 0; i < 6; i++)
		{
			DX::ClearShaderResources(myContext, i + 21);
		}
		myRenderData->GBufferTarget->ClearTextures();
		myRenderData->cubeDepth->ClearDepth();
		myRenderData->GBufferTarget->SetAsActiveTarget(myRenderData->cubeDepth);

		ClearMeshes(meshesToFill, aRenderBuffer->sortedMeshesSize, aRenderBuffer->sortedAnimMeshesSize);
		SortModelsAndCalculateMVPs(aRenderBuffer, meshesToFill, myRenderData->renderCam, jobCounter, myModelManager);
		SetBlendState(myContext, *someRenderStates, BLENDSTATE_DISABLE);
		SetSampleState(myContext, *someRenderStates, SAMPLERSTATE_TRILINEAR, 0);
		SetSampleState(myContext, *someRenderStates, SAMPLERSTATE_POINT, 1);
		SetSampleState(myContext, *someRenderStates, SAMPLERSTATE_TRILINEARWRAP, 2);
		myCBufferManager->MapUnMapEmptyEffectBuffer();
		myCBufferManager->MapUnMapCameraBuffer(myRenderData->renderCam);
		myDeferredRenderer->GenerateGBuffer(meshesToFill, myRenderData->GBufferTarget, myRenderData->cubeDepth, *aRenderBuffer, *myCBufferManager, myRenderData->renderCam, someRenderStates, myFullscreenRenderer, aDrawcallCounter);
		myRenderData->SSAOtarget->ClearTexture();
		myRenderData->SSAOtarget->SetAsActiveTarget();
		myRenderData->GBufferTarget->SetAllAsResources();
		myRenderData->cubeDepth->SetAsResourceOnSlot(27);
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::SSAO, aDrawcallCounter);

		myRenderData->SSAOBlurredtarget->ClearTexture();
		myRenderData->SSAOBlurredtarget->SetAsActiveTarget();
		myRenderData->SSAOtarget->SetAsResourceOnSlot(8);
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::SSAO_BLUR, aDrawcallCounter);

		myRenderData->cubeRenderTargets[side]->ClearTexture();
		myRenderData->cubeRenderTargets[side]->SetAsActiveTarget();
		myRenderData->SSAOBlurredtarget->SetAsResourceOnSlot(8);
		//////////////////////////////
		SetBlendState(myContext, *someRenderStates, BLENDSTATE_ADDITIVE);
		DX::FillClusterCS(*myCBufferManager, true);
		myDeferredRenderer->RenderBakedLights(aRenderBuffer, aDrawcallCounter);
	}

	CopyCubeTextureResource(aGeneratePrefiltered);

	if (aGeneratePrefiltered)
	{
		PrefilterCubemapResource();
	}
	else
	{
		GenerateSphericalHormonicsSet();
	}
	SetBlendState(myContext, *someRenderStates, BLENDSTATE_DISABLE);

}

void Engine::CubemapRenderer::RenderLightProbeGrid(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, RenderStates* someRenderStates, std::atomic<unsigned int>& aDrawcallCounter)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	SHBakeData& data = myEditorData->gridsBakeData[myEditorData->gridToRender];
	data.gridBakeProgress = (float)(data.progressCounter) / (float)(data.numberOfProbes);
	unsigned int counter = 0;

	for (unsigned int linearIndex = data.linearProgress; linearIndex < data.numberOfProbes; linearIndex++)
	{
		unsigned int currentD = linearIndex % data.bakeD;
		unsigned int currentH = (linearIndex / data.bakeD) % data.bakeH;
		unsigned int currentW = linearIndex / (data.bakeH * data.bakeD);
		int index = (currentD * data.bakeW * data.bakeH) + (currentH * data.bakeW) + currentW;
		v3f gridPos = (data.bakeSize * -1.f) + v3f(data.bakeSpacing * 0.5f, data.bakeSpacing * 0.5f, data.bakeSpacing * 0.5f);
		gridPos.x += (data.bakeSpacing * currentW);
		gridPos.y += (data.bakeSpacing * currentH);
		gridPos.z += (data.bakeSpacing * currentD);
		v4f multGridPos = v4f(gridPos.x, gridPos.y, gridPos.z, 1.f);
		multGridPos = multGridPos * data.bakeRotation;
		RenderCubeMapAndSH(aRenderBuffer, v3f(multGridPos.x, multGridPos.y, multGridPos.z), meshesToFill, someRenderStates, aDrawcallCounter, false, data.bakeSpacing * 4);
		mySHSet[index + myEditorData->currentOffset] = myRenderData->SHToFill;
		counter++;
		data.progressCounter++;
		if (data.progressCounter == data.numberOfProbes)
		{
			if (myEditorData->isRenderingAllGrids)
			{
				myEditorData->currentOffset += data.numberOfProbes;
				myEditorData->gridToRender++;
			}
			else
			{
				myEditorData->gridToRender = myEditorData->amountOfBakeGridsInLevel;
			}
			if (myEditorData->gridToRender == myEditorData->amountOfBakeGridsInLevel)
			{
				myCBufferManager->MapUnmapSHSet(mySHSet, (myEditorData->totalNumberOfBakedSHProbes));
				myEditorData->isRendering = false;
				SetNumberOfLightProbesOnGPU(myEditorData->totalNumberOfBakedSHProbes);
			}
			data.linearProgress = linearIndex;
			return;
		}
		if (counter >= 1)
   		{
			data.linearProgress = linearIndex + 1;
   			return;
   		}
	}
}

void Engine::CubemapRenderer::StartRenderOfAllGrids(unsigned int aNumberOfGrids)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	myEditorData->gridToRender = 0;
	myEditorData->amountOfBakeGridsInLevel = aNumberOfGrids;
	myCBufferManager->myGlobalFrameBufferData.SHGridAmount = myEditorData->amountOfBakeGridsInLevel;
	if (myEditorData->amountOfBakeGridsInLevel > 0)
	{
		myEditorData->isRendering = true;
		myEditorData->isRenderingAllGrids = true;
		SetBakeDataWithGridData(nullptr);
	}
}
void Engine::CubemapRenderer::StartRenderOfGrid(unsigned int aNumberOfGrids, unsigned int aGridToRender)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	myEditorData->gridToRender = aGridToRender;
	myEditorData->amountOfBakeGridsInLevel = aNumberOfGrids;
	myCBufferManager->myGlobalFrameBufferData.SHGridAmount = myEditorData->amountOfBakeGridsInLevel;
	if (myEditorData->amountOfBakeGridsInLevel > 0)
	{
		myEditorData->isRendering = true;
		myEditorData->isRenderingAllGrids = false;
		SetBakeDataWithGridData(nullptr);
	}
}

const bool Engine::CubemapRenderer::GetIsRenderingGrid()
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return false; }
	return myEditorData->isRendering;
}

const bool Engine::CubemapRenderer::GetIsBakingReflectionProbes()
{
	return myBakeReflectionProbes;
}

SHGridData* Engine::CubemapRenderer::GetSHGridData(unsigned int aIndex)
{
	if (myRenderData)
	{
		return &myLoadedGridGPUData[aIndex];
	}
	return nullptr;
}

void Engine::CubemapRenderer::Debug_DrawGrid(Camera& aRenderCamera)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	if (myRenderData == nullptr)
	{
		return;
	}
	for (unsigned int grid = 0; grid < myEditorData->amountOfDebugGridsInLevel; grid++)
	{
		if (myEditorData->gridsToDebugRender[grid] == false)
		{
			continue;
		}
		SHGridData gridData = myLoadedGridGPUData[grid];

		const float gridSpacing = gridData.gridSpacing;
		const v3f gridSize = gridData.gridHalfSize * 2.0f;
		const v3f gridMin = gridData.gridHalfSize * -1.0f;
		const unsigned int width = (int)(gridSize.x / gridSpacing);
		const unsigned int heigth = (int)(gridSize.y / gridSpacing);
		const unsigned int depth = (int)(gridSize.z / gridSpacing);
		std::atomic<unsigned int> tempDrawCallCounter = 0;
	
		myEditorData->meshBuffer->numberOfModels = 0;
		unsigned short& numbModels = myEditorData->meshBuffer->numberOfModels;
		myCBufferManager->MapUnMapEmptyEffectBuffer();
		myContext->PSSetShader(myEditorData->gridPS, nullptr, 0);

		SetDebugGridDataForGPU(grid);
		for (unsigned int linearIndex = 0; linearIndex < width * heigth * depth; linearIndex++)
		{
			const unsigned int currentD = linearIndex % depth;
			const unsigned int currentH = (linearIndex / depth) % heigth;
			const unsigned int currentW = linearIndex / (heigth * depth);
			const unsigned int index = (currentD * width * heigth) + (currentH * width) + currentW;
			v3f gridPos = gridMin + v3f(gridSpacing * 0.5f, gridSpacing * 0.5f, gridSpacing * 0.5f);
			gridPos.x += (gridSpacing * currentW);
			gridPos.y += (gridSpacing * currentH);
			gridPos.z += (gridSpacing * currentD);

			myEditorData->debugSphere.matrix = m4f();
			myEditorData->debugSphere.matrix.SetScale({ 0.4f, 0.4f, 0.4f });
			myEditorData->debugSphere.matrix.SetTranslation(gridPos);
			myEditorData->debugSphere.matrix *= gridData.gridRotation;
			myEditorData->meshBuffer->transforms[numbModels++] = myEditorData->debugSphere.matrix;
			if (numbModels >= NUMB_MODELSPERTYPE)
			{
				DX::RenderInstancedModelBatch(*myEditorData->meshBuffer, *myCBufferManager, DX::EModelRenderMode::EOnlyVertexShader, tempDrawCallCounter, true, true);
				numbModels = 0;
			}
		}
		DX::RenderInstancedModelBatch(*myEditorData->meshBuffer, *myCBufferManager, DX::EModelRenderMode::EOnlyVertexShader, tempDrawCallCounter, true, true);
	}
}

void Engine::CubemapRenderer::Debug_SetDrawSpecificGrid(bool aDrawState, unsigned int aIndex)
{
	if (myEditorData->isDebugRenderingGrid == false)
	{
		myEditorData->isDebugRenderingGrid = true;
	}
	myEditorData->gridsToDebugRender[aIndex] = aDrawState;
}

void Engine::CubemapRenderer::Debug_SetAmbientSHLight(bool aDrawState)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	{ myEditorData->isDebugRenderingSHAmbientLight = aDrawState; }
}

void Engine::CubemapRenderer::Debug_SetDrawGrid(bool aDrawState)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	{ myEditorData->isDebugRenderingGrid = aDrawState; memset(myEditorData->gridsToDebugRender, (int)aDrawState, sizeof(bool) * NUMBOF_RENDERTARGETS); }
}

bool Engine::CubemapRenderer::Debug_DrawGrid()
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return false; }
	{ return myEditorData->isDebugRenderingGrid; }
}

void Engine::CubemapRenderer::Debug_SetNumberOfGrids(unsigned int aNumberOfGrids)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	myEditorData->amountOfDebugGridsInLevel = aNumberOfGrids; 
}

void Engine::CubemapRenderer::Debug_SetNumberOfBakeGrids(unsigned int aNumberOfGrids)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	myEditorData->amountOfBakeGridsInLevel = aNumberOfGrids;
}


void Engine::CubemapRenderer::Debug_SetReflectionProbeToRender(unsigned int aReflectionProbeToRender)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	myEditorData->reflectionProbeToDebug = aReflectionProbeToRender;
}

bool Engine::CubemapRenderer::Debug_DrawReflectionProbe()
{
	if (myEditorData == nullptr) { return false; }
	return true;
}

const bool Engine::CubemapRenderer::IsEditorInited()
{
	if (myEditorData == nullptr) { return false; }
	return true;
}

float Engine::CubemapRenderer::GetProgressOfGridBake(unsigned int aIndex)
{ 
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return 0; }
	if (myEditorData->isRenderingAllGrids == false)
	{
		return myEditorData->gridsBakeData[aIndex].gridBakeProgress;
	}
	else
	{
		return myEditorData->gridsBakeData[myEditorData->gridToRender].gridBakeProgress;
	}
}

float Engine::CubemapRenderer::GetTotalProgressOfGridBakes()
{ 
	return (float)(myEditorData->gridToRender) / (float)(myEditorData->amountOfBakeGridsInLevel);
}

SH* Engine::CubemapRenderer::GetSHLevelData()
{
	if (myRenderData == nullptr) { assert(false && "Cube Render Data needs to be inited"); return nullptr; }
	return mySHSet;
}

void Engine::CubemapRenderer::SetDebugGridDataForGPU(unsigned int aIndex)
{
	if (myRenderData == nullptr) { assert(false && "Cube Render Data needs to be inited"); return; }
	m4f oldRotation = myLoadedGridGPUData[aIndex].gridRotation;
	myLoadedGridGPUData[aIndex].gridRotation = m4f::GetFastInverse(myLoadedGridGPUData[aIndex].gridRotation);
	SHGridData* dataToMap = &myLoadedGridGPUData[aIndex];
	DX::MapUnmapConstantBuffer(myContext, myEditorData->SHGridBuffer, dataToMap, sizeof(SHGridData), UINT_MAX, UINT_MAX, 9);
	myLoadedGridGPUData[aIndex].gridRotation = oldRotation;
}

void Engine::CubemapRenderer::SetBakeDataWithGridData(SH* aHarmonicSet)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	myEditorData->SHOffsetCounter = 0;
	myEditorData->totalNumberOfBakedSHProbes = 0;
	for (unsigned int grid = 0; grid < myEditorData->amountOfBakeGridsInLevel; grid++)
	{
		myEditorData->gridsBakeData[grid].bakeSpacing = myLoadedGridGPUData[grid].gridSpacing;
		myEditorData->gridsBakeData[grid].bakeW = (int)((myLoadedGridGPUData[grid].gridHalfSize.x * 2) / myEditorData->gridsBakeData[grid].bakeSpacing);
		myEditorData->gridsBakeData[grid].bakeH = (int)((myLoadedGridGPUData[grid].gridHalfSize.y * 2) / myEditorData->gridsBakeData[grid].bakeSpacing);
		myEditorData->gridsBakeData[grid].bakeD = (int)((myLoadedGridGPUData[grid].gridHalfSize.z * 2) / myEditorData->gridsBakeData[grid].bakeSpacing);
		myEditorData->gridsBakeData[grid].bakeSize = myLoadedGridGPUData[grid].gridHalfSize;
		myEditorData->gridsBakeData[grid].progressCounter = 0;
		myEditorData->gridsBakeData[grid].linearProgress = 0;
		myEditorData->gridsBakeData[grid].bakeAmount = (float)((myEditorData->gridsBakeData[grid].bakeW) * (myEditorData->gridsBakeData[grid].bakeH) * (myEditorData->gridsBakeData[grid].bakeD));
		myEditorData->gridsBakeData[grid].bakeRotation = (myLoadedGridGPUData[grid].gridRotation);
		myLoadedGridGPUData[grid].gridRotation = m4f::GetFastInverse(myLoadedGridGPUData[grid].gridRotation); //Inversing the rotation data for the GPU
		//This is to add the offset of where the grid will sample from on the GPU
		myLoadedGridGPUData[grid].numberOfHarmonics = myEditorData->gridsBakeData[grid].bakeW * myEditorData->gridsBakeData[grid].bakeH * myEditorData->gridsBakeData[grid].bakeD;
		myEditorData->gridsBakeData[grid].numberOfProbes = myLoadedGridGPUData[grid].numberOfHarmonics;
		myLoadedGridGPUData[grid].globalOffset = myEditorData->SHOffsetCounter;
		myEditorData->SHOffsetCounter += myLoadedGridGPUData[grid].numberOfHarmonics;
		myEditorData->totalNumberOfBakedSHProbes += myLoadedGridGPUData[grid].numberOfHarmonics;
	}
	myEditorData->currentOffset = 0;
	if (myEditorData->isRenderingAllGrids == false)
	{
		for (unsigned int grid = 0; grid < myEditorData->gridToRender; grid++)
		{
			myEditorData->currentOffset += myLoadedGridGPUData[grid].numberOfHarmonics;
		}
	}
	UpdateSHLightProbeData(myEditorData->totalNumberOfBakedSHProbes, aHarmonicSet);
	UpdateGridsGPUData((unsigned short)myEditorData->amountOfBakeGridsInLevel);
}

void Engine::CubemapRenderer::UpdateGridsGPUDataWithEdtiorData()
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	if (myEditorData->amountOfBakeGridsInLevel == 0)
	{
		return;
	}

	SHGridData pushData [NUMBOF_SHGRIDS];
	for (unsigned int grid = 0; grid < myEditorData->amountOfBakeGridsInLevel; grid++)
	{
		pushData[grid] = myLoadedGridGPUData[grid];
		pushData[grid].gridSpacing = myEditorData->gridsBakeData[grid].bakeSpacing;
		pushData[grid].gridHalfSize = myEditorData->gridsBakeData[grid].bakeSize;
		pushData[grid].gridRotation = m4f::GetFastInverse(myEditorData->gridsBakeData[grid].bakeRotation);
	}

	ID3D11Buffer* levelGridBuffer = nullptr;
	ID3D11ShaderResourceView* levelGridBufferSRV = nullptr;
	if (DX::CreateStructuredBuffer(myDevice, &levelGridBufferSRV, &levelGridBuffer, myEditorData->amountOfBakeGridsInLevel, sizeof(SHGridData)))
	{
		DX::MapUnmapDynamicBuffer(myContext, levelGridBufferSRV, levelGridBuffer, pushData, sizeof(SHGridData), (unsigned short)myEditorData->amountOfBakeGridsInLevel, 38, false, false, true, true);
	}
	levelGridBuffer->Release();
	levelGridBufferSRV->Release();
}

void Engine::CubemapRenderer::UpdateGridsGPUData(unsigned short aGridAmount)
{
	//Mapping the grid data to the GPU
	myLoadedGridAmount = aGridAmount;
	unsigned int gridSHOffset = 0;
	for (size_t grid = 0; grid < myLoadedGridAmount; grid++)
	{
		myLoadedGridGPUData[grid].globalOffset = gridSHOffset;
		gridSHOffset += myLoadedGridGPUData[grid].numberOfHarmonics;
	}
	ID3D11ShaderResourceView* NULLSRV = nullptr;
	myContext->PSSetShaderResources(38, 1, &NULLSRV);
	myContext->CSSetShaderResources(38, 1, &NULLSRV);
	ID3D11Buffer* levelGridBuffer = nullptr;
	ID3D11ShaderResourceView* levelGridBufferSRV = nullptr;
	if (DX::CreateStructuredBuffer(myDevice, &levelGridBufferSRV, &levelGridBuffer, aGridAmount, sizeof(SHGridData)))
	{
		DX::MapUnmapDynamicBuffer(myContext, levelGridBufferSRV, levelGridBuffer, myLoadedGridGPUData, sizeof(SHGridData), aGridAmount, 38, false, false, true, true);
	}
	else
	{
		assert(false && "Failed to create light grid Data on the GPU");
	}
	levelGridBuffer->Release();
	levelGridBufferSRV->Release();
	myTotalLightProbeCount = 0;
	for (size_t grid = 0; grid < myLoadedGridAmount; grid++)
	{
		myTotalLightProbeCount += myLoadedGridGPUData[grid].numberOfHarmonics;
	}
}

void Engine::CubemapRenderer::UpdateSHLightProbeData(unsigned int aLightProbeAmount, SH* aSHSet)
{
	SH* oldData = nullptr;
	unsigned int oldSize = myBakedTotalLightProbeCount;
	if (mySHSet)
	{
		oldData = new SH[myBakedTotalLightProbeCount];
		memcpy(&oldData[0], &mySHSet[0], sizeof(SH) * myBakedTotalLightProbeCount);
		delete[] mySHSet;
		mySHSet = nullptr;
	}
	mySHSet = new SH[aLightProbeAmount];
	if (oldSize <= aLightProbeAmount)
	{
		if (aSHSet)
		{
			memcpy(&mySHSet[0], &aSHSet[0], sizeof(SH) * aLightProbeAmount);
		}
		else
		{
			memcpy(&mySHSet[0], &oldData[0], sizeof(SH) * oldSize);
		}
	}
}

void Engine::CubemapRenderer::BakeLevelReflectionProbes(unsigned int aReflectionProbeAmount, EditorReflectionProbe* someLoadedProbesToRender, bool aAddReflectionProbes)
{
	ID3D11ShaderResourceView* NULLSRV = nullptr;
	myContext->PSSetShaderResources(36, 1, &NULLSRV);
	myContext->PSSetShaderResources(37, 1, &NULLSRV);
	myContext->CSSetShaderResources(37, 1, &NULLSRV);
	if (myRenderData->levelReflectionProbes.GetIsInited())
	{
		myRenderData->levelReflectionProbes.ReleaseResources();
	}
	myRenderData->levelReflectionProbes.InitWithFullMip(myDevice, myContext, { 128, 128 }, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false, aReflectionProbeAmount);
	if (aAddReflectionProbes)
	{
		for (size_t reflectionProbe = myLoadedReflectionProbeAmount; reflectionProbe < myLoadedReflectionProbeAmount + aReflectionProbeAmount; reflectionProbe++)
		{
			EditorReflectionProbe& probe = someLoadedProbesToRender[reflectionProbe - myLoadedReflectionProbeAmount];
			v3f pos = probe.transform.GetPosition();
			myLoadedLevelReflectionProbesData[reflectionProbe].position = probe.transform.GetPosition();
			myLoadedLevelReflectionProbesData[reflectionProbe].brightness = probe.brightness;
			myLoadedLevelReflectionProbesData[reflectionProbe].innerRadius = (probe.innerRadius * 10.0f) * 0.5f;
			myLoadedLevelReflectionProbesData[reflectionProbe].outerRadius = (probe.outerRadius * 10.0f) * 0.5f;
		}
		myLoadedReflectionProbeAmount += aReflectionProbeAmount;
	}
	else
	{
		myLoadedReflectionProbeAmount = aReflectionProbeAmount;
		for (size_t reflectionProbe = 0; reflectionProbe < aReflectionProbeAmount; reflectionProbe++)
		{
			EditorReflectionProbe& probe = someLoadedProbesToRender[reflectionProbe];
			v3f pos = probe.transform.GetPosition();
			myLoadedLevelReflectionProbesData[reflectionProbe].position = probe.transform.GetPosition();
			myLoadedLevelReflectionProbesData[reflectionProbe].brightness = probe.brightness;
			myLoadedLevelReflectionProbesData[reflectionProbe].innerRadius = (probe.innerRadius * 10.0f) * 0.5f;
			myLoadedLevelReflectionProbesData[reflectionProbe].outerRadius = (probe.outerRadius * 10.0f) * 0.5f;
		}
	}

	myBakeReflectionProbes = true;
}
void Engine::CubemapRenderer::Debug_RenderReflectionProbe(Camera& aRenderCamera)
{
	if (myEditorData == nullptr) { assert(false && "Cube Editor Data needs to be inited"); return; }
	if (myEditorData->reflectionProbeToDebug != INVALID_ENTITY)
	{
		v4f debugData;
		debugData.x = (float)myEditorData->reflectionProbeToDebug;
		DX::MapUnmapConstantBuffer(myContext, myRenderData->cubemapRenderSizeBuffer, &debugData, sizeof(v4f), UINT_MAX, UINT_MAX, 6, UINT_MAX);

		ReflectionProbe& probe = myLoadedLevelReflectionProbesData[myEditorData->reflectionProbeToDebug];
		myEditorData->debugSphere.matrix.SetTranslation(probe.position);
		myContext->PSSetShader(myEditorData->spherePS, nullptr, 0);
		myRenderData->cubeMap.SetAsResourceOnSlot(8);
		myEditorData->debugSphere.matrix.SetScale({ 1.4f, 1.4f, 1.4f });
		std::atomic<unsigned int> drawCallCounter = 0;
		DX::RenderModel(myEditorData->debugSphere, *myCBufferManager, DX::EModelRenderMode::EOnlyVertexShader, drawCallCounter, true, true, true);
	}
}
void Engine::CubemapRenderer::RenderReflectionProbes(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, RenderStates* someRenderStates, std::atomic<unsigned int>& aDrawcallCounter)
{
	if (myRenderData == nullptr) { assert(false && "Cube Render Data needs to be inited"); return; }
	if (myLoadedReflectionProbeAmount > 0)
	{
		D3D11_TEXTURE2D_DESC texArrayDesc;
		D3D11_TEXTURE2D_DESC texArrayDescMipped;
		myRenderData->levelReflectionProbes.GetTexture()->GetDesc(&texArrayDescMipped);
		myRenderData->filteredCubeMap.GetTexture()->GetDesc(&texArrayDesc);
		D3D11_BOX sourceRegion;
		for (unsigned int reflectionProbe = 0; reflectionProbe < myLoadedReflectionProbeAmount; reflectionProbe++)
		{
			RenderCubeMapAndSH(aRenderBuffer, myLoadedLevelReflectionProbesData[reflectionProbe].position, meshesToFill, someRenderStates, aDrawcallCounter, true, myLoadedLevelReflectionProbesData[reflectionProbe].outerRadius * 2.0f);
			myLoadedLevelReflectionProbesData[reflectionProbe].irradianceLight = myRenderData->SHToFill;
			for (UINT mipLevel = 0; mipLevel < texArrayDescMipped.MipLevels; mipLevel++)
			{
				sourceRegion.left = 0;
				sourceRegion.right = (texArrayDescMipped.Width >> mipLevel);
				sourceRegion.top = 0;
				sourceRegion.bottom = (texArrayDescMipped.Height >> mipLevel);
				sourceRegion.front = 0;
				sourceRegion.back = 1;
				//test for overflow
				if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
					break;
				//for each texture slice in the mip level
				for (unsigned int x = 0; x < 6; x++)
				{
					myContext->CopySubresourceRegion(myRenderData->levelReflectionProbes.GetTexture(), D3D11CalcSubresource(mipLevel, x + 6 * reflectionProbe, texArrayDescMipped.MipLevels), 0, 0, 0, myRenderData->filteredCubeMap.GetTexture(), D3D11CalcSubresource(mipLevel, x, texArrayDesc.MipLevels), &sourceRegion);
				}
			}
		}
		DX::MapUnmapDynamicBuffer(myContext, myRenderData->levelReflectionProbeBufferSRV, myRenderData->levelReflectionProbeBuffer, myLoadedLevelReflectionProbesData, sizeof(ReflectionProbe), (unsigned short)myLoadedReflectionProbeAmount, 37, false, false, true, true);
		myCBufferManager->myGlobalFrameBufferData.reflectionProbeAmount = myLoadedReflectionProbeAmount;
		myRenderData->levelReflectionProbes.SetAsResourceOnSlot(36);
		myBakeReflectionProbes = false;
	}

}

const SHGridData Engine::CubemapRenderer::GetSHLevelBakeData(unsigned int aIndex)
{
	SHGridData data;
	data.gridSpacing = myEditorData->gridsBakeData[aIndex].bakeSpacing;
	data.gridHalfSize = myEditorData->gridsBakeData[aIndex].bakeSize;
	data.numberOfHarmonics = myEditorData->gridsBakeData[aIndex].numberOfProbes;
	data.gridRotation = m4f::GetFastInverse(myEditorData->gridsBakeData[aIndex].bakeRotation);
	return data;
}

void Engine::CubemapRenderer::GenerateSphericalHormonicsSet()
{
	float R[9];
	float G[9];
	float B[9];
	HRESULT result;
	result = DirectX::SHProjectCubeMap(myContext, 3, myRenderData->cubeMap.GetTexture(), R, G, B);
	assert(result == S_OK && "Failed to generate SH Set");
	for (size_t b = 0; b < 9; b++)
	{
		myRenderData->SHToFill.bands[b][0] = R[b];
		myRenderData->SHToFill.bands[b][1] = G[b];
		myRenderData->SHToFill.bands[b][2] = B[b];
		myRenderData->SHToFill.bands[b][3] = 0;
	}
}

void Engine::CubemapRenderer::PrefilterCubemapResource()
{
	D3D11_TEXTURE2D_DESC texArrayDescMipped;
	myRenderData->filteredCubeMap.GetTexture()->GetDesc(&texArrayDescMipped);
	myRenderData->cubeMap.SetAsCSResourceOnSlot(8);
	myContext->GenerateMips(myRenderData->filteredCubeMap.GetSRV());
	v4f dispatchData;
	D3D11_BOX sourceRegion;
	sourceRegion.top = 0;
	sourceRegion.left = 0;
	sourceRegion.front = 0;
	sourceRegion.back = 1;
	for (UINT mipLevel = 0; mipLevel < texArrayDescMipped.MipLevels; mipLevel++)
	{
		dispatchData.x = (float)mipLevel / (float)(texArrayDescMipped.MipLevels - 1);
		dispatchData.y = (float)mipLevel;
		dispatchData.z = (float)myRenderData->cubeDownSampledTargets_Lods[mipLevel]->GetResolution().x;
		DX::MapUnmapConstantBuffer(myContext, myRenderData->cubemapRenderSizeBuffer, &dispatchData, sizeof(v4f), UINT_MAX, UINT_MAX, UINT_MAX, 3);
		myRenderData->cubeDownSampledTargets_Lods[mipLevel]->SetAsCSOutput(0);
		v2ui res = myRenderData->cubeDownSampledTargets_Lods[mipLevel]->GetResolution();
		myContext->CSSetShader(myRenderData->preBlurShader, nullptr, 0);
		res.x = CU::Max(1U, res.x / texArrayDescMipped.MipLevels);
		res.y = CU::Max(1U, res.y / texArrayDescMipped.MipLevels);
		myContext->Dispatch(res.x, res.y, 1);

		ID3D11UnorderedAccessView* NULLUAV = nullptr;
		myContext->CSSetUnorderedAccessViews(0, 1, &NULLUAV, 0);
		sourceRegion.right = (texArrayDescMipped.Width >> mipLevel);
		sourceRegion.bottom = (texArrayDescMipped.Height >> mipLevel);
		//test for overflow
		if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
			break;
		//for each texture slice in the mip level
		D3D11_TEXTURE2D_DESC texArrayDesc;
		myRenderData->cubeDownSampledTargets_Lods[mipLevel]->GetTexture()->GetDesc(&texArrayDesc);
		for (unsigned int x = 0; x < 6; x++)
		{
			myContext->CopySubresourceRegion(myRenderData->filteredCubeMap.GetTexture(), D3D11CalcSubresource(mipLevel, x, texArrayDescMipped.MipLevels), 0, 0, 0, myRenderData->cubeDownSampledTargets_Lods[mipLevel]->GetTexture(), D3D11CalcSubresource(0, x, texArrayDesc.MipLevels), &sourceRegion);
		}

	}
	myContext->CSSetShader(nullptr, nullptr, 0);
	ID3D11ShaderResourceView* NULLSRV = nullptr;
	myContext->CSSetShaderResources(8, 1, &NULLSRV);
	GenerateSphericalHormonicsSet();
}

void Engine::CubemapRenderer::CopyCubeTextureResource(bool aGeneratePrefiltered)
{
	D3D11_TEXTURE2D_DESC texArrayDesc;
	D3D11_TEXTURE2D_DESC texArrayDescMipped;
	myRenderData->filteredCubeMap.GetTexture()->GetDesc(&texArrayDescMipped);
	myRenderData->cubeMap.GetTexture()->GetDesc(&texArrayDesc);
	D3D11_BOX sourceRegion;
	//Copy the mip map levels of the textures for either filtering or use in SH calculation
	for (UINT x = 0; x < 6; x++)
	{
		if (aGeneratePrefiltered)
		{
			for (UINT mipLevel = 0; mipLevel < texArrayDescMipped.MipLevels; mipLevel++)
			{
				sourceRegion.left = 0;
				sourceRegion.right = (texArrayDesc.Width >> mipLevel);
				sourceRegion.top = 0;
				sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
				sourceRegion.front = 0;
				sourceRegion.back = 1;

				//test for overflow
				if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
					break;
				myContext->CopySubresourceRegion(myRenderData->filteredCubeMap.GetTexture(), D3D11CalcSubresource(mipLevel, x, texArrayDescMipped.MipLevels), 0, 0, 0, myRenderData->cubeRenderTargets[x]->GetTexture(), mipLevel, &sourceRegion);
			}
		}
		for (UINT mipLevel = 0; mipLevel < texArrayDesc.MipLevels; mipLevel++)
		{
			sourceRegion.left = 0;
			sourceRegion.right = (texArrayDesc.Width >> mipLevel);
			sourceRegion.top = 0;
			sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
			sourceRegion.front = 0;
			sourceRegion.back = 1;
			//test for overflow
			if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
				break;
			myContext->CopySubresourceRegion(myRenderData->cubeMap.GetTexture(), D3D11CalcSubresource(mipLevel, x, texArrayDesc.MipLevels), 0, 0, 0, myRenderData->cubeRenderTargets[x]->GetTexture(), mipLevel, &sourceRegion);
		}
	}
}


