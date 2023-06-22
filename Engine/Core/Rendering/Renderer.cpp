#include "stdafx.h"
#include "Renderer.h"
#include "..\DirectXFramework.h"
#include "EngineInterface.h"

#include "Renderers\Renderers.h"

#include "GameObjects/Model.h"
#include "GameObjects/ModelInstance.h"

#include "Managers\Managers.h"
#include "Resources\GBuffer.h"
#include <d3d11.h>
#include <assert.h>

#include "CU\Input\InputManager.h"
#include "CU\Containers\ThreadPool.h"
#include "CU\Utility\Timer\Timer.h"
#include "Resources\FullScreenTexture.h"
#include "Resources\FullScreenTexture_Factory.h"
#include "DX_Functions/DX_RenderFunctions.h"

#include "ECS/Systems/Animation/AnimationSystem.h"
#include "imgui\imgui.h"
#include "GameObjects\ParticleEmitter_Instance.h"

#include "RenderData.h"
#include "Resources\LerpRenderBuffers.h"
#include "Resources\CameraClusterCreation.h"
#include "Resources\GBuffer.h"
#include "Renderers\SingelModelRenderer.h"

#include <../Engine/RenderData.h>
#include "Resources\HandleRendererDataPackage.h"



Engine::Renderer::Renderer()
{
	myBackBuffer = nullptr;
	myIntermediateDepth = nullptr;
	myIntermediateTexture = nullptr;

	myToneMapTexture = nullptr;
	myHalfSizeTexture = nullptr;
	myQuarterSizeTexture = nullptr;
	myBlurTexture1 = nullptr;
	myBlurTexture2 = nullptr;
}

bool Engine::Renderer::Init(DirectXFramework* aFrameworkPtr, CU::Timer* aTimer, const CU::Color aBackGroundColor, EngineManagers* someManagers, v2ui& aRenderResToFill)
{
	myInputManager = &someManagers->myInputManager;
	myParticleManager = &someManagers->myParticleManager;
	myFramework = aFrameworkPtr;
	myModelManager = &someManagers->myModelManager;
	myCurrentDebugState = UINT_MAX;

	if (!aFrameworkPtr)
	{
		return false;
	}
	myContext = aFrameworkPtr->GetDeviceContext();
	if (!myContext)
	{
		return false;
	}
	myDevice = aFrameworkPtr->GetDevice();
	if (!myDevice)
	{
		return false;
	}
	myRenderers = new Renderers();
	myRenderers->Forward.Init(aFrameworkPtr, myModelManager);

	if (!myRenderers->Fullscreen.Init(myDevice, myContext))
	{
		return false;
	}

	if (!myRenderers->Deferred.Init(myDevice, myContext, myModelManager))
	{
		return false;
	}
#ifndef _DISTRIBUTION
	if (!myRenderers->Debug.Init(myDevice, myContext))
	{
		return false;
	}
#endif
	myRenderers->Sprite.Init(myDevice, myContext, myConstantBufferManager);
	
	if (!myRenderers->Particle.Init(myDevice, myContext))
	{
		return false;
	}
	if (!myRenderers->Shadow.Init(myDevice, myContext, &myConstantBufferManager))
	{
		return false;
	}
	if (!InitRenderStates(myDevice, myRenderStates))
	{
		return false;
	}
	unsigned int numberOfMipsInReflectionProbes = 0;
	if (!myRenderers->Cube.Init(myDevice, myContext, &myConstantBufferManager, &myRenderers->Fullscreen, &myRenderers->Deferred, &myRenderers->Shadow, numberOfMipsInReflectionProbes))
	{
		return false;
	}

	myConstantBufferManager.Init(myDevice, myContext, myTimer, numberOfMipsInReflectionProbes);

	if (!myEffectManager.Init(myDevice, myContext, &myRenderers->Fullscreen, &myConstantBufferManager, this, {1280, 720} ))
	{
		return false;
	}

	myWindowResolution = aFrameworkPtr->GetWindowResolution();
	aRenderResToFill = OnResize(myWindowResolution, myWindowResolution);
	myTimer = aTimer;
	myBackgroundColor = aBackGroundColor;

	myMeshesToRender.myUniqueNormalMeshes.Init(100);
	myMeshesToRender.myUniqueAnimatedNormalMeshes.Init(100);
	myMeshesToRender.myUniqueStaticMeshes.Init(100);

	myMeshesToRender.myUniqueForwardMeshes.Init(100);
	myMeshesToRender.myUniqueFWD_AnimMeshes.Init(100);

	myMeshesToRender.myUniqueOutlineMeshes.Init(100);
	myMeshesToRender.myUniqueOutlinedAnimMeshes.Init(100);

	myMeshesToRender.myUniqueTCutoutMeshes.Init(100);
	myMeshesToRender.myUniqueTCutout_AnimMeshes.Init(100);

	myMeshesToRender.myRenderTargetMeshes.Init(20);

	myMeshesToRender.particlesGlowing.buffer = new ParticleRenderCommand[MAX_PARTICLES];
	myMeshesToRender.particlesStandard.buffer = new ParticleRenderCommand[MAX_PARTICLES];

	GetLastReadBuffer()->camera.RecalculateProjectionMatrix(90, myRenderResolution);
	GetReadBuffer()->camera.RecalculateProjectionMatrix(90, myRenderResolution);
	myRenderData = new RenderData();
	myRenderData->name = "Render Buffer";

	myID_PS = DX::LoadPS(myDevice, "Content/Shaders/PS_IdPicking.cso");
	if (!myID_PS)
	{
		return false;
	}
	mySSAOBlur_CS = DX::LoadCS(myDevice, "Content/Shaders/CS_Fullscreen_SSAO.cso");
	if (!mySSAOBlur_CS)
	{
		return false;
	}
	HRESULT result;
	D3D11_BUFFER_DESC constantBufferDescription = { 0 };
	constantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	constantBufferDescription.ByteWidth = sizeof(v4f);
	result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myGBufferDebugBuffer);
	if (FAILED(result)) { return false; }
	ID3D11ShaderResourceView* envTexForBakedLight = EngineInterface::GetTextureManager()->GetTextureObject("", ETextureTypes::eEnvironment);
	myContext->PSSetShaderResources(20, 1, &envTexForBakedLight);

	return true;
}

