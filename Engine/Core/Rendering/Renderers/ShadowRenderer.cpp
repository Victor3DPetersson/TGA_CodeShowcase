#include "stdafx.h"
#include "ShadowRenderer.h"
#include "GameObjects\Camera.h"
#include <d3d11.h>
#include "GameObjects\Model.h"
#include "GameObjects\ModelData.h"
#include "..\Resources\FullScreenTexture_Factory.h"
#include "..\DX_Functions/DX_RenderFunctions.h"
#include <fstream>
#include "CU/Collision/Intersection.hpp"
#include "..\Resources\LightCamera.h"
#include "..\Resources\ConstantBufferManager.h"
#include "RenderData.h"
#include "..\Resources\RenderFunctions.h"
#include "..\Resources\LerpRenderBuffers.h"
#include "..\Resources\FullScreenTexture.h"
#include "FullScreenRenderer.h"

#include "imgui\imgui.h"

#include "..\Resources\MeshStruct.h"

#include "EngineInterface.h"
#include "Managers\Managers.h"
#include "Managers\ModelManager.h"
#include "Managers\ParticleManager.h"

bool Engine::ShadowRenderer::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ConstantBufferManager* aConstantBufferManager)
{
	myConstantBufferManager = aConstantBufferManager;
	myDevice = aDevice;
	myContext = aDeviceContext;
	myModelManager = EngineInterface::GetModelManager();
	//HRESULT result;
	for (unsigned int c = 0; c < 6; c++)
	{
		switch (c)
		{
		case 0:
			//RIGHT
			myPointsRotationTransform[c].SetRotation({ CU::AngleToRadian(0), CU::AngleToRadian(90), CU::AngleToRadian(0) });
			break;
		case 1:
			//LEFT
			myPointsRotationTransform[c].SetRotation({ CU::AngleToRadian(0), CU::AngleToRadian(-90), CU::AngleToRadian(0) });
			break;
		case 2:
			//UP
			myPointsRotationTransform[c].SetRotation({ CU::AngleToRadian(-90), CU::AngleToRadian(0), CU::AngleToRadian(0) });
			break;
		case 3:
			//DOWN
			myPointsRotationTransform[c].SetRotation({ CU::AngleToRadian(90), CU::AngleToRadian(0), CU::AngleToRadian(0) });
			break;
		case 4:
			//FRONT
			break;
		case 5:
			//BACK
			myPointsRotationTransform[c].SetRotation({ CU::AngleToRadian(0), CU::AngleToRadian(180), CU::AngleToRadian(0) });
			break;
		default:
			break;
		}
	}
	myFinalTexture = CreateFullScreenTexture({ SHADOWMAP_SIZE, SHADOWMAP_SIZE }, DXGI_FORMAT_R32_FLOAT, myDevice, myContext);
	if (myFinalTexture == nullptr) { return false; }
	myAtlasTexture = CreateDepthTexture({ SHADOWMAP_SIZE, SHADOWMAP_STATIC_SIZE }, DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE);
	if (myAtlasTexture == nullptr){	return false; }
	mySpotTexture = CreateDepthTexture({ SHADOWMAP_STATIC_SIZE, SHADOWMAP_STATIC_SIZE }, DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE);
	if (mySpotTexture == nullptr) { return false; }
	myPointTexture = CreateDepthTexture({ SHADOWMAP_STATIC_SIZE, SHADOWMAP_STATIC_SIZE }, DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE);
	if (myPointTexture == nullptr) { return false; }

	myViewPort = new D3D11_VIEWPORT(
	{
		0,
		0,
		(float)SHADOWMAP_SIZE,
		(float)SHADOWMAP_SIZE,
		0,
		1
	});
	for (unsigned int w = 0; w < NUMB_SHADOWMAP_TILES; w++)
	{
		for (unsigned int d = 0; d < NUMB_SHADOWMAP_TILES; d++)
		{
			myUsedIndices[w][d] = false;
		}
	}
	mySortedStaticSpotLights.Init(64);
	mySortedStaticPointLights.Init(64);





	//Camera testCam;
	//testCam.RecalculateProjectionMatrix(90, v2f(512, 512), true, 10.f, 500.f, false);

	//m4f cameraTranslation = m4f(
	//	0, 0, 1.0f, 0,
	//	0, 1.0f, 0, 0,
	//	1.0f, 0, 0, 0,
	//	-560.0005f, 91.65f, 380.0003f, 1.0f);

	//m4f toCamera = m4f::GetInverse(cameraTranslation);

	//v4f worldToLightView = toCamera * v4f(500.f, 500.f, 0, 1);
	//v4f lightViewToLightProj = testCam.GetProjection() * worldToLightView;

	//v2f projectedTexCoord = v2f(((lightViewToLightProj.x / lightViewToLightProj.w) * 0.50f + 0.5f), ((lightViewToLightProj.y / lightViewToLightProj.w * -1.0f) * 0.50f + 0.5f));


	//projectedTexCoord = projectedTexCoord;


	return true;
}


