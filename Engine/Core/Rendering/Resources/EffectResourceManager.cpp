#include "stdafx.h"
#include "EffectResourceManager.h"
#include "FullScreenTexture.h"
#include "FullScreenTexture_Factory.h"

#include "../Renderers/FullScreenRenderer.h"
#include "ConstantBufferManager.h"
#include "../DX_Functions/DX_RenderFunctions.h"
#include "../Renderer.h"

#include "EngineInterface.h"
#include "Managers\ModelManager.h"
#include "Managers\TextureManager.h"

#include "GameObjects\Model.h"
namespace Engine
{
	bool EffectResourceManager::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, FullScreenRenderer* aFullscreenRenderer, ConstantBufferManager* aCBufferManager, Renderer* aRenderer, v2ui aRenderResolution)
	{
		myDevice = aDevice;
		myContext = aContext;
		myFullscreenRenderer = aFullscreenRenderer;
		myCBM = aCBufferManager;
		myRenderer = aRenderer;

		ID3D11PixelShader* PSprecomputeBlur = DX::LoadPS(myDevice, "Content/Shaders/PS_PrecomputeBlurShader.cso");
		if (!PSprecomputeBlur)
		{
			return false;
		}
		FullScreenTexture* precomputedColors = CreateFullScreenTexture({ 68, 1 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true);
		myContext->PSSetShader(PSprecomputeBlur, nullptr, 0);
		precomputedColors->SetAsActiveTarget();
		myFullscreenRenderer->RenderToCustomPS();

		ID3D11RenderTargetView* NULLRTV = nullptr;
		myContext->OMSetRenderTargets(1, &NULLRTV, nullptr);
		precomputedColors->SetAsResourceOnSlot(39);
		precomputedColors->ReleaseResources();
		delete precomputedColors;
		PSprecomputeBlur->Release();

		return true;
	}
	bool EffectResourceManager::Icon_InitResources()
	{
		v2ui res = { 256, 256 };
		myIcon_depth = CreateDepthTexture(res, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE, EDepthStencilFlag::BOTH);
		myIcon_intermediate = CreateFullScreenTexture(res, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true);
		myIcon_gBuffer = CreateGBuffer(res, myDevice, myContext);
		myIcon_RT = new RenderTarget();
		myIcon_RT->camera.RecalculateProjectionMatrix(50, res, true, 5, 2500.f, true);
		if (myIcon_gBuffer == nullptr || myIcon_gBuffer == nullptr || myIcon_gBuffer == nullptr || myIcon_gBuffer == nullptr || myIcon_gBuffer == nullptr )
		{
			Icon_ReleaseResources();
			return false;
		}
		myIcon_RT->depthTexture = &myIcon_depth;
		myIcon_RT->gBufferTexture = &myIcon_gBuffer;
		myIcon_RT->intermediateTexture = &myIcon_intermediate;
		myIcon_RT->renderFlag = RenderFlag_NoUiOrPost;
		return true;
	}
	void EffectResourceManager::Icon_ReleaseResources()
	{
		myIcon_depth->ReleaseResources();
		SAFE_DELETE(myIcon_depth);
		myIcon_intermediate->ReleaseResources();
		SAFE_DELETE(myIcon_intermediate);
		myIcon_gBuffer->ReleaseResources();
		SAFE_DELETE(myIcon_gBuffer);
		SAFE_DELETE(myIcon_RT);
	}
	//void EffectResourceManager::Icon_Render(void* aIconResourceToRender, unsigned int aIconType)
	//{
	//	std::atomic<unsigned int> drawcallCounter = 0;
	//	switch ((ELoadPackageTypes)aIconType)
	//	{
	//	case ELoadPackageTypes::PlayerPortraits:
	//	{
	//		PlayerPortraitPackage* portraitResource = (PlayerPortraitPackage*)aIconResourceToRender;
	//		ModelAnimated** modelsToRender = new ModelAnimated*[portraitResource->numberOfMeshes];
	//		CU::AABB3Df boundingVolume;
	//		for (unsigned short model = 0; model < portraitResource->numberOfMeshes; model++)
	//		{
	//			modelsToRender[model] = EngineInterface::GetModelManager()->GetAnimatedModel(portraitResource->meshes[model]);

	//			CU::AABB3Df collider = modelsToRender[model]->GetCollider();
	//			if (collider.myMin.x < boundingVolume.myMin.x) boundingVolume.myMin.x = collider.myMin.x;
	//			if (collider.myMin.y < boundingVolume.myMin.y) boundingVolume.myMin.y = collider.myMin.y;
	//			if (collider.myMin.z < boundingVolume.myMin.z) boundingVolume.myMin.z = collider.myMin.z;
	//			if (collider.myMax.x > boundingVolume.myMax.x) boundingVolume.myMax.x = collider.myMax.x;
	//			if (collider.myMax.y > boundingVolume.myMax.y) boundingVolume.myMax.y = collider.myMax.y;
	//			if (collider.myMax.z > boundingVolume.myMax.z) boundingVolume.myMax.z = collider.myMax.z;
	//		}
	//		v3f focusPoint = v3f(0, 235.f, 0);
	//		v3f cameraPos = v3f(0, 240.f, 265.f);
	//		myIcon_RT->camera.LookAt(focusPoint, cameraPos);
	//		myIcon_RT->camera.GetMatrix() = myIcon_RT->camera.GetTransform().GetMatrix();
	//		TextureManager* textureManager = EngineInterface::GetTextureManager();
	//		FullScreenTexture* finalTarget = textureManager->GetIconPortraitResource(portraitResource->playerID);
	//		myIcon_RT->texture = &finalTarget;
	//		m4f* skeleton = new m4f[modelsToRender[0]->myAnimationData.myIndexedSkeleton.Size()];
	//		finalTarget->ClearTexture(CU::Color(0, 0, 0, 0));
	//		myIcon_depth->ClearDepth();
	//		m4f lightDir;
	//		lightDir.LookAt(v3f(0, 0, 0), v3f(150, 250, 250));
	//		myRenderer->RenderModelAnimatedToResource(modelsToRender, &finalTarget, *myIcon_RT, skeleton, false, (unsigned int)portraitResource->numberOfMeshes, lightDir.GetForwardVector(), false, portraitResource->playerColor);

	//		myIcon_intermediate->SetAsActiveTarget();
	//		finalTarget->SetAsResourceOnSlot(8);
	//		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::COPY, drawcallCounter);
	//		ID3D11ShaderResourceView* nullSRV = nullptr;
	//		myContext->PSSetShaderResources(8, 1, &nullSRV);
	//		finalTarget->SetAsActiveTarget();
	//		myIcon_intermediate->SetAsResourceOnSlot(8);
	//		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::CLEAR_BLACKPIXELS, drawcallCounter);


	//		delete[] modelsToRender;
	//		delete[] skeleton;
	//	}
	//	break;
	//	case ELoadPackageTypes::PresetIcon:
	//	{
	//		PresetPackage* presetResource = (PresetPackage*)aIconResourceToRender;
	//		ModelAnimated* modelToRender = nullptr;
	//		CU::AABB3Df boundingVolume;
	//		modelToRender = EngineInterface::GetModelManager()->GetAnimatedModel(presetResource->mesh);
	//		if (modelToRender == nullptr)
	//		{
	//			return;
	//		}
	//		CU::AABB3Df collider = modelToRender->GetCollider();
	//		if (collider.myMin.x < boundingVolume.myMin.x) boundingVolume.myMin.x = collider.myMin.x;
	//		if (collider.myMin.y < boundingVolume.myMin.y) boundingVolume.myMin.y = collider.myMin.y;
	//		if (collider.myMin.z < boundingVolume.myMin.z) boundingVolume.myMin.z = collider.myMin.z;
	//		if (collider.myMax.x > boundingVolume.myMax.x) boundingVolume.myMax.x = collider.myMax.x;
	//		if (collider.myMax.y > boundingVolume.myMax.y) boundingVolume.myMax.y = collider.myMax.y;
	//		if (collider.myMax.z > boundingVolume.myMax.z) boundingVolume.myMax.z = collider.myMax.z;
	//		v3f focusPoint = boundingVolume.GetMiddlePosition();
	//		v3f cameraPos = v3f(boundingVolume.myMin.x * 0.5f, boundingVolume.myMax.y + 100, boundingVolume.GetSize().Length() * 0.8f);
	//		//cameraPos.Normalize();
	//		//cameraPos *= boundingVolume.GetSize().Length() * 0.65f;
	//		//cameraPos.y += boundingVolume.myMax.y * 0.25f;
	//		cameraPos = v3f(-60, 310, 310.f);
	//		m4f matrix;
	//		matrix.LookAt(focusPoint, cameraPos);
	//		myIcon_RT->camera.GetMatrix() = matrix;
	//		TextureManager* textureManager = EngineInterface::GetTextureManager();
	//		FullScreenTexture* finalTarget = textureManager->GetIconPresetResource(presetResource->id);
	//		myIcon_RT->texture = &finalTarget;
	//		finalTarget->ClearTexture(CU::Color(0, 0, 0, 0));
	//		myIcon_depth->ClearDepth();
	//		m4f* skeleton = new m4f[modelToRender->myAnimationData.myIndexedSkeleton.Size()];
	//		m4f lightDir;
	//		lightDir.LookAt(v3f(0, 0, 0), v3f(150, 250, 250));
	//		myRenderer->RenderModelAnimatedToResource(&modelToRender, &finalTarget, *myIcon_RT, skeleton, false, 1, lightDir.GetForwardVector(), false, {128, 128, 128, 255});

	//		myIcon_intermediate->SetAsActiveTarget();
	//		finalTarget->SetAsResourceOnSlot(8);
	//		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::COPY, drawcallCounter);
	//		ID3D11ShaderResourceView* nullSRV = nullptr;
	//		myContext->PSSetShaderResources(8, 1, &nullSRV);
	//		finalTarget->SetAsActiveTarget();
	//		myIcon_intermediate->SetAsResourceOnSlot(8);
	//		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::CLEAR_BLACKPIXELS, drawcallCounter);

	//		delete[] skeleton;
	//	}
	//	break;
	//	case ELoadPackageTypes::AccessoryIcon:
	//	{
	//		AccessoryPackage* accessoryResource = (AccessoryPackage*)aIconResourceToRender;
	//		ModelAnimated* modelToRender = nullptr;
	//		CU::AABB3Df boundingVolume;
	//		modelToRender = EngineInterface::GetModelManager()->GetAnimatedModel(accessoryResource->mesh);
	//		if (modelToRender == nullptr)
	//		{
	//			return;
	//		}
	//		CU::AABB3Df collider = modelToRender->GetCollider();
	//		if (collider.myMin.x < boundingVolume.myMin.x) boundingVolume.myMin.x = collider.myMin.x;
	//		if (collider.myMin.y < boundingVolume.myMin.y) boundingVolume.myMin.y = collider.myMin.y;
	//		if (collider.myMin.z < boundingVolume.myMin.z) boundingVolume.myMin.z = collider.myMin.z;
	//		if (collider.myMax.x > boundingVolume.myMax.x) boundingVolume.myMax.x = collider.myMax.x;
	//		if (collider.myMax.y > boundingVolume.myMax.y) boundingVolume.myMax.y = collider.myMax.y;
	//		if (collider.myMax.z > boundingVolume.myMax.z) boundingVolume.myMax.z = collider.myMax.z;
	//		v3f focusPoint = boundingVolume.GetMiddlePosition();
	//		focusPoint.y += 50.f;
	//		v3f cameraPos = v3f(boundingVolume.myMin.x * 0.5f, boundingVolume.myMax.y + 100, boundingVolume.GetSize().Length() * 0.8f);
	//		cameraPos.Normalize();
	//		cameraPos *= boundingVolume.GetSize().Length() * 0.875f;
	//		cameraPos.y += boundingVolume.myMax.y * 0.55f;
	//		m4f matrix;
	//		matrix.LookAt(focusPoint, cameraPos);
	//		myIcon_RT->camera.GetMatrix() = matrix;
	//		TextureManager* textureManager = EngineInterface::GetTextureManager();
	//		FullScreenTexture* finalTarget = textureManager->GetIconAccessoryResource(accessoryResource->id);
	//		myIcon_RT->texture = &finalTarget;
	//		finalTarget->ClearTexture(CU::Color(0, 0, 0, 0));
	//		myIcon_depth->ClearDepth();
	//		m4f* skeleton = new m4f[modelToRender->myAnimationData.myIndexedSkeleton.Size()];
	//		m4f lightDir;
	//		lightDir.LookAt(v3f(0, 0, 0), v3f(150, 250, 250));
	//		myRenderer->RenderModelAnimatedToResource(&modelToRender, &finalTarget, *myIcon_RT, skeleton, false, 1, lightDir.GetForwardVector(), false);

	//		myIcon_intermediate->SetAsActiveTarget();
	//		finalTarget->SetAsResourceOnSlot(8);
	//		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::COPY, drawcallCounter);
	//		ID3D11ShaderResourceView* nullSRV = nullptr;
	//		myContext->PSSetShaderResources(8, 1, &nullSRV);
	//		finalTarget->SetAsActiveTarget();
	//		myIcon_intermediate->SetAsResourceOnSlot(8);
	//		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::CLEAR_BLACKPIXELS, drawcallCounter);

	//		delete[] skeleton;
	//	}
	//	break;
	//	default:
	//		break;
	//	}


	//}
	void EffectResourceManager::ResizeScreenTextures(v2ui aRenderResolution)
	{
		/// //////////////////////// ----------------- Depth of Field ------------------------ 
		if (myDoF_Near)	{ myDoF_Near->ReleaseResources(); }
		SAFE_DELETE(myDoF_Near);
		if (myDoF_Far) { myDoF_Far->ReleaseResources(); }
		SAFE_DELETE(myDoF_Far);
		if (myDoF_Intermediary) { myDoF_Intermediary->ReleaseResources(); }
		SAFE_DELETE(myDoF_Intermediary);
		if (myDoF_SeperatedColorArray) { myDoF_SeperatedColorArray->ReleaseResources(); }
		SAFE_DELETE(myDoF_SeperatedColorArray);
		if (myDOF_DepthMasks) { myDOF_DepthMasks->ReleaseResources(); }
		SAFE_DELETE(myDOF_DepthMasks);

		myDoF_Near = CreateFullScreenTexture({ aRenderResolution.x / 2, aRenderResolution.y / 2 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true);
		myDoF_Far = CreateFullScreenTexture({ aRenderResolution.x / 2, aRenderResolution.y / 2 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true);
		myDoF_Intermediary = CreateFullScreenTexture({ aRenderResolution.x / 2, aRenderResolution.y / 2 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true);
		//myDoF_Intermediary = CreateFullScreenTextureWithMips({ aRenderResolution.x / 2, aRenderResolution.y / 2 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true);
		myDOF_DepthMasks = CreateFullScreenTexture({ aRenderResolution.x / 2, aRenderResolution.y / 2 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, false, false, true);
		myDoF_SeperatedColorArray = CreateFullScreenTextureArray({ aRenderResolution.x / 2, aRenderResolution.y / 2 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext, 1, 3, false, false, true);
	}
	void EffectResourceManager::RenderDoF(FullScreenTexture* aMainTarget, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter)
	{
		ID3D11ShaderResourceView* NULLSRV = nullptr;
		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		myDoF_Intermediary->SetAsActiveTarget();
		//aIntermediateTarget->SetAsActiveTarget();
		aMainTarget->SetAsResourceOnSlot(8);
		//myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
		aIntermediateTarget->SetAsActiveTarget();
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
		myContext->PSSetShaderResources(8, 1, &NULLSRV);



		//// Render the image to 2 seperate images of depth ranges
		FullScreenTexture* depthSplitTexturesToMap[3];
		depthSplitTexturesToMap[0] = myDoF_Far;
		depthSplitTexturesToMap[1] = myDoF_Near;
		depthSplitTexturesToMap[2] = myDOF_DepthMasks;
		DX::SetMRTAsTarget(myContext, &depthSplitTexturesToMap[0], 3);
		myDoF_Intermediary->SetAsResourceOnSlot(8);
		//aIntermediateTarget->SetAsResourceOnSlot(8);
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::DoF_DEPTHSPLIT, aDrawcallCounter);
		myContext->PSSetShaderResources(8, 1, &NULLSRV);

		/////---------- Far Split and blur----------------------///
		////myContext->PSSetShaderResources()
		myDoF_SeperatedColorArray->SetAsActiveTargets();
		myDoF_Far->SetAsResourceOnSlot(8);
		myDOF_DepthMasks->SetAsResourceOnSlot(11);
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::DoF_COLORSPLIT, aDrawcallCounter);

		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		myDoF_Far->SetAsActiveTarget();
		myDoF_SeperatedColorArray->SetAsResourceOnSlot(8);
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::DoF_BLUR, aDrawcallCounter);

		///---------- Near Split and blur----------------------///
		//myContext->PSSetShaderResources(8, 1, &NULLSRV);
		//myDoF_SeperatedColorArray->SetAsActiveTargets();
		//myDoF_Near->SetAsResourceOnSlot(8);
		//myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::DoF_COLORSPLIT, aDrawcallCounter);

		//myContext->PSSetShaderResources(8, 1, &NULLSRV);
		//myDoF_Near->SetAsActiveTarget();
		//myDoF_SeperatedColorArray->SetAsResourceOnSlot(8);
		//myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::DoF_BLUR, aDrawcallCounter);


		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		aMainTarget->SetAsActiveTarget();
		aIntermediateTarget->SetAsResourceOnSlot(8);
		myDoF_Near->SetAsResourceOnSlot(9);
		myDoF_Far->SetAsResourceOnSlot(10);
		myFullscreenRenderer->Render(FullScreenRenderer::EFullScreenShader::DoF_COMBINE, aDrawcallCounter);
		myContext->PSSetShaderResources(8, 1, &NULLSRV);
		myContext->PSSetShaderResources(9, 1, &NULLSRV);
		myContext->PSSetShaderResources(10, 1, &NULLSRV);
	}
}