void Engine::Renderer::Render(std::atomic<unsigned int>& aDrawcallCounter, bool aIgnoreShadowRendering)
{
	if (myRenderers->Cube.IsEditorInited() == false) { SetRenderDebugState(); }
	if (GetReadBuffer()->numberOfDataPackages > 0)
	{
		HandlePreRenderDataPackages(myRenderData, GetReadBuffer(), this, &myConstantBufferManager, myMeshesToRender, &myRenderStates, &myEffectManager);
	}
	RenderData* bufferFrom = GetReadBuffer(); // GetLerpFromRenderBuffer()
	RenderData* bufferTo = GetLastReadBuffer();
	if (bufferFrom->resolutionChanged || bufferTo->resolutionChanged)
	{
		bufferFrom->camera.RecalculateProjectionMatrix(myRenderResolution);
		bufferTo->camera.RecalculateProjectionMatrix(myRenderResolution);
	}
	myFrameTValue = (float)globalRenderData.frameTimer.load(std::memory_order_relaxed) / INV_STEPF;
	//Lerping the buffers
	LerpBuffers(GetReadBuffer(), GetLastReadBuffer(), myFrameTValue, myRenderData, myMeshesToRender, myJobCounter, myModelManager);
	//Setting Global Frame Data
	GlobalFrameBufferData& globalFrameData = myConstantBufferManager.myGlobalFrameBufferData;
	globalFrameData.deltaTime = (float)myTimer->GetDeltaTime();
	globalFrameData.totalTime = (float)myTimer->GetTotalTime();
	LerpCommonBufferData(bufferFrom, bufferTo, myFrameTValue, myRenderData, globalFrameData);
	myConstantBufferManager.MapUnMapGlobalFrameBuffer();

	if (GetReadBuffer()->numberOfDataPackages > 0)
	{
		HandleDataPackages(myRenderData, GetReadBuffer(), this, &myConstantBufferManager, myMeshesToRender, &myRenderStates);
	}

	if (myRenderers->Cube.IsEditorInited())
	{
		if (myRenderers->Cube.GetIsRenderingGrid())
		{
			myRenderers->Cube.RenderLightProbeGrid(myRenderData, myMeshesToRender, &myRenderStates, aDrawcallCounter);
		}
	}
	if (myRenderers->Cube.GetIsBakingReflectionProbes())
	{
		if (myWaitForFrameSwitch == false)
		{
			myRenderers->Cube.RenderReflectionProbes(myRenderData, myMeshesToRender, &myRenderStates, aDrawcallCounter);
			if (myRenderers->Cube.IsEditorInited() == false)
			{
				myRenderers->Cube.DeInitGPUData();
			}
		}
	}

	ParticleManager& pm = *myParticleManager;
	pm.Update(myRenderData->particles, myRenderData->particlesSize, (float)myTimer->GetDeltaTime());
	pm.FillMeshParticles(myMeshesToRender);

	if (aIgnoreShadowRendering == false)
	{
		myRenderers->Shadow.SortLights(myRenderData, myRenderData->camera);
		myRenderers->Shadow.RenderDirectionalShadowMap(myRenderData, myMeshesToRender, myRenderData->camera, aDrawcallCounter);
		myRenderers->Shadow.RenderShadowMaps(myRenderData, myMeshesToRender, aDrawcallCounter, myRenderers->Fullscreen);
	}

	///-----------------Rendering the Views-------------------------///
	for (size_t i = 0; i < NUMBOF_RENDERTARGETS; i++)
	{
		if (myRenderData->renderTargetcameraFlags[PO2(i)])
		{
			(*myRenderData->renderTargetCameras[i].texture)->ClearTexture({0, 0, 0, 0});
			myConstantBufferManager.MapUnMapScreenBuffer(myWindowResolution, (*myRenderData->renderTargetCameras[i].texture)->GetResolution());
			FullScreenTexture* depth = *myRenderData->renderTargetCameras[i].depthTexture;
			depth->ClearDepth();
			RenderToTarget(*myRenderData->renderTargetCameras[i].texture, depth, *myRenderData->renderTargetCameras[i].intermediateTexture, *myRenderData->renderTargetCameras[i].gBufferTexture, myRenderData->renderTargetCameras[i].renderFlag, myRenderData->renderTargetCameras[i].camera, aDrawcallCounter, false, false, false);
		}
	}
	if (myRenderData->renderNormalView)
	{
		myConstantBufferManager.MapUnMapScreenBuffer(myWindowResolution, myRenderResolution);
		myIntermediateTexture->ClearTexture();
		myIntermediateDepth->ClearDepth();
		if (myCurrentDebugState != UINT_MAX)
		{
			myRenderData->mainRenderFlag = RenderFlag_GbufferDebug;
		}
		RenderToTarget(myIntermediateTexture, myIntermediateDepth, myDefferedTexture, myGBuffer, myRenderData->mainRenderFlag, myRenderData->camera, aDrawcallCounter, true, false, false);
	}

	if (myBuffersAreSwapped)
	{
		myWaitForFrameSwitch = false;
	}
	myBuffersAreSwapped = false;
}