void Engine::ShadowRenderer::RenderShadowMaps(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, FullScreenRenderer& aFullScreenRenderer)
{
	myContext->GSSetShader(nullptr, nullptr, 0);
	myContext->PSSetShader(nullptr, nullptr, 0);
	ID3D11ShaderResourceView* nullSRV[] = { nullptr };
	ID3D11RenderTargetView* nullRenderTarget = { nullptr };
	ID3D11DepthStencilView* nullDepth = { nullptr };
	ID3D11UnorderedAccessView* nullUAV = { nullptr };

	//Rendering rest of the lights
	myContext->PSSetShaderResources(27, 1, nullSRV);
	myContext->PSSetShaderResources(28, 1, nullSRV);
	m4f& cameraTransform = myShadowCamera.GetMatrix();
	cameraTransform = m4f();
	RenderStaticLights(aRenderBuffer, meshesToFill, aDrawcallCounter, cameraTransform);


	//myContext->ClearDepthStencilView(myAtlasDepth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, (UINT8)1.0f, (UINT8)0);
	myAtlasTexture->SetAsActiveDepth();
	//myContext->OMSetRenderTargets(0, &nullRenderTarget, myAtlasDepth);
	unsigned short lastTakenHorizontalIndex = SHADOWMAPDIRECTIONAL_SIZE / SHADOWMAP_TILESIZE;
	unsigned short lastTakenVerticalIndex = 0;

	unsigned short lastSkippedVerticalIndex = 0;
	unsigned short lastSkippedHorizontalIndex = SHADOWMAPDIRECTIONAL_SIZE / SHADOWMAP_TILESIZE;
	const unsigned int tileAmountSpot_T1 = (SHADOWMAP_TILESIZE * 16) / SHADOWMAP_TILESIZE;
	const unsigned int tileAmountSpot_T2 = (SHADOWMAP_TILESIZE * 8) / SHADOWMAP_TILESIZE;
	const unsigned int tileAmountSpot_T3 = (SHADOWMAP_TILESIZE * 4) / SHADOWMAP_TILESIZE;

	const unsigned int tileAmountPoint_T1 = (SHADOWMAP_TILESIZE * 8) / SHADOWMAP_TILESIZE;
	const unsigned int tileAmountPoint_T2 = (SHADOWMAP_TILESIZE * 4) / SHADOWMAP_TILESIZE;
	const unsigned int tileAmountPoint_T3 = (SHADOWMAP_TILESIZE) / SHADOWMAP_TILESIZE;

	unsigned short shadowLightCount = 1; 

	cameraTransform = m4f();

	bool hasJumped = false;
	unsigned short currentLightThreshold = 0;
	unsigned short previousLightThreshold = 0;
	while (mySortedShadowLights.Size() > 0)
	{
		SortedLight light = mySortedShadowLights.Dequeue();
		//Check distance to light and set its tileAmount to proper value
		if (light.distance < LIGHTRANGE_THRESHOLD1)
		{
			light.tileAmount = (light.isSpotLight) ? tileAmountSpot_T1 : tileAmountPoint_T1;
		}
		else if (light.distance < LIGHTRANGE_THRESHOLD2)
		{
			light.tileAmount = (light.isSpotLight) ? tileAmountSpot_T2 : tileAmountPoint_T2;
			currentLightThreshold = 1;
		}
		else if (light.distance < LIGHTRANGE_THRESHOLD3)
		{
			light.tileAmount = (light.isSpotLight) ? tileAmountSpot_T3 : tileAmountPoint_T3;
			currentLightThreshold = 2;
		}
		else
		{
			light.tileAmount = tileAmountPoint_T3;
			currentLightThreshold = 3;
		}

		if (currentLightThreshold != previousLightThreshold)
		{
			RenderSpotLights(aRenderBuffer, meshesToFill, aDrawcallCounter, shadowLightCount, cameraTransform, lastTakenHorizontalIndex, lastTakenVerticalIndex, lastSkippedHorizontalIndex, lastSkippedVerticalIndex, hasJumped);
			RenderPointLights(aRenderBuffer, meshesToFill, aDrawcallCounter, shadowLightCount, cameraTransform, lastTakenHorizontalIndex, lastTakenVerticalIndex, lastSkippedHorizontalIndex, lastSkippedVerticalIndex, hasJumped);
		}
		if (light.isSpotLight)
		{
			mySpotLightsUntilRangeThreshold.Add(light);
		}
		else
		{
			myPointLightsUntilRangeThreshold.Add(light);
		}
		previousLightThreshold = currentLightThreshold;
	}
	RenderSpotLights(aRenderBuffer, meshesToFill, aDrawcallCounter, shadowLightCount, cameraTransform, lastTakenHorizontalIndex, lastTakenVerticalIndex, lastSkippedHorizontalIndex, lastSkippedVerticalIndex, hasJumped);
	RenderPointLights(aRenderBuffer, meshesToFill, aDrawcallCounter, shadowLightCount, cameraTransform, lastTakenHorizontalIndex, lastTakenVerticalIndex, lastSkippedHorizontalIndex, lastSkippedVerticalIndex, hasJumped);
	
	//Mapping shadow cameras and Light buffer to GPU
	myConstantBufferManager->MapUnmapLightBuffer(*aRenderBuffer);
	myConstantBufferManager->MapUnmapShadowCameraBuffer(myShadowCameras, (size_t)(shadowLightCount + myAmountOfRenderedStaticSpots + (myAmountOfRenderedStaticPoints * 6)));


	//Copying over data to atlas
	myContext->OMSetRenderTargets(0, &nullRenderTarget, nullDepth);
	myFinalTexture->SetAsActiveTarget();

	myViewPort->Width = (float)(SHADOWMAP_SIZE);
	myViewPort->Height = (float)(SHADOWMAP_STATIC_SIZE);

	myAtlasTexture->SetAsResourceOnSlot(8);
	myViewPort->TopLeftX = (float)(0);
	myViewPort->TopLeftY = (float)(0);
	myContext->RSSetViewports(1, myViewPort);
	aFullScreenRenderer.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);

	myViewPort->Width = (float)(SHADOWMAP_STATIC_SIZE);
	if (myAmountOfRenderedStaticSpots > 0)
	{
		mySpotTexture->SetAsResourceOnSlot(8);
		myViewPort->TopLeftX = (float)(0);
		myViewPort->TopLeftY = (float)(SHADOWMAP_STATIC_SIZE);
		myContext->RSSetViewports(1, myViewPort);
		aFullScreenRenderer.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
	}
	if (myAmountOfRenderedStaticPoints > 0)
	{
		myPointTexture->SetAsResourceOnSlot(8);
		myViewPort->TopLeftX = (float)(SHADOWMAP_STATIC_SIZE);
		myViewPort->TopLeftY = (float)(SHADOWMAP_STATIC_SIZE);
		myContext->RSSetViewports(1, myViewPort);
		aFullScreenRenderer.Render(FullScreenRenderer::EFullScreenShader::COPY, aDrawcallCounter);
	}

	myContext->OMSetRenderTargets(0, &nullRenderTarget, nullDepth);
	myFinalTexture->SetAsResourceOnSlot(28);
	//myContext->PSSetShaderResources(28, 1, &myAtlasSRV);
}

