#include "stdafx.h"
#include "ConstantBufferManager.h"
#include "Core\Rendering\DX_Functions\DX_RenderFunctions.h"
#include "RenderData.h"
#include "NoiseGeneration.h"
#include "FullScreenTexture_Factory.h"
#include "FullScreenTexture.h"

namespace Engine
{
	bool ConstantBufferManager::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, CU::Timer* aTimer, unsigned int aNumberOfMips)
	{
		myDevice = aDevice;
		myContext = aContext;
		HRESULT result;
		D3D11_BUFFER_DESC constantBufferDescription = { 0 };
		constantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
		constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		constantBufferDescription.ByteWidth = sizeof(ScreenResBufferData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myScreenResBuffer);
		if (FAILED(result)){ return false; }

		constantBufferDescription.ByteWidth = sizeof(DefinesBufferData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myDefinesBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(GlobalFrameBufferData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myGlobalFrameBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(ObjectBufferData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myObjectBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(ObjectBoneBufferData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myObjectBoneBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(ObjectEffectData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myObjectEffectBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(DecalObjectBufferData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myDecalObjectBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(RenderCameraBuffer);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myRenderCameraBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(NoiseBufferData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myNoiseBuffer);
		if (FAILED(result)) { return false; }

		constantBufferDescription.ByteWidth = sizeof(PostProcessingData);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &myPostProcessingBuffer);
		if (FAILED(result)) { return false; }

		if (!DX::CreateStructuredBuffer(myDevice, &myClusteredPointlightsSRV, &myClusteredPointlightBuffer, MAX_POINT_LIGHTS, sizeof(PointLightRenderCommand))){ return false; }
		if (!DX::CreateStructuredBuffer(myDevice, &myClusteredPointlightsShadowIndexSRV, &myClusteredPointlightShadowIndexBuffer, MAX_POINT_LIGHTS, sizeof(v4ui))){	return false; }

		if (!DX::CreateStructuredBuffer(myDevice, &myClusteredSpotlightsSRV, &myClusteredSpotlightBuffer, MAX_SPOT_LIGHTS, sizeof(SpotLightRenderCommand))){ return false;	}
		if (!DX::CreateStructuredBuffer(myDevice, &myClusteredSpotlightsShadowIndexSRV, &myClusteredSpotlightShadowIndexBuffer, MAX_SPOT_LIGHTS, sizeof(v4ui))){ return false;	}

		if (!DX::CreateStructuredBuffer(myDevice, &myModelOBToWorldSRV, &myModelOBToWorldBuffer, NUMB_MODELSPERTYPE, sizeof(m4f))) { return false; }
		if (!DX::CreateStructuredBuffer(myDevice, &myModelEffectSRV, &myModelEffectBuffer, NUMB_MODELSPERTYPE, sizeof(ObjectEffectData))) { return false; }

		if (!DX::CreateStructuredBuffer(myDevice, &myAnimatedModelOBToWorldSRV, &myAnimatedModelOBToWorldBuffer, NUMB_ANIMMODELSPERTYPE, sizeof(m4f))) { return false; }
		if (!DX::CreateStructuredBuffer(myDevice, &myAnimatedModelEffectSRV, &myAnimatedModelEffectBuffer, NUMB_ANIMMODELSPERTYPE, sizeof(ObjectEffectData))) { return false; }
		if (!DX::CreateStructuredBuffer(myDevice, &myAnimatedModelSkeletonSRV, &myAnimatedModelSkeletonBuffer, NUMB_ANIMMODELSPERTYPE * MAX_BONECOUNT, sizeof(m4f))) { return false; }

		if (!DX::CreateStructuredBuffer(myDevice, &myLightShadowCamerasSRV, &myLightShadowCameras, NUMB_SHADOWMAP_TILETOTAL, sizeof(LightShadowCamera))) { return false; }

		if (!DX::CreateStructuredBuffer(myDevice, &mySHGridSRV, &mySHGrid, 16384 * 8, sizeof(SH))) { return false; }


		//Define Cluster Data
		myDefinesBufferData.clusterDepth = CLUSTER_DEPTH_GPU;
		myDefinesBufferData.clusterHeigth = CLUSTER_HEIGTH_GPU;
		myDefinesBufferData.clusterWidth = CLUSTER_WIDTH_GPU;
		myDefinesBufferData.lightIndexListSize = MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS;

		myDefinesBufferData.shadowMapSize = SHADOWMAP_SIZE;
		myDefinesBufferData.shadowMapTileSize = SHADOWMAP_TILESIZE;
		myDefinesBufferData.numberOfShadowmapTiles = NUMB_SHADOWMAP_TILES;
		myDefinesBufferData.numberOfShadowmapTilesTotal = NUMB_SHADOWMAP_TILETOTAL;
		myDefinesBufferData.NOISE_RotationalTextureSize = NOISE_ROTATIONALTEXTURESIZE;
		myDefinesBufferData.NumberOfMipsReflectionProbe = aNumberOfMips;
		DX::MapUnmapConstantBuffer(myContext, myDefinesBuffer, &myDefinesBufferData, sizeof(DefinesBufferData), 2, 2, 2, 2);

		//---------------Cluster creation data-------------------//
		myClusterLightIndexTexture = CreateFullScreenTexture({ (CLUSTER_DEPTH_GPU) * (CLUSTER_HEIGTH_GPU) *CLUSTER_WIDTH_GPU, 512}, DXGI_FORMAT_R32_UINT, myDevice, myContext, false, false, false, true);
		if (!myClusterData.Init({ CLUSTER_WIDTH_GPU, CLUSTER_HEIGTH_GPU, CLUSTER_DEPTH_GPU }, DXGI_FORMAT_R32G32B32A32_UINT, ETextureUsageFlags::TEXTUREUSAGEFLAG_DEFAULT, true, myDevice, myContext, false, true)) { return false; }
		if (!DX::CreateStructuredUAVBuffer(myDevice, &myClusterBoundingVolumesSRV, &myClusterVolumesUAV,&myClusterBoundingVolumesBuffer, CLUSTER_DEPTH_GPU * CLUSTER_HEIGTH_GPU * CLUSTER_WIDTH_GPU, sizeof(v4f) * 2)) { return false; }

		myClusterBoundsCreation = DX::LoadCS(myDevice, "Content/Shaders/CS_ClusterCreation.cso");
		myClusterFill = DX::LoadCS(myDevice, "Content/Shaders/CS_FillCluster.cso");
		myClusterFillNoHarmonicsOrProbes = DX::LoadCS(myDevice, "Content/Shaders/CS_FillClusterNoHarmonicsOrProbes.cso");

		//---------------Noise Data-------------------//
		NoiseBufferData noise;
		Noise::CreateWeightedUnitHemisphere(NOISE_HALFHEMISPHEREAMOUNT, noise.halfNoiseHemisphere);
		Noise::GeneratePoissonSphereSamples(noise.poissonKernel);
		DX::MapUnmapConstantBuffer(myContext, myNoiseBuffer, &noise, sizeof(NoiseBufferData), 8, 8, 8);

		myRotationKernel = CreateFullScreenTexture({ NOISE_ROTATIONALTEXTURESIZE, NOISE_ROTATIONALTEXTURESIZE }, DXGI_FORMAT_R32G32B32A32_FLOAT, myDevice, myContext, true, true);
		
		D3D11_MAPPED_SUBRESOURCE mappedNoiseTex;
		v4f* noiseData = nullptr;
		result = myContext->Map(myRotationKernel->GetTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedNoiseTex);
		if (FAILED(result)) { assert(false && "Failed to Unmap Noise Texture"); }
		else
		{
			noiseData = (v4f*)mappedNoiseTex.pData;
			Noise::CreateRandomKernelRotations(NOISE_ROTATIONALTEXTURESIZE, noiseData);
		}
		myContext->Unmap(myRotationKernel->GetTexture(), 0);

		myRotationKernel->SetAsResourceOnSlot(33);


		///-------------Create LUT texture for PBR lookup------------------///
		ID3D11Buffer* tempSizeBuffer;
		constantBufferDescription.ByteWidth = sizeof(v4f);
		result = myDevice->CreateBuffer(&constantBufferDescription, nullptr, &tempSizeBuffer);
		if (FAILED(result)) { return false; }


		myScatteringLUT = CreateFullScreenTexture({ 128, 128}, DXGI_FORMAT_R16G16_FLOAT, myDevice, myContext, false, false, false, true);
		myScatteringLUT->SetAsCSOutput(0);
		v4f data;
		data.x = 128; data.y = 128;
		DX::MapUnmapConstantBuffer(myContext, tempSizeBuffer, &data, sizeof(v4f), UINT_MAX, UINT_MAX, UINT_MAX, 0);
		ID3D11ComputeShader* scatterLutCreation = DX::LoadCS(myDevice, "Content/Shaders/CS_ComputeScatteringLUT.cso");
		myContext->CSSetShader(scatterLutCreation, nullptr, 0);
		myContext->Dispatch(myScatteringLUT->GetResolution().x / 8, myScatteringLUT->GetResolution().y / 8, 1);
		
		myContext->CSSetShader(nullptr, nullptr, 0);
		ID3D11UnorderedAccessView* NULLUAV = nullptr;
		myContext->CSSetUnorderedAccessViews(0, 1, &NULLUAV, 0);
		scatterLutCreation->Release();
		tempSizeBuffer->Release();
		myScatteringLUT->SetAsResourceOnSlot(35);

		DX::MapUnmapConstantBuffer(myContext, myPostProcessingBuffer, &myPostProcessingData, sizeof(PostProcessingData), 10, 10, 10, 10);

		return true;
	}

	bool ConstantBufferManager::MapUnMapScreenBuffer(v2f aWindowResolution, v2f aRenderResolution)
	{
		myScreenBufferData.windowResolutionX = aWindowResolution.x;
		myScreenBufferData.windowResolutionY = aWindowResolution.y;
		myScreenBufferData.frameRenderResolutionX = aRenderResolution.x;
		myScreenBufferData.frameRenderResolutionY = aRenderResolution.y;
		return DX::MapUnmapConstantBuffer(myContext, myScreenResBuffer, &myScreenBufferData, sizeof(ScreenResBufferData), 0, 0, 0, 0);
	}

	bool ConstantBufferManager::MapUnMapGlobalFrameBuffer()
	{
		return DX::MapUnmapConstantBuffer(myContext, myGlobalFrameBuffer, &myGlobalFrameBufferData, sizeof(GlobalFrameBufferData), 1, 1, 1, 1);
	}

	void ConstantBufferManager::MapUnmapLightBuffer(RenderData& aRenderBuffer)
	{
		uint16_t numberOfPoints = aRenderBuffer.pointLightsSize;
		uint16_t numberOfSpots = aRenderBuffer.spotLightsSize;
		DX::MapUnmapDynamicBuffer(myContext, myClusteredPointlightsSRV, myClusteredPointlightBuffer, &aRenderBuffer.pointLights[0], sizeof(PointLightRenderCommand), numberOfPoints, 0, false, false, true, true);
		DX::MapUnmapDynamicBuffer(myContext, myClusteredPointlightsShadowIndexSRV, myClusteredPointlightShadowIndexBuffer, &myPointlightShadowIndex[0], sizeof(v4ui), numberOfPoints, 1, false, false, true);
		DX::MapUnmapDynamicBuffer(myContext, myClusteredSpotlightsSRV, myClusteredSpotlightBuffer, &aRenderBuffer.spotLights[0], sizeof(SpotLightRenderCommand), numberOfSpots, 2, false, false, true, true);
		DX::MapUnmapDynamicBuffer(myContext, myClusteredSpotlightsShadowIndexSRV, myClusteredSpotlightShadowIndexBuffer, &mySpotlightShadowIndex[0], sizeof(v4ui), numberOfSpots, 3, false, false, true);
	}

	void ConstantBufferManager::MapUnmapShadowCameraBuffer(LightShadowCamera* someLightCams, size_t aAmountOfCameras)
	{
		DX::MapUnmapDynamicBuffer(myContext, myLightShadowCamerasSRV, myLightShadowCameras, someLightCams, sizeof(LightShadowCamera), (unsigned short)aAmountOfCameras, 31, false, false, true);
	}

	void ConstantBufferManager::MapUnMapObjectBuffer()
	{
		DX::MapUnmapConstantBuffer(myContext, myObjectBuffer, &myObjectBufferData, sizeof(ObjectBufferData), 3, 3, 3);
	}

	void ConstantBufferManager::MapUnMapObjectToBuffers(MeshRenderCommand& aRenderCommand)
	{
		myObjectBufferData.fromOB_toWorld = aRenderCommand.matrix;
		DX::MapUnmapConstantBuffer(myContext, myObjectBuffer, &myObjectBufferData, sizeof(ObjectBufferData), 3, 3, 3);
		DX::MapUnmapConstantBuffer(myContext, myObjectEffectBuffer, &aRenderCommand.effectData, sizeof(ObjectEffectData), 5, 5, 5);
	}

	void ConstantBufferManager::MapUnMapAnimatedObjectToBuffers(AnimatedMeshRenderCommand& aRenderCommand)
	{
		myObjectBufferData.fromOB_toWorld = aRenderCommand.matrix;
		DX::MapUnmapConstantBuffer(myContext, myObjectBuffer, &myObjectBufferData, sizeof(ObjectBufferData), 3, 3, 3);
		DX::MapUnmapConstantBuffer(myContext, myObjectBoneBuffer, &aRenderCommand.boneTransformsFinal[0], sizeof(m4f) * aRenderCommand.numberOfBones, 4, 4, 4);
		DX::MapUnmapConstantBuffer(myContext, myObjectEffectBuffer, &aRenderCommand.effectData, sizeof(ObjectEffectData), 5, 5, 5);
	}

	void ConstantBufferManager::MapUnMapDecalToBuffers(m4f aDecalMatrix)
	{
		myDecalObjectBufferData.fromOB_toWorld = aDecalMatrix;
		aDecalMatrix.SetScale({ 1, 1, 1 });
		myDecalObjectBufferData.fromWorld_toOB = m4f::GetFastInverse(aDecalMatrix);
		DX::MapUnmapConstantBuffer(myContext, myObjectBuffer, &myDecalObjectBufferData, sizeof(DecalObjectBufferData), 6, UINT_MAX, 6);
	}

	void ConstantBufferManager::MapUnMapCameraBuffer(Camera& aCamera)
	{
		const CameraData data = aCamera.GetCameraData();

		memcpy(&myRenderCameraBufferData.renderCam, &data, sizeof(CameraData));
		myRenderCameraBufferData.renderCameraPosition = data.fromCamera.GetTranslationVector();
		DX::MapUnmapConstantBuffer(myContext, myRenderCameraBuffer, &myRenderCameraBufferData, sizeof(RenderCameraBuffer), 7, 7, 7, 7);
	}

	void ConstantBufferManager::MapUnMapEmptyEffectBuffer()
	{
		ObjectEffectData data;
		DX::MapUnmapConstantBuffer(myContext, myObjectEffectBuffer, &data, sizeof(ObjectEffectData), 5, 5, 5);
	}

	void ConstantBufferManager::MapUnMapEffectBuffer()
	{
		DX::MapUnmapConstantBuffer(myContext, myObjectEffectBuffer, &objectEffectBufferData, sizeof(ObjectEffectData), 5, 5, 5);
	}

	void ConstantBufferManager::MapUnmapSHSet(SH* aSHSet, const unsigned int aNumberOfSH)
	{
		DX::MapUnmapDynamicBuffer(myContext, mySHGridSRV, mySHGrid, aSHSet, sizeof(SH), (unsigned short)aNumberOfSH, 34, false, false, true);
		//DX::MapUnmapDynamicBufferWithRange(myContext, mySHGridSRV, mySHGrid, aSHSet, sizeof(SH), (unsigned short)aNumberOfSH, aStartSlot, 34, false, false, true);
	}

	void ConstantBufferManager::MapUnmapPostProcessing()
	{
		DX::MapUnmapConstantBuffer(myContext, myPostProcessingBuffer, &myPostProcessingData, sizeof(PostProcessingData), 10, 10, 10, 10);
	}

	void ConstantBufferManager::MapUnMapStructuredMeshBuffer(SortedModelDataForRendering& aMeshBuffer)
	{
		DX::MapUnmapDynamicBuffer(myContext, myModelOBToWorldSRV, myModelOBToWorldBuffer, &aMeshBuffer.transforms[0], sizeof(m4f), aMeshBuffer.numberOfModels, 4, true, true, true);
		DX::MapUnmapDynamicBuffer(myContext, myModelEffectSRV, myModelEffectBuffer, &aMeshBuffer.effectData[0], sizeof(ObjectEffectData), aMeshBuffer.numberOfModels, 6, true, true, true);
	}

	void ConstantBufferManager::MapUnMapStructuredAnimatedMeshBuffer(SortedAnimationDataForBuffers& aAnimatedMeshBuffer)
	{
		DX::MapUnmapDynamicBuffer(myContext, myAnimatedModelOBToWorldSRV, myAnimatedModelOBToWorldBuffer, &aAnimatedMeshBuffer.transforms[0], sizeof(m4f), aAnimatedMeshBuffer.numberOfModels, 4, true, true, true);
		DX::MapUnmapDynamicBuffer(myContext, myAnimatedModelEffectSRV, myAnimatedModelEffectBuffer, &aAnimatedMeshBuffer.effectData[0], sizeof(ObjectEffectData), aAnimatedMeshBuffer.numberOfModels, 6, true, true, true);
		DX::MapUnmapDynamicBuffer(myContext, myAnimatedModelSkeletonSRV, myAnimatedModelSkeletonBuffer, &aAnimatedMeshBuffer.boneTransforms[0], sizeof(m4f) * MAX_BONECOUNT, aAnimatedMeshBuffer.numberOfModels, 7, true, false, false);
	}
}