void Engine::Renderer::RenderSpecifiedTarget(RenderTarget& aTargetToRender)
{
	std::atomic<unsigned int> aTempDrawCallCounter;
	if (*aTargetToRender.intermediateTexture)
	{
		if (aTargetToRender.renderFlag == RenderFlag::RenderFlag_AllPasses
			|| aTargetToRender.renderFlag == RenderFlag::RenderFlag_NoUiOrPost
			|| aTargetToRender.renderFlag == RenderFlag::RenderFlag_NoUI
			|| aTargetToRender.renderFlag == RenderFlag::RenderFlag_LightingOnly)
		{
			myRenderers->Shadow.SortLights(myRenderData, aTargetToRender.camera);
			myRenderers->Shadow.RenderDirectionalShadowMap(myRenderData, myMeshesToRender, myRenderData->camera, aTempDrawCallCounter);
			myRenderers->Shadow.RenderShadowMaps(myRenderData, myMeshesToRender, aTempDrawCallCounter, myRenderers->Fullscreen);
		}
		aTargetToRender.camera.GetMatrix() = aTargetToRender.camera.GetTransform().GetMatrix();
		RenderToTarget(*aTargetToRender.texture, *aTargetToRender.depthTexture, *aTargetToRender.intermediateTexture, *aTargetToRender.gBufferTexture, aTargetToRender.renderFlag, aTargetToRender.camera, aTempDrawCallCounter, false, false, false);
	}
}

void Engine::Renderer::RenderModelToResource(Model* aModel, FullScreenTexture** aTextureToFill, RenderTarget& aTargetToRender, v3f dirLightRot)
{
	std::atomic<unsigned int> aTempDrawCallCounter;
	ClearRenderDataAndModels();
	ClearMeshes(myMeshesToRender, myRenderData->sortedMeshesSize, myRenderData->sortedAnimMeshesSize);
	myConstantBufferManager.myGlobalFrameBufferData.directionalColor = { 1, 1, 1, 1 };
	myConstantBufferManager.myGlobalFrameBufferData.directionalDirection = dirLightRot;
	myConstantBufferManager.myGlobalFrameBufferData.ambientLightColor = { .1f, .1f, .1f, .1f };
	myConstantBufferManager.myGlobalFrameBufferData.pointLightCount = 0;
	myConstantBufferManager.myGlobalFrameBufferData.spotLightCount = 0;
	myConstantBufferManager.MapUnMapGlobalFrameBuffer();

	SortMesh(myMeshesToRender, aModel, aTargetToRender.camera, myModelManager);
	bool previousState = myRenderWithHarmonics;
	myRenderWithHarmonics = false;
	RenderToTarget(*aTextureToFill, *aTargetToRender.depthTexture, *aTargetToRender.intermediateTexture, *aTargetToRender.gBufferTexture, aTargetToRender.renderFlag, aTargetToRender.camera, aTempDrawCallCounter, false, true, true);
	myRenderWithHarmonics = previousState;
}

void Engine::Renderer::RenderModelAnimatedToResource(ModelAnimated** aModel, FullScreenTexture** aTextureToFill, RenderTarget& aTargetToRender, m4f* aSkeleton, bool aDrawSkeleton, unsigned int aNumberOfModels, v3f dirLightRot, bool aRenderWithFloor, CU::Color aMeshColor)
{
	std::atomic<unsigned int> aTempDrawCallCounter;
	ClearRenderDataAndModels();
	ClearMeshes(myMeshesToRender, myRenderData->sortedMeshesSize, myRenderData->sortedAnimMeshesSize);
	myConstantBufferManager.myGlobalFrameBufferData.directionalColor = { 1, 1, 1, 1 };
	myConstantBufferManager.myGlobalFrameBufferData.directionalDirection = dirLightRot;
	myConstantBufferManager.myGlobalFrameBufferData.ambientLightColor = { 20, 20, 60, 100 };
	myConstantBufferManager.myGlobalFrameBufferData.pointLightCount = 0;
	myConstantBufferManager.myGlobalFrameBufferData.spotLightCount = 0;
	myConstantBufferManager.MapUnMapGlobalFrameBuffer();
	ID3D11ShaderResourceView* environmentTexture = EngineInterface::GetTextureManager()->GetTextureObject("", ETextureTypes::eEnvironment);
	myContext->PSSetShaderResources(20, 1, &environmentTexture);
	SortMeshAnimated(myMeshesToRender, aModel, aTargetToRender.camera, aSkeleton, aNumberOfModels, aRenderWithFloor, aMeshColor);
	bool previousState = myRenderWithHarmonics;
	myRenderWithHarmonics = false;
	RenderToTarget(*aTextureToFill, *aTargetToRender.depthTexture, *aTargetToRender.intermediateTexture, *aTargetToRender.gBufferTexture, aTargetToRender.renderFlag, aTargetToRender.camera, aTempDrawCallCounter, false, true, true);
	myRenderWithHarmonics = previousState;
}