void Engine::ShadowRenderer::RenderDirectionalShadowMap(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, Camera& aMainCamera, std::atomic<unsigned int>& aDrawcallCounter)
{
	myContext->GSSetShader(nullptr, nullptr, 0);
	myContext->PSSetShader(nullptr, nullptr, 0);
	ID3D11ShaderResourceView* nullSRV[] = { nullptr };
	ID3D11RenderTargetView* nullRenderTarget = { nullptr };
	ID3D11DepthStencilView* nullDepth = { nullptr };
	myAtlasTexture->ClearDepth();
	myFinalTexture->ClearTexture();
	myAtlasTexture->SetAsActiveDepth();

	//Fixing DirectionalLightPosition
	//CalculateDirectionalLightCamera(directionalLightCamera, directionalDir, aRenderCamera, myDirectionalLightCamera);
	myShadowCamera.RecalculateProjectionMatrix(90, v2f(SHADOWMAPDIRECTIONAL_SIZE * 2.5f, SHADOWMAPDIRECTIONAL_SIZE * 2.5f), false, 100.0f, 20000.0f, false);
	//RecalculateProjectionMatrix(90, v2f(SHADOWMAPDIRECTIONAL_SIZE * 4, SHADOWMAPDIRECTIONAL_SIZE * 4), false, 100.0f, 30000.0f, myShadowCamera.GetProjection());
	v3f directionalDir = v3f(aRenderBuffer->dirLight.myDirection.x, aRenderBuffer->dirLight.myDirection.y, aRenderBuffer->dirLight.myDirection.z);
	v3f lightPosition = (directionalDir) * 5000.0f;
	LookAt(v3f(), lightPosition, myShadowCamera.GetMatrix());

	myViewPort->TopLeftX = (float)0;
	myViewPort->TopLeftY = (float)0;
	myViewPort->Width = (float)(SHADOWMAPDIRECTIONAL_SIZE);
	myViewPort->Height = (float)(SHADOWMAPDIRECTIONAL_SIZE);
	myContext->RSSetViewports(1, myViewPort);
	//Set Shadow Camera
	LightShadowCamera& lightCamera = myShadowCameras[0];
	lightCamera.projection = myShadowCamera.GetProjection();
	lightCamera.transform = m4f::GetFastInverse(myShadowCamera.GetMatrix());
	for (unsigned int w = 0; w < SHADOWMAPDIRECTIONAL_SIZE / SHADOWMAP_TILESIZE; w++)
	{
		for (unsigned int d = 0; d < SHADOWMAPDIRECTIONAL_SIZE / SHADOWMAP_TILESIZE; d++)
		{
			myUsedIndices[w][d] = true;
		}
	}
	myConstantBufferManager->MapUnMapCameraBuffer(myShadowCamera);
	//Sorting meshes
	ClearMeshes(meshesToFill, aRenderBuffer->sortedMeshesSize, aRenderBuffer->sortedAnimMeshesSize);
	std::atomic<unsigned int> jobCounter = 4;
	SortModelsAndCalculateMVPs(aRenderBuffer, meshesToFill, myShadowCamera, jobCounter, myModelManager);

	//Rendering Directional light
	//Clear Resource for write
	//myContext->PSSetShaderResources(32, 1, nullSRV);
	//Clearing map
	//myContext->ClearDepthStencilView(myDirectionalDepth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, (UINT8)1.0f, (UINT8)0);
	//Settig map as Target
	//myContext->OMSetRenderTargets(0, &nullRenderTarget, myDirectionalDepth);
	//Render
	RenderModels(meshesToFill, aDrawcallCounter, false);
	//myContext->OMSetRenderTargets(0, &nullRenderTarget, nullDepth);
	//myContext->PSSetShaderResources(32, 1, &myDirectionalSRV);
	//myConstantBufferManager->MapUnmapShadowCameraBuffer(myShadowCameras, 1);
}

void Engine::ShadowRenderer::SortLights(RenderData* aRenderBuffer, Camera& aMainCamera, bool onlyMainCamera)
{
	//Clearing occupied tiles
	for (unsigned int w = 0; w < NUMB_SHADOWMAP_TILES; w++)
	{
		for (unsigned int d = 0; d < NUMB_SHADOWMAP_TILES; d++)
		{
			myUsedIndices[w][d] = false;
		}
	}
	//Clear Data
	mySortedShadowLights.Clear();
	mySortedStaticSpotLights.RemoveAll();
	mySortedStaticPointLights.RemoveAll();
	memset(&myConstantBufferManager->mySpotlightShadowIndex[0], 0, sizeof(v4ui) * MAX_SPOT_LIGHTS);
	memset(&myConstantBufferManager->myPointlightShadowIndex[0], 0, sizeof(v4ui) * MAX_POINT_LIGHTS);

	//Fetch all render Cameras for culling
	myToCameras[0] = m4f::GetFastInverse(aMainCamera.GetMatrix());
	myRenderTargetCameras[0] = &aMainCamera;
	myNumberOfRenderTargets = 1;
	if (onlyMainCamera == false)
	{
		for (size_t i = 0; i < NUMBOF_RENDERTARGETS; i++)
		{
			if (aRenderBuffer->renderTargetcameraFlags[PO2(i)])
			{
				myRenderTargetCameras[myNumberOfRenderTargets] = &aRenderBuffer->renderTargetCameras[i].camera;
				myToCameras[myNumberOfRenderTargets++] = m4f::GetFastInverse(aRenderBuffer->renderTargetCameras[i].camera.GetMatrix());
			}
		}
	}
	//Data preAllocated on cache for easy access
	size_t numberOfCamerasAffected = 0;
	v3f cameraPosition[NUMBOF_RENDERTARGETS + 1];
	m3f rotationMatrix;
	const unsigned short spotLightSize = aRenderBuffer->spotLightsSize;
	const unsigned short pointLightSize = aRenderBuffer->pointLightsSize;
	//Sort lights
	for (unsigned short light = 0; light < spotLightSize + pointLightSize; light++)
	{
		// SPOTS
		if (light < spotLightSize && aRenderBuffer->spotlightsShadowCaster[light] != 0)
		{
			numberOfCamerasAffected = 0;
			if (aRenderBuffer->spotlightsShadowCaster[light] == 1)
			{
				// Check if light is inside any RenderTarget Frustum
				SpotLightRenderCommand& spot = aRenderBuffer->spotLights[light];
				const v4f lightPosition = v4f(spot.position.x, spot.position.y, spot.position.z, 1);
				const v4f spotDirection = v4f(spot.direction.x, spot.direction.y, spot.direction.z, 0);
				bool isInsideAFrustum = false;
				for (size_t camera = 0; camera < myNumberOfRenderTargets; camera++)
				{
					const v4f lightPosInCameraSpace = lightPosition * myToCameras[camera];
					const v4f lightDirInCameraSpace = spotDirection * myToCameras[camera];
					const v3f multipliedPos = v3f(lightPosInCameraSpace.x, lightPosInCameraSpace.y, lightPosInCameraSpace.z);
					const v3f multipliedDir = v3f(lightDirInCameraSpace.x, lightDirInCameraSpace.y, lightDirInCameraSpace.z);
					bool insideACluster = false;
					for (unsigned short depth = 0; depth < CLUSTER_DEPTH; depth++)
					{
						if (TestConeVsSphere(multipliedPos, multipliedDir, spot.range, spot.angle, myRenderTargetCameras[camera]->GetClusterDepth()[depth].GetSphereFromVolume()))
						{
						/*	for (uint32_t cluster = 0; cluster < CLUSTER_WIDTH * CLUSTER_HEIGTH; cluster++)
							{
								if (TestConeVsSphere(multipliedPos, multipliedDir, spot.range, spot.angle, myRenderTargetCameras[camera]->GetClusterWidthHeigth(depth)[cluster].GetSphereFromVolume()))
								{*/
							cameraPosition[numberOfCamerasAffected++] = myRenderTargetCameras[camera]->GetMatrix().GetTranslationVector();
							isInsideAFrustum = true;
							insideACluster = true;
							break;

						}
					}
				}
				if (isInsideAFrustum == false)
				{
					continue;
				}
				//Find Closest distance from light to Camera
				float minDistance = FLT_MAX;
				for (size_t camera = 0; camera < numberOfCamerasAffected; camera++)
				{
					minDistance = CU::Min((cameraPosition[camera] - spot.position).Dot(cameraPosition[camera] - spot.position), minDistance);
				}

				SortedLight sortedLight;
				sortedLight.distance = minDistance;
				sortedLight.lightIndex = light;
				sortedLight.isSpotLight = true;
				mySortedShadowLights.Enqueue(sortedLight);
				continue;
			}
			if (aRenderBuffer->spotlightsShadowCaster[light] == 2)
			{
				SortedLight sortedLight;
				sortedLight.distance = 0;
				sortedLight.lightIndex = light;
				sortedLight.isSpotLight = true;
				mySortedStaticSpotLights.Add(sortedLight);
			}

			
		}
		// POINTS
		else if (light >= spotLightSize && aRenderBuffer->pointlightShadowCaster[light - spotLightSize] != 0)
		{
			if (aRenderBuffer->pointlightShadowCaster[light - spotLightSize] == 1)
			{
				numberOfCamerasAffected = 0;
				// Check if light is inside any RenderTarget Frustum
				PointLightRenderCommand& point = aRenderBuffer->pointLights[light - spotLightSize];
				const v4f lightPosition = v4f(point.position.x, point.position.y, point.position.z, 1);
				bool isInsideAFrustum = false;
				for (size_t camera = 0; camera < myNumberOfRenderTargets; camera++)
				{
					const v4f lightPosInCameraSpace = lightPosition * myToCameras[camera];
					const v3f multipliedPos = v3f(lightPosInCameraSpace.x, lightPosInCameraSpace.y, lightPosInCameraSpace.z);
					CU::AABB3Df col = myRenderTargetCameras[camera]->GetClusterBounds();
					if (testSphereAABB(point.range, col, multipliedPos))
					{
						cameraPosition[numberOfCamerasAffected++] = myRenderTargetCameras[camera]->GetMatrix().GetTranslationVector();
						isInsideAFrustum = true;
					}
				}
				if (isInsideAFrustum == false)
				{
					continue;
				}

				//Find Closest distance from light to Camera
				float minDistance = FLT_MAX;
				for (size_t camera = 0; camera < numberOfCamerasAffected; camera++)
				{
					minDistance = CU::Min((cameraPosition[camera] - point.position).Dot(cameraPosition[camera] - point.position), minDistance);
				}

				//Add light to sorted list
				SortedLight sortedLight;
				sortedLight.distance = minDistance;
				sortedLight.lightIndex = light - spotLightSize;
				mySortedShadowLights.Enqueue(sortedLight);
				continue;
			}
			if (aRenderBuffer->pointlightShadowCaster[light - spotLightSize] == 2)
			{
				SortedLight sortedLight;
				sortedLight.distance = 0;
				sortedLight.lightIndex = light - spotLightSize;
				sortedLight.isSpotLight = false;
				mySortedStaticPointLights.Add(sortedLight);
			}
		}
	}
	SortStaticLights(aRenderBuffer);
}

void Engine::ShadowRenderer::SortForSpecificCamera(RenderData* aRenderBuffer, Camera& aMainCamera)
{

	SortLights(aRenderBuffer, aMainCamera, true);
}