void Engine::Renderer::RenderModelParticleToResource(ParticleRenderCommand* aCommand, size_t aNumbOfCommands, ParticleMeshRenderCommand* aMeshC, size_t aNumbOfMeshes, FullScreenTexture** aTextureToFill, RenderTarget& aTargetToRender, v3f dirLightRot, bool aRenderGround)
{
	std::atomic<unsigned int> aTempDrawCallCounter;
	ClearRenderDataAndModels();
	ClearMeshes(myMeshesToRender, myRenderData->sortedMeshesSize, myRenderData->sortedAnimMeshesSize);
	myConstantBufferManager.myGlobalFrameBufferData.directionalDirection = dirLightRot;
	myConstantBufferManager.myGlobalFrameBufferData.directionalColor = { 0.8f, 0.8f, 0.8f, 0.5f };
	myConstantBufferManager.myGlobalFrameBufferData.ambientLightColor = { 25, 25, 25, 50 };
	myConstantBufferManager.myGlobalFrameBufferData.pointLightCount = 0;
	myConstantBufferManager.myGlobalFrameBufferData.spotLightCount = 0;
	myConstantBufferManager.MapUnMapGlobalFrameBuffer();
	SortMeshParticle(myMeshesToRender, aNumbOfCommands, aCommand, aTargetToRender.camera, aRenderGround);
	myMeshesToRender.particleMeshCount = 0;
	for (size_t i = 0; i < aNumbOfMeshes; i++)
	{
		myMeshesToRender.particlesMesh[myMeshesToRender.particleMeshCount++] = aMeshC[i];
	}
	bool previousState = myRenderWithHarmonics;
	myRenderWithHarmonics = false;
	RenderToTarget(*aTextureToFill, *aTargetToRender.depthTexture, *aTargetToRender.intermediateTexture, *aTargetToRender.gBufferTexture, aTargetToRender.renderFlag, aTargetToRender.camera, aTempDrawCallCounter, false, true, true);
	myRenderWithHarmonics = previousState;
}

void Engine::Renderer::RenderToTarget(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, GBuffer* aGbufferTarget, RenderFlag aRenderFlag, Camera& aRenderCamera, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender, bool aCustomModelBuffer, bool aIgnoreParticles)
{
	ID3D11ShaderResourceView* NULLSRV = nullptr;
	//myRenderers->Shadow.RenderDirectionalShadowMap(myRenderData, myMeshesToRender, aRenderCamera, aDrawcallCounter);
	myConstantBufferManager.MapUnMapCameraBuffer(aRenderCamera);
	if (aCustomModelBuffer == false)
	{
		PreFrame();
		ClearMeshes(myMeshesToRender, myRenderData->sortedMeshesSize, myRenderData->sortedAnimMeshesSize);
		SortModelsAndCalculateMVPs(myRenderData, myMeshesToRender, aRenderCamera, myJobCounter, myModelManager);
		RenderData& renderBuffer = *myRenderData;
		ParticleManager& pm = *myParticleManager;
		pm.UpdateDepthSort(aRenderCamera.GetMatrix().GetTranslationVector());
	}
	myConstantBufferManager.MapUnMapEmptyEffectBuffer();
	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEAR, 0);
	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_POINT, 1);
	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEARWRAP, 2);
	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_CLAMP_COMPARISON, 3);
	switch (aRenderFlag)
	{
	case RenderFlag::RenderFlag_AllPasses:
		RenderDeferredPasses(aTarget, aTargetDepth, aGbufferTarget, aRenderCamera, aRenderFlag, aDrawcallCounter, isMainRender, aIgnoreParticles);
		RenderRenderTargetpass(aTarget, aTargetDepth, aIntermediateTarget, aDrawcallCounter, isMainRender);
		RenderPostProcessingPass(aTarget, aTargetDepth, aIntermediateTarget, aDrawcallCounter, isMainRender);
		RenderUIPass(aTarget, aDrawcallCounter, aRenderCamera);
		break;
	case RenderFlag::RenderFlag_Wireframe:
		SetRasterizerState(myContext, myRenderStates, RasterizerState::RASTERIZERSTATE_WIREFRAME);
		RenderDeferredPasses(aTarget, aTargetDepth, aGbufferTarget, aRenderCamera, aRenderFlag, aDrawcallCounter, isMainRender, aIgnoreParticles);
		break;
	case RenderFlag::RenderFlag_NoUiOrPost:
		RenderDeferredPasses(aTarget, aTargetDepth, aGbufferTarget, aRenderCamera, aRenderFlag, aDrawcallCounter, isMainRender, aIgnoreParticles);
		myRenderers->Sprite.RenderWorldSpaceUI(myRenderData->wsprites, myRenderData->wspritesSize, &aRenderCamera);
		RenderRenderTargetpass(aTarget, aTargetDepth, aIntermediateTarget, aDrawcallCounter, isMainRender);
		break;
	case RenderFlag::RenderFlag_IndexPass:
		RenderIndexPass(aTarget, aTargetDepth, aDrawcallCounter);
		break;
	case RenderFlag::RenderFlag_GbufferDebug:
	{
		RenderDeferredPasses(aTarget, aTargetDepth, aGbufferTarget, aRenderCamera, aRenderFlag, aDrawcallCounter, isMainRender, aIgnoreParticles);
		v4f debugBufferData;
		debugBufferData.w = (float)myCurrentDebugState;		
		DX::MapUnmapConstantBuffer(myContext, myGBufferDebugBuffer, &debugBufferData, sizeof(v4f), UINT_MAX, UINT_MAX, 9);

		aIntermediateTarget->SetAsActiveTarget();
		SetDepthStencilState(myContext, myRenderStates, DEPTHSTENCILSTATE_OFF);
		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		myBlurredSSAOTexture->SetAsResourceOnSlot(8);
		aTargetDepth->SetAsResourceOnSlot(27);
		if (myCurrentDebugState == 8)
		{
			SetBlendState(myContext, myRenderStates, BLENDSTATE_ADDITIVE);
			myConstantBufferManager.MapUnmapLightBuffer(*myRenderData);
		}
		myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::GBUFFER_DEBUG, aDrawcallCounter);
		if (myCurrentDebugState == 8){ SetBlendState(myContext, myRenderStates, BLENDSTATE_DISABLE); }

		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		aTarget->SetAsActiveTarget();
		aIntermediateTarget->SetAsResourceOnSlot(8);
		myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
	}
		break;
	case RenderFlag::RenderFlag_NoLighting:
		RenderDeferredPasses(aTarget, aTargetDepth, aGbufferTarget, aRenderCamera, aRenderFlag, aDrawcallCounter, isMainRender, aIgnoreParticles);
		RenderRenderTargetpass(aTarget, aTargetDepth, aIntermediateTarget, aDrawcallCounter, isMainRender);
		break;
	case RenderFlag::RenderFlag_LightingOnly:
		RenderDeferredPasses(aTarget, aTargetDepth, aGbufferTarget, aRenderCamera, aRenderFlag, aDrawcallCounter, isMainRender, aIgnoreParticles);
		break;
	case RenderFlag::RenderFlag_UIOnly:
		aTarget->SetAsActiveTarget();
		RenderUIPass(aTarget, aDrawcallCounter, aRenderCamera);
		break;
	case RenderFlag::RenderFlag_NoUI:
		RenderDeferredPasses(aTarget, aTargetDepth, aGbufferTarget, aRenderCamera, aRenderFlag, aDrawcallCounter, isMainRender, aIgnoreParticles);
		RenderRenderTargetpass(aTarget, aTargetDepth, aIntermediateTarget, aDrawcallCounter, isMainRender);
		RenderPostProcessingPass(aTarget, aTargetDepth, aIntermediateTarget, aDrawcallCounter, isMainRender);
		break;
	default:
		break;
	}
	if (isMainRender || myRenderers->Cube.IsEditorInited())
	{
		myBackBuffer->SetAsActiveTarget();
		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		aTarget->SetAsResourceOnSlot(8);
		myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY_LETTERBOX, aDrawcallCounter);
	}
	SetDepthStencilState(myContext, myRenderStates, DEPTHSTENCILSTATE_DEFAULT);
	SetBlendState(myContext, myRenderStates, BLENDSTATE_DISABLE);
	SetRasterizerState(myContext, myRenderStates, RASTERIZERSTATE_DEFAULT);

	ID3D11ShaderResourceView* null[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	myContext->PSSetShaderResources(8, 12, null);

}