void Engine::ShadowRenderer::SortStaticLights(RenderData* aRenderBuffer)
{
	const unsigned short spotLightSize = aRenderBuffer->spotLightsSize;
	const unsigned short pointLightSize = aRenderBuffer->pointLightsSize;

	const unsigned short tileAmountSpot_T1 = (unsigned short)(SHADOWMAP_TILESIZE * 8) / SHADOWMAP_TILESIZE;
	const unsigned short tileAmountSpot_T2 = (unsigned short)(SHADOWMAP_TILESIZE * 4) / SHADOWMAP_TILESIZE;
	const unsigned short tileAmountSpot_T3 = (unsigned short)(SHADOWMAP_TILESIZE * 2) / SHADOWMAP_TILESIZE;

	const unsigned short tileAmountPoint_T1 = (unsigned short)(SHADOWMAP_TILESIZE * 4) / SHADOWMAP_TILESIZE;
	const unsigned short tileAmountPoint_T2 = (unsigned short)(SHADOWMAP_TILESIZE * 2) / SHADOWMAP_TILESIZE;
	const unsigned short tileAmountPoint_T3 = (unsigned short)(SHADOWMAP_TILESIZE) / SHADOWMAP_TILESIZE;

	if (mySortedStaticSpotLights.Size() != myAmountOfRenderedStaticSpots)
	{
		myWillReRenderStaticSpots = true;
		myAmountOfRenderedStaticSpots = mySortedStaticSpotLights.Size();
		if ((NUMB_STATICSHADOWMAP_TILES / tileAmountSpot_T1) * (NUMB_STATICSHADOWMAP_TILES / tileAmountSpot_T1) >= myAmountOfRenderedStaticSpots)
		{
			myStaticSpotTileSize = tileAmountSpot_T1;
		}
		else if ((NUMB_STATICSHADOWMAP_TILES / tileAmountSpot_T2) * (NUMB_STATICSHADOWMAP_TILES / tileAmountSpot_T2) >= myAmountOfRenderedStaticSpots)
		{
			myStaticSpotTileSize = tileAmountSpot_T2;
		}
		else if ((NUMB_STATICSHADOWMAP_TILES / tileAmountSpot_T3) * (NUMB_STATICSHADOWMAP_TILES / tileAmountSpot_T3) >= myAmountOfRenderedStaticSpots)
		{
			myStaticSpotTileSize = tileAmountSpot_T3;
		}
		else
		{
			myStaticSpotTileSize = tileAmountPoint_T3;
		}
	}
	if (mySortedStaticPointLights.Size() != myAmountOfRenderedStaticPoints)
	{
		myAmountOfRenderedStaticPoints = mySortedStaticPointLights.Size();
		myWillReRenderStaticPoints = true;
		unsigned short tileAmountPoint = tileAmountPoint_T1;
		bool foundPointTileSize = false;
		while (foundPointTileSize == false)
		{
			unsigned short numberOfTilesTotalHorizontal = (unsigned short)NUMB_STATICSHADOWMAP_TILES / (tileAmountPoint * 6);
			if ( myAmountOfRenderedStaticPoints <= (unsigned short)NUMB_STATICSHADOWMAP_TILES / tileAmountPoint)
			{
				myStaticPointTileSize = tileAmountPoint;
				foundPointTileSize = true;
			}
			else
			{
				tileAmountPoint--;
				if (tileAmountPoint == 1)
				{
					myStaticPointTileSize = 1;
					foundPointTileSize = true;
				}
			}
		}
	}
	unsigned short spotH = 0;
	unsigned short spotV = 0;
	for (unsigned short spot = 0; spot < myAmountOfRenderedStaticSpots; spot++)
	{
		const SortedLight sortedSpot = mySortedStaticSpotLights[spot];

		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].x = (unsigned int)spotH;
		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].y = (unsigned int)NUMB_STATICSHADOWMAP_TILES + spotV;
		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].z = (unsigned int)myStaticSpotTileSize * SHADOWMAP_TILESIZE;
		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].w = (unsigned int)spot;//Camera position for spots is from [0 - staticSpotCount]

		spotH += myStaticSpotTileSize;
		if (spotH >= NUMB_STATICSHADOWMAP_TILES)
		{
			spotH = 0;
			spotV += myStaticSpotTileSize;
		}
		if (spotV == NUMB_STATICSHADOWMAP_TILES)
		{
			break;
		}
	}
	unsigned short pointH = 0;
	unsigned short pointV = 0;
	for (unsigned short point = 0; point < myAmountOfRenderedStaticPoints; point++)
	{
		const SortedLight sortedPoint = mySortedStaticPointLights[point];
		if (pointH + (myStaticPointTileSize * 6) >= NUMB_STATICSHADOWMAP_TILES)
		{
			pointH = 0;
 			pointV += myStaticPointTileSize;
		}
 		if (pointV == NUMB_STATICSHADOWMAP_TILES)
		{
			break;
		}
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].x = (unsigned int)NUMB_STATICSHADOWMAP_TILES + pointH;
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].y = (unsigned int)NUMB_STATICSHADOWMAP_TILES + pointV;
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].z = (unsigned int)myStaticPointTileSize * SHADOWMAP_TILESIZE;
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].w = (unsigned int)myAmountOfRenderedStaticSpots + point * 6;//Camera position for spots is from [staticSpotCount - staticPointCount]
		pointH += myStaticPointTileSize * 6;
	}
}