void Engine::Renderer::PreFrame()
{
	//SetRenderDebugState();
	myBackBuffer->ClearTexture();
	myOutlineBuffer->ClearDepth();
	myToneMapTexture->ClearTexture();
	myHalfSizeTexture->ClearTexture();
	myQuarterSizeTexture->ClearTexture();
	myBlurTexture1->ClearTexture();
	myBlurTexture2->ClearTexture();
	myDefferedTexture->ClearTexture();

}

void Engine::Renderer::PrepareForResize()
{
	myBackBuffer->ReleaseResources();
	SAFE_DELETE(myBackBuffer);
	myIntermediateDepth->ReleaseResources();
	SAFE_DELETE(myIntermediateDepth);
	myOutlineBuffer->ReleaseResources();
	SAFE_DELETE(myOutlineBuffer);
	myIntermediateTexture->ReleaseResources();
	SAFE_DELETE(myIntermediateTexture);
	myToneMapTexture->ReleaseResources();
	SAFE_DELETE(myToneMapTexture);
	myHalfSizeTexture->ReleaseResources();
	SAFE_DELETE(myHalfSizeTexture);
	myQuarterSizeTexture->ReleaseResources();
	SAFE_DELETE(myQuarterSizeTexture);
	myBlurTexture1->ReleaseResources();
	SAFE_DELETE(myBlurTexture1);
	myBlurTexture2->ReleaseResources();
	SAFE_DELETE(myBlurTexture2);
	myDefferedTexture->ReleaseResources();
	SAFE_DELETE(myDefferedTexture);
	myGBuffer->ReleaseResources();
	SAFE_DELETE(myGBuffer);
}

v2ui Engine::Renderer::OnResize(v2ui aResolution, v2ui aWindowResolution)
{
	ID3D11Resource* backBufferResource = nullptr;

	myFramework->GetBackBuffer()->GetResource(&backBufferResource);
	ID3D11Texture2D* backBufferTexture = reinterpret_cast<ID3D11Texture2D*>(backBufferResource);
	float newRatio = (float)aResolution.x / (float)aResolution.y;

	v2ui renderTargetResolution = aResolution;
	if (newRatio > my19_9Ratio)
	{
		renderTargetResolution.x = (unsigned int)((float)renderTargetResolution.y * my19_9Ratio);
	}
	if (newRatio < my19_9Ratio)
	{
		renderTargetResolution.y = (unsigned int)((float)renderTargetResolution.x / my19_9Ratio);
	}
	myWindowResolution = aWindowResolution;
	myBackBuffer = CreateFullScreenTextureFromTexture(backBufferTexture, myDevice, myContext, true);
	myIntermediateDepth = CreateDepthTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE, EDepthStencilFlag::BOTH);
	myOutlineBuffer = CreateDepthTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE, EDepthStencilFlag::BOTH);
	myIntermediateTexture = CreateFullScreenTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, myDevice, myContext);
	myDefferedTexture = CreateFullScreenTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, myDevice, myContext);
	myToneMapTexture = CreateFullScreenTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, myDevice, myContext);
	myBlurTexture1 = CreateFullScreenTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, myDevice, myContext);
	myBlurTexture2 = CreateFullScreenTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, myDevice, myContext);
	mySSAOTexture = CreateFullScreenTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT, myDevice, myContext);
	myBlurredSSAOTexture = CreateFullScreenTexture(renderTargetResolution, DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT, myDevice, myContext);
	myHalfSizeTexture = CreateFullScreenTexture({ (unsigned int)((float)renderTargetResolution.x * 0.5f), (unsigned int)((float)renderTargetResolution.y * 0.5f) }, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, myDevice, myContext);
	myQuarterSizeTexture = CreateFullScreenTexture({ (unsigned int)((float)renderTargetResolution.x * 0.25f), (unsigned int)((float)renderTargetResolution.y * 0.25f) }, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, myDevice, myContext);
	myGBuffer = CreateGBuffer(renderTargetResolution, myDevice, myContext);
	myRenderResolution = { (float)renderTargetResolution.x, (float)renderTargetResolution.y };

	myConstantBufferManager.MapUnMapScreenBuffer(myWindowResolution, myRenderResolution);
	GetLastReadBuffer()->resolutionChanged = true;
	GetReadBuffer()->resolutionChanged = true;
	myEffectManager.ResizeScreenTextures(renderTargetResolution);
	return renderTargetResolution;
}