void Engine::ShadowRenderer::RenderStaticLights(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, m4f& aCameraTransform)
{
	if (myWillReRenderStaticSpots)
	{
		std::atomic<unsigned int> jobCounter = 0;
		ID3D11RenderTargetView* nullRenderTarget = { nullptr };
		mySpotTexture->ClearDepth();
		mySpotTexture->SetAsActiveDepth();
		//myContext->ClearDepthStencilView(myStaticSpotDepth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, (UINT8)1.0f, (UINT8)0);
		//myContext->OMSetRenderTargets(0, &nullRenderTarget, myStaticSpotDepth);
		unsigned short spotH = 0;
		unsigned short spotV = 0;
		for (unsigned short spot = 0; spot < myAmountOfRenderedStaticSpots; spot++)
		{
			const SortedLight sortedSpot = mySortedStaticSpotLights[spot];
			myViewPort->TopLeftX = (float)(SHADOWMAP_TILESIZE * spotH);
			myViewPort->TopLeftY = (float)(SHADOWMAP_TILESIZE * spotV);
			myViewPort->Width = (float)(SHADOWMAP_TILESIZE * myStaticSpotTileSize);
			myViewPort->Height = (float)(SHADOWMAP_TILESIZE * myStaticSpotTileSize);
			myContext->RSSetViewports(1, myViewPort);
			SpotLightRenderCommand& spotL = aRenderBuffer->spotLights[sortedSpot.lightIndex];
			//Set Shadow Camera
			LightShadowCamera& lightCamera = myShadowCameras[spot + 1];
			//memset(&lightCamera, 0, sizeof(LightShadowCamera));
			myShadowCamera.RecalculateProjectionMatrix(CU::RadianToAngle(spotL.angle), v2f(myViewPort->Width, myViewPort->Height), true, 1.0f, spotL.range * 3.5f, false);
			aCameraTransform.MakeRotationDir(spotL.direction);
			aCameraTransform.SetTranslation(spotL.position);
			lightCamera.projection = myShadowCamera.GetProjection();
			lightCamera.transform = m4f::GetFastInverse(aCameraTransform);

			myConstantBufferManager->MapUnMapCameraBuffer(myShadowCamera);
			//RenderModels
			ClearMeshes(meshesToFill, aRenderBuffer->sortedMeshesSize, aRenderBuffer->sortedAnimMeshesSize);
			jobCounter = 4;
			SortModelsAndCalculateMVPs(aRenderBuffer, meshesToFill, myShadowCamera, jobCounter, myModelManager);
			RenderModels(meshesToFill, aDrawcallCounter, true);
			spotH += myStaticSpotTileSize;
			if (spotH >= NUMB_STATICSHADOWMAP_TILES)
			{
				spotH = 0;
				spotV += myStaticSpotTileSize;
			}
			if (spotV == NUMB_STATICSHADOWMAP_TILES)
			{
				break;
			}
		}
		myWillReRenderStaticSpots = false;
	}
	if (myWillReRenderStaticPoints)
	{
		std::atomic<unsigned int> jobCounter = 0;
		///ID3D11RenderTargetView* nullRenderTarget = {nullptr};
		myPointTexture->ClearDepth();
		myPointTexture->SetAsActiveDepth();
		unsigned short pointH = 0;
		unsigned short pointV = 0;
		for (unsigned short point = 0; point < myAmountOfRenderedStaticPoints; point++)
		{
			if (pointH + (myStaticPointTileSize * 6) >= NUMB_STATICSHADOWMAP_TILES)
			{
				pointH = 0;
				pointV += myStaticPointTileSize;
			}
			if (pointV == NUMB_STATICSHADOWMAP_TILES)
			{
				break;
			}
			const SortedLight sortedPoint = mySortedStaticPointLights[point];

			PointLightRenderCommand& pointL = aRenderBuffer->pointLights[sortedPoint.lightIndex];

			myViewPort->Width = (float)(SHADOWMAP_TILESIZE * myStaticPointTileSize);
			myViewPort->Height = (float)(SHADOWMAP_TILESIZE * myStaticPointTileSize);

			myShadowCamera.RecalculateProjectionMatrix(90, { myViewPort->Width, myViewPort->Height }, true, 1.0f, pointL.range * 10.5f, false);

			aCameraTransform.SetTranslation(pointL.position);

			for (unsigned short direction = 0; direction < 6; direction++)
			{
				myViewPort->TopLeftX = (float)(SHADOWMAP_TILESIZE * pointH);
				myViewPort->TopLeftY = (float)(SHADOWMAP_TILESIZE * pointV);

				myContext->RSSetViewports(1, myViewPort);

				pointH += myStaticPointTileSize;
				//Set Shadow Camer
				int cameraIndex = (int)(myAmountOfRenderedStaticSpots + (point * 6) + direction);
				LightShadowCamera& lightCamera = myShadowCameras[cameraIndex + 1];
				myShadowCamera.GetMatrix().SetRotationIgnoreScale(myPointsRotationTransform[direction]);
				lightCamera.projection = myShadowCamera.GetProjection();
				lightCamera.transform = m4f::GetFastInverse(aCameraTransform);

				myConstantBufferManager->MapUnMapCameraBuffer(myShadowCamera);
				//RenderModels
				ClearMeshes(meshesToFill, aRenderBuffer->sortedMeshesSize, aRenderBuffer->sortedAnimMeshesSize);
				jobCounter = 4;
				SortModelsAndCalculateMVPs(aRenderBuffer, meshesToFill, myShadowCamera, jobCounter, myModelManager);
				RenderModels(meshesToFill, aDrawcallCounter, true);
			}
		}
		myWillReRenderStaticPoints = false;
	}
}

void Engine::ShadowRenderer::RenderSpotLights(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, unsigned short& aShadowLightCount, m4f& aCameraTransform, unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped)
{
	std::atomic<unsigned int> jobCounter = 0;
	while (mySpotLightsUntilRangeThreshold.Size() > 0)
	{
		if (aLastSkippedIndex_V >= NUMB_SHADOWMAP_TILES / 2)
		{
			mySpotLightsUntilRangeThreshold.Clear();
			break;
		}
		const SortedLight sortedSpot = mySpotLightsUntilRangeThreshold[0];
		mySpotLightsUntilRangeThreshold.RemoveCyclicAtIndex(0);
		myViewPort->TopLeftX = (float)(SHADOWMAP_TILESIZE * aLastSkippedIndex_H);
		myViewPort->TopLeftY = (float)(SHADOWMAP_TILESIZE * aLastSkippedIndex_V);
		myViewPort->Width = (float)(SHADOWMAP_TILESIZE * sortedSpot.tileAmount);
		myViewPort->Height = (float)(SHADOWMAP_TILESIZE * sortedSpot.tileAmount);
		myContext->RSSetViewports(1, myViewPort);

		SpotLightRenderCommand& spot = aRenderBuffer->spotLights[sortedSpot.lightIndex];

		unsigned short cameraIndex = aShadowLightCount + myAmountOfRenderedStaticSpots + (myAmountOfRenderedStaticPoints * 6);
		//Set LightData
		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].x = (unsigned int)aLastSkippedIndex_H;
		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].y = (unsigned int)aLastSkippedIndex_V;
		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].z = (unsigned int)sortedSpot.tileAmount * SHADOWMAP_TILESIZE;
		myConstantBufferManager->mySpotlightShadowIndex[sortedSpot.lightIndex].w = (unsigned int)cameraIndex;

		//Set Shadow Camera
		LightShadowCamera& lightCamera = myShadowCameras[cameraIndex];
		aShadowLightCount++;
		//memset(&lightCamera, 0, sizeof(LightShadowCamera));
		myShadowCamera.RecalculateProjectionMatrix(CU::RadianToAngle(spot.angle), v2f(myViewPort->Width, myViewPort->Height), true, 1.0f, spot.range * 1.5f, false);
		aCameraTransform.MakeRotationDir(spot.direction);
		aCameraTransform.SetTranslation(spot.position);

		lightCamera.projection = myShadowCamera.GetProjection();
		lightCamera.transform = m4f::GetFastInverse(aCameraTransform);

		myConstantBufferManager->MapUnMapCameraBuffer(myShadowCamera);
		//RenderModels
		ClearMeshes(meshesToFill, aRenderBuffer->sortedMeshesSize, aRenderBuffer->sortedAnimMeshesSize);
		jobCounter = 4;
		SortModelsAndCalculateMVPs(aRenderBuffer, meshesToFill, myShadowCamera, jobCounter, myModelManager);
		RenderModels(meshesToFill, aDrawcallCounter, false);
		//filling indexed bool map of all tiles
		for (unsigned short w = 0; w < sortedSpot.tileAmount; w++)
		{
			for (unsigned short h = 0; h < sortedSpot.tileAmount; h++)
			{
				myUsedIndices[w + aLastSkippedIndex_H][h + aLastSkippedIndex_V] = true;
			}
		}
		//Atlas Tile Logic
		if (aLastSkippedIndex_V < aLastTakenIndex_V)
		{
			SpotLightShadowAtlasCatchUpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, sortedSpot.tileAmount);
		}
		else
		{
			SpotLightShadowAtlasJumpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, aHasJumped, sortedSpot.tileAmount);
		}
	}
}