Engine::SpriteRenderer* Engine::Renderer::GetSpriteRenderer()
{
	return &myRenderers->Sprite;
}

Engine::CubemapRenderer* Engine::Renderer::GetCubeMapRenderer()
{
	return &myRenderers->Cube;
}

void Engine::Renderer::RenderSplatMap()
{
	myRenderLevelTopDown = true;
}

void Engine::Renderer::ShutDownRenderer()
{
}

void Engine::Renderer::SetPostProcessingData(PostProcessingData somePPData)
{
	memcpy(&myConstantBufferManager.myPostProcessingData, &somePPData, sizeof(PostProcessingData));
	myConstantBufferManager.MapUnmapPostProcessing();
}

void Engine::Renderer::SetRenderDebugState()
{
	if (GetAsyncKeyState(VK_F6) & 0x01) { myCurrentDebugState++; }
	if (myCurrentDebugState > 8){ myCurrentDebugState = UINT_MAX; }
}

void Engine::Renderer::RenderDeferredPasses(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, GBuffer* aGbufferTarget, Camera& aRenderCamera, RenderFlag aRenderFlag, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender, bool aIgnoreParticles)
{
	myConstantBufferManager.MapUnMapScreenBuffer(myWindowResolution, aTarget->GetResolution());
	bool renderLights = true;
	bool renderOnlyLights = false;
	switch (aRenderFlag)
	{
	case RenderFlag::RenderFlag_Wireframe:
		renderLights = false;
		break;
	case RenderFlag::RenderFlag_GbufferDebug:
		break;
	case RenderFlag::RenderFlag_NoLighting:
		renderLights = false;
		break;
	case RenderFlag::RenderFlag_LightingOnly:
		renderOnlyLights = true;
		break;
	default:
		break;
	}

	///////////////////////BUILD GBuffer
	aGbufferTarget->ClearTextures();
	aGbufferTarget->SetAsActiveTarget(aTargetDepth);

	myRenderers->Deferred.GenerateGBuffer(myMeshesToRender, aGbufferTarget, aTargetDepth, *myRenderData,
		myConstantBufferManager, aRenderCamera, &myRenderStates, &myRenderers->Fullscreen, aDrawcallCounter);
	
	mySSAOTexture->SetAsActiveTarget();
	aGbufferTarget->SetAllAsResources();
	aTargetDepth->SetAsResourceOnSlot(27);

	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEAR, 0);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::SSAO, aDrawcallCounter);
	
	myBlurredSSAOTexture->SetAsActiveTarget();
	mySSAOTexture->SetAsResourceOnSlot(8); 
	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEAR, 0);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::SSAO_BLUR, aDrawcallCounter);

	aTarget->SetAsActiveTarget();
	myBlurredSSAOTexture->SetAsResourceOnSlot(8);

	//////////////////////////////
	SetBlendState(myContext, myRenderStates, BLENDSTATE_ADDITIVE);
	if (renderLights)
	{
		myConstantBufferManager.MapUnmapLightBuffer(*myRenderData);
		DX::FillClusterCS(myConstantBufferManager, false);
		if (myRenderIrradianceOnly)
		{
			myRenderers->Deferred.RenderIrradiantLight(myRenderData, aDrawcallCounter);
		}
		else
		{
			myRenderers->Deferred.RenderLights(myRenderData, aDrawcallCounter, myRenderWithHarmonics);
		}
	}
	else
	{
		aGbufferTarget->SetAsResourceOnSlot(ALBEDO_AO, 8);
		myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
	}
	//////////////////////////////////////////
	//ForwardRendering
	aTarget->SetAsActiveTarget(aTargetDepth);

	if (myRenderers->Cube.IsEditorInited())
	{
		SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_DISABLE);
		if (myRenderers->Cube.Debug_DrawGrid())
		{
			myRenderers->Cube.Debug_DrawGrid(aRenderCamera);
		}
		if (myRenderers->Cube.Debug_DrawReflectionProbe())
		{
			myRenderers->Cube.Debug_RenderReflectionProbe(aRenderCamera);
		}
	}

	SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_ALPHABLEND);
	//SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEARWRAP);

	myRenderers->Forward.RenderTransparentCutoutMeshes(myMeshesToRender, aDrawcallCounter, myConstantBufferManager, renderOnlyLights, renderLights);

	SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_ALPHABLEND_NOCOVERAGE);
	SetDepthStencilState(myContext, myRenderStates, DepthStencilState::DEPTHSTENCILSTATE_READONLY);
	myRenderers->Forward.RenderTranslucent(myMeshesToRender, aDrawcallCounter, myConstantBufferManager, isMainRender);

	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_POINT, 0);
	SetBlendState(myContext, myRenderStates, BLENDSTATE_DISABLE);
	SetDepthStencilState(myContext, myRenderStates, DepthStencilState::DEPTHSTENCILSTATE_READONLY_STENCIL_WRITEDISCARDED);
	aGbufferTarget->SetAsResourceOnSlot(EGBufferTexture::DEPTH, 26);
	aTargetDepth->SetAsResourceOnSlot(27);
	myRenderers->Forward.RenderOutlines(myMeshesToRender, this, aTargetDepth, aTarget, aDrawcallCounter, myConstantBufferManager);
	DX::ClearShaderResources(myContext, 3);

	////PARTICLES
	//SetBlendState(myContext, myRenderStates, BLENDSTATE_DISABLE);
	aGbufferTarget->SetAsResourceOnSlot(EGBufferTexture::DEPTH, 26);
	aTarget->SetAsActiveTarget(aTargetDepth);
	if (aIgnoreParticles == false)
	{
		for (unsigned short i = 0; i < myParticleManager->GetNumberOfRenderCommands(); i++)
		{
			if (myParticleManager->myRenderCommands[i].myIsMeshParticle == false)
			{
				if (myParticleManager->myRenderCommands[i].myMaterial->myMaterialType == MaterialTypes::EParticle_Glow)
				{
					myMeshesToRender.particlesGlowing.buffer[myMeshesToRender.particlesGlowing.myNumberOfSystems] = myParticleManager->myRenderCommands[i];
					myMeshesToRender.particlesGlowing.myNumberOfSystems++;
				}
				else
				{
					myMeshesToRender.particlesStandard.buffer[myMeshesToRender.particlesStandard.myNumberOfSystems] = myParticleManager->myRenderCommands[i];
					myMeshesToRender.particlesStandard.myNumberOfSystems++;
				}
			}
		}
	}
	SetDepthStencilState(myContext, myRenderStates, DEPTHSTENCILSTATE_READONLY);
	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEARWRAP, 0);
	SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_ALPHABLEND_NOCOVERAGE);
	myRenderers->Particle.RenderParticles(myMeshesToRender.particlesStandard, myConstantBufferManager, aRenderCamera);
	SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_ADDITIVE);
	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEARWRAP, 0);
	myRenderers->Particle.RenderParticles(myMeshesToRender.particlesGlowing, myConstantBufferManager, aRenderCamera);