void Engine::ShadowRenderer::RenderPointLights(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, unsigned short& aShadowLightCount, m4f& aCameraTransform, unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped)
{
	std::atomic<unsigned int> jobCounter = 0;
	while (myPointLightsUntilRangeThreshold.Size() > 0)
	{
		if (aLastTakenIndex_V >= NUMB_SHADOWMAP_TILES / 2)
		{
			myPointLightsUntilRangeThreshold.Clear();
			break;
		}
		SortedLight sortedPoint = myPointLightsUntilRangeThreshold[0];
		myPointLightsUntilRangeThreshold.RemoveCyclicAtIndex(0);
		//assert(sortedPoint.tileAmount < tileAmountSpot_T1 && "Magic tile amount makes me sad");
		if (aLastTakenIndex_H + (sortedPoint.tileAmount * 6) >= NUMB_SHADOWMAP_TILES)
		{
			PointLightShadowAtlasJumpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, aHasJumped, sortedPoint.tileAmount);
		}
		for (unsigned short w = 0; w < sortedPoint.tileAmount * 6; w++)
		{
			for (unsigned short h = 0; h < sortedPoint.tileAmount; h++)
			{
				myUsedIndices[w + aLastTakenIndex_H][h + aLastTakenIndex_V] = true;
			}
		}

		unsigned short cameraIndex = aShadowLightCount + myAmountOfRenderedStaticSpots + (myAmountOfRenderedStaticPoints * 6);
		//Set LightData
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].x = (unsigned int)aLastTakenIndex_H;
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].y = (unsigned int)aLastTakenIndex_V;
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].z = (unsigned int)sortedPoint.tileAmount * SHADOWMAP_TILESIZE;
		myConstantBufferManager->myPointlightShadowIndex[sortedPoint.lightIndex].w = (unsigned int)cameraIndex;

		PointLightRenderCommand& point = aRenderBuffer->pointLights[sortedPoint.lightIndex];

		myViewPort->Width = (float)(SHADOWMAP_TILESIZE * sortedPoint.tileAmount);
		myViewPort->Height = (float)(SHADOWMAP_TILESIZE * sortedPoint.tileAmount);

		myShadowCamera.RecalculateProjectionMatrix(92.f, { myViewPort->Width, myViewPort->Height }, true, 1.0f, point.range * 2.5f, false);

		aCameraTransform.SetTranslation(point.position);

		for (unsigned short direction = 0; direction < 6; direction++)
		{
			myViewPort->TopLeftX = (float)(SHADOWMAP_TILESIZE * aLastTakenIndex_H);
			myViewPort->TopLeftY = (float)(SHADOWMAP_TILESIZE * aLastTakenIndex_V);

			myContext->RSSetViewports(1, myViewPort);

			aLastTakenIndex_H += sortedPoint.tileAmount;
			//Set Shadow Camera
			LightShadowCamera& lightCamera = myShadowCameras[cameraIndex + direction];
			aShadowLightCount++;
			aCameraTransform.SetRotationIgnoreScale(myPointsRotationTransform[direction]);
			lightCamera.projection = myShadowCamera.GetProjection();
			lightCamera.transform = m4f::GetFastInverse(aCameraTransform);

			myConstantBufferManager->MapUnMapCameraBuffer(myShadowCamera);
			//RenderModels
			ClearMeshes(meshesToFill, aRenderBuffer->sortedMeshesSize, aRenderBuffer->sortedAnimMeshesSize);
			jobCounter = 4;
			SortModelsAndCalculateMVPs(aRenderBuffer, meshesToFill, myShadowCamera, jobCounter, myModelManager);
			RenderModels(meshesToFill, aDrawcallCounter, false);
		}
		if (aHasJumped == false)
		{
			aLastSkippedIndex_V = aLastTakenIndex_V;
			aLastSkippedIndex_H = aLastTakenIndex_H;
		}
	}
}

void Engine::ShadowRenderer::PointLightShadowAtlasJumpLogic(unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped, const unsigned short aTileAmount)
{
	if (aHasJumped == false)
	{
		aLastSkippedIndex_V = aLastTakenIndex_V;
		aLastSkippedIndex_H = aLastTakenIndex_H;
		aHasJumped = true;
	}
	aLastTakenIndex_H = 0;
	aLastTakenIndex_V += aTileAmount;
	bool foundAClearTileArea = false;
	if (myUsedIndices[aLastTakenIndex_H][aLastTakenIndex_V])
	{
		for (unsigned short w = 0; w < NUMB_SHADOWMAP_TILES; w++)
		{
			if (myUsedIndices[w][aLastTakenIndex_V] == false)
			{
				aLastTakenIndex_H = w;
				//foundAClearTileArea = true;
				break;
			}
		}
	}
	if (myUsedIndices[aLastTakenIndex_H][aLastTakenIndex_V])
	{
		for (unsigned short h = aLastTakenIndex_V; h < NUMB_SHADOWMAP_TILES; h++)
		{
			if (myUsedIndices[aLastTakenIndex_H][h] == false)
			{
				aLastTakenIndex_V = h;
				//foundAClearTileArea = true;
				break;
			}
		}
	}


	if (aLastTakenIndex_H + (aTileAmount * 6) >= NUMB_SHADOWMAP_TILES)
	{
		PointLightShadowAtlasJumpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, aHasJumped, aTileAmount);
	}
	//if (foundAClearTileArea == false)
	//{
	//	PointLightShadowAtlasJumpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, aHasJumped, aTileAmount);
	//}
	bool tileNotClear = true;
	for (unsigned short w = aLastTakenIndex_H; w < aLastTakenIndex_H + (aTileAmount * 6); w++)
	{
		for (unsigned short h = aLastTakenIndex_V; h < aLastTakenIndex_V + aTileAmount; h++)
		{
			if (myUsedIndices[w][h] == true)
			{
				tileNotClear = false;
				break;
			}
		}
		if (tileNotClear)
		{
			break;
		}
	}
	if (tileNotClear == false)
	{
		PointLightShadowAtlasJumpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, aHasJumped, aTileAmount);
	}

}

void Engine::ShadowRenderer::SpotLightShadowAtlasCatchUpLogic(unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, const unsigned short aTileAmount)
{
	aLastSkippedIndex_H += aTileAmount;
	if (aLastSkippedIndex_H >= NUMB_SHADOWMAP_TILES)
	{
		aLastSkippedIndex_V += aTileAmount;
		aLastSkippedIndex_H = 0;	
	}
	bool foundFreeSpot = false;
	if (myUsedIndices[aLastSkippedIndex_H][aLastSkippedIndex_V])
	{
		for (unsigned short w = 0; w < NUMB_SHADOWMAP_TILES; w++)
		{
			if (myUsedIndices[w][aLastSkippedIndex_V] == false)
			{
				aLastSkippedIndex_H = w;
				foundFreeSpot = true;
				break;
			}
		}
	}
	if (myUsedIndices[aLastSkippedIndex_H][aLastSkippedIndex_V])
	{
		for (unsigned short h = aLastSkippedIndex_V; h < NUMB_SHADOWMAP_TILES; h++)
		{
			if (myUsedIndices[aLastSkippedIndex_H][h] == false)
			{
				aLastSkippedIndex_V = h;
				foundFreeSpot = true;
				break;
			}
		}
	}
	if (foundFreeSpot)
	{
		bool tileNotClear = true;
		for (unsigned short w = aLastSkippedIndex_H; w < aLastSkippedIndex_H + aTileAmount; w++)
		{
			for (unsigned short h = aLastTakenIndex_V; h < aLastTakenIndex_V + aTileAmount; h++)
			{
				if (myUsedIndices[w][h] == true)
				{
					tileNotClear = false;
					break;
				}
			}
			if (tileNotClear)
			{
				break;
			}
		}
		if (tileNotClear)
		{
			SpotLightShadowAtlasCatchUpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, aTileAmount);
		}
	}
}

void Engine::ShadowRenderer::SpotLightShadowAtlasJumpLogic(unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped, const unsigned short aTileAmount)
{
	aLastTakenIndex_H += aTileAmount;
	if (aLastTakenIndex_H >= NUMB_SHADOWMAP_TILES)
	{
		aLastTakenIndex_H = 0;
		aLastTakenIndex_V += aTileAmount;
	}

	bool foundFreeSpot = true;
	if (myUsedIndices[aLastTakenIndex_H][aLastTakenIndex_V])
	{
		foundFreeSpot = false;
		for (unsigned short w = 0; w < NUMB_SHADOWMAP_TILES - aTileAmount; w++)
		{
			if (myUsedIndices[w][aLastTakenIndex_V] == false)
			{
				aLastTakenIndex_H = w;
				foundFreeSpot = true;
				break;
			}
			else
			{
				aLastTakenIndex_H = w;
			}
		}
	}
	//if (/*myUsedIndices[aLastTakenIndex_H][aLastTakenIndex_V]*/foundFreeSpot == false)
	//{
	//	for (unsigned short h = aLastTakenIndex_V; h < NUMB_SHADOWMAP_TILES; h++)
	//	{
	//		if (myUsedIndices[aLastTakenIndex_H][h] == false)
	//		{
	//			aLastSkippedIndex_V = h;
	//			foundFreeSpot = true;
	//			break;
	//		}
	//	}

	//}
	if (foundFreeSpot == false)
	{
		SpotLightShadowAtlasJumpLogic(aLastTakenIndex_H, aLastTakenIndex_V, aLastSkippedIndex_H, aLastSkippedIndex_V, aHasJumped, aTileAmount);
	}
	aLastSkippedIndex_V = aLastTakenIndex_V;
	aLastSkippedIndex_H = aLastTakenIndex_H;
	aHasJumped = false;
}

void Engine::ShadowRenderer::RenderModels(MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, bool aStaticRender)
{
	unsigned short staticSize = (unsigned short)meshesToFill.staticMeshCount;
	for (unsigned short i = 0; i < staticSize; i++)
	{
		DX::RenderInstancedModelBatch(meshesToFill.myStaticMeshes[i], *myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, myModelManager, true);
	}
	if (aStaticRender == false)
	{
		unsigned short normalSize = meshesToFill.normalMeshListCount;
		for (unsigned short i = 0; i < normalSize; i++)
		{
			DX::RenderInstancedModelBatch(meshesToFill.myNormalMeshes[i], *myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, myModelManager, true);
		}
	}
	unsigned short uniqueStaticSize = meshesToFill.myUniqueStaticMeshes.Size();
	for (unsigned short i = 0; i < uniqueStaticSize; i++)
	{
		DX::RenderModel(meshesToFill.myUniqueStaticMeshes[i], *myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, myModelManager, true, true);
	}
	if (aStaticRender == false)
	{
		unsigned short uniqueNormalSize = meshesToFill.myUniqueNormalMeshes.Size();
		for (unsigned short i = 0; i < uniqueNormalSize; i++)
		{
			DX::RenderModel(meshesToFill.myUniqueNormalMeshes[i], *myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, myModelManager, true, true);
		}

		unsigned int animSize = (unsigned int)meshesToFill.animNormalCount;
		for (unsigned short i = 0; i < animSize; i++)
		{
			DX::RenderInstancedAnimatedModelBatch(meshesToFill.myNormalAnimMeshes[i], *myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true);
		}
		unsigned short uniqueAnimSize = meshesToFill.myUniqueAnimatedNormalMeshes.Size();
		for (unsigned short i = 0; i < uniqueAnimSize; i++)
		{
			DX::RenderAnimatedModel(meshesToFill.myUniqueAnimatedNormalMeshes[i], *myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter, true, true);
		}
		myConstantBufferManager->MapUnMapEmptyEffectBuffer();
		for (size_t i = 0; i < meshesToFill.particleMeshCount; i++)
		{
			DX::RenderParticleMeshBatch(myContext, &meshesToFill.particlesMesh[i], *myConstantBufferManager, DX::EModelRenderMode::EOnlyVertexShader, aDrawcallCounter);
		}
	}

	myConstantBufferManager->MapUnMapEmptyEffectBuffer();
}