#ifndef _DISTRIBUTION
	SetDepthStencilState(myContext, myRenderStates, DEPTHSTENCILSTATE_DEFAULT);
	SetRasterizerState(myContext, myRenderStates, RASTERIZERSTATE_DOUBLESIDED);
	myRenderers->Debug.Render(myRenderData, myRenderStates.rasterizerStates, aRenderCamera);
	SetRasterizerState(myContext, myRenderStates, RASTERIZERSTATE_DEFAULT);
	SetDepthStencilState(myContext, myRenderStates, DEPTHSTENCILSTATE_READONLY);
#endif
	DX::ClearShaderResources(myContext, 8);
	SetBlendState(myContext, myRenderStates, BLENDSTATE_DISABLE);
}

void Engine::Renderer::RenderPostProcessingPass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender)
{
	ID3D11ShaderResourceView* NULLSRV = nullptr;
	aIntermediateTarget->ClearTexture();
	myHalfSizeTexture->SetAsActiveTarget();
	aTargetDepth->SetAsResourceOnSlot(27);
	aTarget->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

	myQuarterSizeTexture->SetAsActiveTarget();
	myHalfSizeTexture->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

	myBlurTexture1->SetAsActiveTarget();
	myQuarterSizeTexture->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

	myBlurTexture2->SetAsActiveTarget();
	myBlurTexture1->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::GAUSSIANHORIZONTAL, aDrawcallCounter);

	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	myBlurTexture1->SetAsActiveTarget();
	myBlurTexture2->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::GAUSSIANVERTICAL, aDrawcallCounter);

	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	myBlurTexture2->SetAsActiveTarget();
	myBlurTexture1->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::GAUSSIANHORIZONTAL, aDrawcallCounter);

	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	myBlurTexture1->SetAsActiveTarget();
	myBlurTexture2->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::GAUSSIANVERTICAL, aDrawcallCounter);

	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	myQuarterSizeTexture->SetAsActiveTarget();
	myBlurTexture1->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	myHalfSizeTexture->SetAsActiveTarget();
	myQuarterSizeTexture->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	myBlurTexture1->SetAsActiveTarget();
	aTarget->SetAsResourceOnSlot(8);
	myHalfSizeTexture->SetAsResourceOnSlot(9);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::BLOOM, aDrawcallCounter);



	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	aTarget->SetAsActiveTarget();
	aTargetDepth->SetAsResourceOnSlot(8);
	myBlurTexture1->SetAsResourceOnSlot(9);
	//SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_ALPHABLEND_NOCOVERAGE);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::ATMOSPHERE, aDrawcallCounter);
	//SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_DISABLE);

	SetSampleState(myContext, myRenderStates, SAMPLERSTATE_TRILINEAR, 0);
	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	aIntermediateTarget->SetAsActiveTarget();
	aTarget->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::CHROMATICABBERATION, aDrawcallCounter);

	//aTarget->SetAsResourceOnSlot(8);
	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	aTarget->SetAsActiveTarget();
	aIntermediateTarget->SetAsResourceOnSlot(8);
	//myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::TONEMAPPING, aDrawcallCounter);

	if (isMainRender)
	{
		myEffectManager.RenderDoF(aTarget, aIntermediateTarget, aDrawcallCounter);
	}

	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	aIntermediateTarget->SetAsActiveTarget();
	aTarget->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);


	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	aTarget->SetAsActiveTarget();
	aIntermediateTarget->SetAsResourceOnSlot(8);
	//aTarget->SetAsActiveTarget();
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::VIGNETTE, aDrawcallCounter);
}

void Engine::Renderer::RenderUIPass(FullScreenTexture* aTarget, std::atomic<unsigned int>& aDrawcallCounter, Camera& aRenderCamera)
{
	//////////////////// RENDERING SPRITESU ////////////////////////////
	SetBlendState(myContext, myRenderStates, BLENDSTATE_ALPHABLEND_NOCOVERAGE);
	myRenderers->Sprite.RenderWorldSpaceUI(myRenderData->wsprites, myRenderData->wspritesSize, &aRenderCamera);
	myRenderers->Sprite.RenderScreenSpaceUI(myRenderData->sprites, myRenderData->spritesSize, aTarget->GetResolution());
}

void Engine::Renderer::RenderIndexPass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, std::atomic<unsigned int>& aDrawcallCounter)
{
	if (aTargetDepth)
	{
		aTarget->SetAsActiveTarget(aTargetDepth);
	}
	else
	{
		aTarget->SetAsActiveTarget();
	}
	myContext->PSSetShader(myID_PS, nullptr, 0);
	for (unsigned short i = 0; i < myMeshesToRender.normalMeshListCount; i++)
	{
		DX::RenderInstancedModelBatch(myMeshesToRender.myNormalMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.staticMeshCount; i++)
	{
		DX::RenderInstancedModelBatch(myMeshesToRender.myStaticMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.forwardMeshListCount; i++)
	{
		DX::RenderInstancedModelBatch(myMeshesToRender.myForwardMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, false);
	}
	for (unsigned short i = 0; i < myMeshesToRender.transparentCutoutCount; i++)
	{
		DX::RenderInstancedModelBatch(myMeshesToRender.myTransparentCutoutMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, false);
	}

	for (unsigned short i = 0; i < myMeshesToRender.animNormalCount; i++)
	{
		DX::RenderInstancedAnimatedModelBatch(myMeshesToRender.myNormalAnimMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.animFwdListCount; i++)
	{
		DX::RenderInstancedAnimatedModelBatch(myMeshesToRender.myFWD_AnimMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, false);
	}
	for (unsigned short i = 0; i < myMeshesToRender.animTCutoutCount; i++)
	{
		DX::RenderInstancedAnimatedModelBatch(myMeshesToRender.animTCutOutMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, false);
	}

	for (unsigned short i = 0; i < myMeshesToRender.myUniqueNormalMeshes.Size(); i++)
	{
		DX::RenderModel(myMeshesToRender.myUniqueNormalMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.myUniqueStaticMeshes.Size(); i++)
	{
		DX::RenderModel(myMeshesToRender.myUniqueStaticMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.myUniqueForwardMeshes.Size(); i++)
	{
		DX::RenderModel(myMeshesToRender.myUniqueForwardMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.myUniqueTCutoutMeshes.Size(); i++)
	{
		DX::RenderModel(myMeshesToRender.myUniqueTCutoutMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.myRenderTargetMeshes.Size(); i++)
	{
		DX::RenderModel(myMeshesToRender.myRenderTargetMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}

	for (unsigned short i = 0; i < myMeshesToRender.myUniqueAnimatedNormalMeshes.Size(); i++)
	{
		DX::RenderAnimatedModel(myMeshesToRender.myUniqueAnimatedNormalMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.myUniqueFWD_AnimMeshes.Size(); i++)
	{
		DX::RenderAnimatedModel(myMeshesToRender.myUniqueFWD_AnimMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	for (unsigned short i = 0; i < myMeshesToRender.myUniqueTCutout_AnimMeshes.Size(); i++)
	{
		DX::RenderAnimatedModel(myMeshesToRender.myUniqueTCutout_AnimMeshes[i], myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true, true);
	}
	myRenderers->Sprite.RenderWorldSpaceUINoSortForPicking(myRenderData->wsprites, myRenderData->wspritesSize);
}

void Engine::Renderer::RenderRenderTargetpass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender)
{
	const unsigned short commandsSize = myMeshesToRender.myRenderTargetMeshes.Size();
	if (isMainRender && commandsSize > 0)
	{
		aIntermediateTarget->SetAsActiveTarget();
		ID3D11ShaderResourceView* NULLSRV = nullptr;
		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		aTarget->SetAsResourceOnSlot(8);
		myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);


		myContext->PSSetShaderResources(27, 1, &NULLSRV);
		aIntermediateTarget->SetAsActiveTarget(aTargetDepth);
		SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_ALPHABLEND);
		for (unsigned short m = 0; m < commandsSize; m++)
		{
			if (myRenderData->renderTargetCameras[myMeshesToRender.myRenderTargetMeshes[m].effectData.gBufferPSEffectIndex].texture)
			{
				DX::RenderRenderTargetModel(myMeshesToRender.myRenderTargetMeshes[m], myConstantBufferManager, DX::EModelRenderMode::EWholeMaterial, aDrawcallCounter, *myRenderData->renderTargetCameras[myMeshesToRender.myRenderTargetMeshes[m].effectData.gBufferPSEffectIndex].texture);
			}
		}
		myConstantBufferManager.MapUnMapEmptyEffectBuffer();
		SetBlendState(myContext, myRenderStates, BlendState::BLENDSTATE_DISABLE);

		aTarget->SetAsActiveTarget();
		aIntermediateTarget->SetAsResourceOnSlot(8);
		myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
	}
}

void Engine::Renderer::RenderGBufferDebugPass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender)
{

	ID3D11ShaderResourceView* NULLSRV = nullptr;
	myContext->PSSetShaderResources(8, 1, &NULLSRV);
	aTarget->SetAsResourceOnSlot(8);
	myRenderers->Fullscreen.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

}

void Engine::Renderer::ClearRenderDataAndModels()
{
	myRenderData->decalsSize = 0;
	myRenderData->particlesSize = 0;
	myRenderData->sortedAnimMeshesSize = 0;
	myRenderData->pointLightsSize = 0;
	myRenderData->sortedMeshesSize = 0;
	myRenderData->spotLightsSize = 0;
	myRenderData->wspritesSize = 0;
	myRenderData->uniqueMeshesSize = 0;
	myRenderData->uniqueAnimatedMeshesSize = 0;

#ifndef _DISTRIBUTION
	myRenderData->debugLinesSize = 0;
	myRenderData->debugSpheresSize = 0;
#endif

	PreFrame();
}
