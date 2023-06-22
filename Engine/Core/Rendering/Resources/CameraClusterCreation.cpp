#include "stdafx.h"
#include "CameraClusterCreation.h"
#include "GameObjects\Camera.h"
#include "RenderFunctions.h"
#include <d3d11.h>
#include "ConstantBufferManager.h"
#include "imgui\imgui.h"
#include "RenderData.h"
#include "FullScreenTexture.h"
#include "Texture3D.h"


//LEGACY Cluster on CPU creation

//uint16_t pointLightClusterList[CLUSTER_WIDTH * CLUSTER_HEIGTH * CLUSTER_DEPTH][USHRT_MAX];
//uint16_t spotlightClusterList[CLUSTER_WIDTH * CLUSTER_HEIGTH * CLUSTER_DEPTH][USHRT_MAX];
//
//void Engine::DX::FillCluster(RenderData& aBuffer, Camera& aCamera, ConstantBufferManager& aCBufferManager)
//{
//	memset(&pointLightClusterList[0], 0, CLUSTER_WIDTH * CLUSTER_HEIGTH * CLUSTER_DEPTH * sizeof(uint16_t));
//	memset(&spotlightClusterList[0], 0, CLUSTER_WIDTH * CLUSTER_HEIGTH * CLUSTER_DEPTH * sizeof(uint16_t));
//	HRESULT result;
//	D3D11_MAPPED_SUBRESOURCE mapped3DTex;
//	uint32_t* clusterData = nullptr;
//	result = aCBufferManager.GetContext()->Map(aCBufferManager.GetLightClusterTexture().GetTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped3DTex);
//	if (FAILED(result)) { assert(false && "Failed to Unmap LightCluster Texture"); }
//	else
//	{
//		clusterData = (uint32_t*)mapped3DTex.pData;
//	}
//	if (clusterData)
//	{
//		unsigned int depthPitch = mapped3DTex.DepthPitch;
//		unsigned int rowPitch = mapped3DTex.RowPitch;
//		memset(&clusterData[0], 0, (mapped3DTex.DepthPitch / 4) * (mapped3DTex.RowPitch / 4));
//		CU::AABB3Df clusterBound = aCamera.GetClusterBounds();
//		const m4f V = m4f::GetFastInverse(aCamera.GetMatrix());
//		const uint32_t size = (CLUSTER_WIDTH * CLUSTER_HEIGTH * CLUSTER_DEPTH);
//		CU::AABB3Df* clusterDepthBounds = aCamera.GetClusterDepth();
//		for (uint16_t point = 0; point < aBuffer.pointLightsSize; point++)
//		{
//			v4f pointPos = v4f(aBuffer.pointLights[point].position, 1.0f);
//			pointPos = pointPos * V;
//			v3f pointPosMultiplied = { pointPos.x, pointPos.y, pointPos.z };
//			//RenderDebugFunctions::DrawBox(&aBuffer, clusterBound, 4.0f, { 50, 255, 0, 255 });
//			//RenderDebugFunctions::DrawSphere(&aBuffer, v3f(pointPos.x, pointPos.y, pointPos.z), aBuffer.pointLights[point].range * 2, {255, 25, 255, 100});
//			//RenderDebugFunctions::DrawLine(&aBuffer, { pointPos.x, pointPos.y, pointPos.z }, 
//				//{ pointPos.x + aBuffer.pointLights[point].range, pointPos.y + aBuffer.pointLights[point].range, pointPos.z + aBuffer.pointLights[point].range }, {0, 20, 255, 255}, 4.0f);
//
//			if (testSphereAABB(aBuffer.pointLights[point].range, clusterBound, pointPosMultiplied))
//			{
//				for (uint32_t d = 0; d < CLUSTER_DEPTH; d++)
//				{
//					if (testSphereAABB(aBuffer.pointLights[point].range, clusterDepthBounds[d], pointPosMultiplied))
//					{
//						for (uint32_t cluster = 0; cluster < CLUSTER_WIDTH * CLUSTER_HEIGTH; cluster++)
//						{
//							if (testSphereAABB(aBuffer.pointLights[point].range, aCamera.GetClusterWidthHeigth(d)[cluster], pointPosMultiplied) && (clusterData[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) * 2 + cluster * 2 + 1] & 0xffff) < 128)
//							{
//								uint32_t pointLightAmount = (clusterData[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) * 2 + cluster * 2 + 1] & 0xffff);
//								(clusterData[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) * 2 + cluster * 2 + 1] &= 0xffff)++;
//								pointLightClusterList[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) + cluster][pointLightAmount] = point;
//							}
//						}
//					}
//				}
//			}
//		}
//		const v4f clusterBoundSphere = clusterBound.GetSphereFromVolume();
//		for (uint16_t spot = 0; spot < aBuffer.spotLightsSize; spot++)
//		{
//			SpotLightRenderCommand& light = aBuffer.spotLights[spot];
//			v4f spotPos = v4f(light.position, 1.0f);
//			spotPos = spotPos * V;
//			v4f spotDirection = v4f(light.direction, 0.0f);
//			spotDirection = spotDirection * V;
//
//			v3f spotMutipliedDirection = v3f(spotDirection.x, spotDirection.y, spotDirection.z);
//			//v3f spotMultipliedDir = { spotDirection.x, spotDirection.y, spotDirection.z };
//			v3f spotMultipliedPos = { spotPos.x, spotPos.y, spotPos.z };
//			//RenderDebugFunctions::DrawSphereSquared(&aBuffer, clusterBoundSphere, {25, 25, 25, 100});
//			//RenderDebugFunctions::DrawSpot(&aBuffer, spotMultipliedPos, spotMutipliedDirection, light.angle, light.range, 2.0f);
//			if (TestConeVsSphere(spotMultipliedPos, spotMutipliedDirection, light.range, light.angle, clusterBoundSphere))
//			{
//				for (uint32_t d = 0; d < CLUSTER_DEPTH; d++)
//				{
//					if (TestConeVsSphere(spotMultipliedPos, spotMutipliedDirection, light.range, light.angle, clusterDepthBounds[d].GetSphereFromVolume()))
//					{
//						for (uint32_t cluster = 0; cluster < CLUSTER_WIDTH * CLUSTER_HEIGTH; cluster++)
//						{
//							if (TestConeVsSphere(spotMultipliedPos, spotMutipliedDirection, light.range, light.angle, aCamera.GetClusterWidthHeigth(d)[cluster].GetSphereFromVolume()) && (clusterData[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) * 2 + cluster * 2 + 1] >> 16) < 128)
//							{
//								uint16_t spotLightAmount = clusterData[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) * 2 + cluster * 2 + 1] >> 16;
//								spotlightClusterList[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) + cluster][spotLightAmount++] = spot;
//								clusterData[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) * 2 + cluster * 2 + 1] &= 0xffff;
//								clusterData[(d * CLUSTER_WIDTH * CLUSTER_HEIGTH) * 2 + cluster * 2 + 1] |= (spotLightAmount << 16);
//							}
//						}
//					}
//				}
//			}
//		}
//
//		D3D11_MAPPED_SUBRESOURCE mapped1DTex;
//		uint16_t* pLights = nullptr;
//		result = aCBufferManager.GetContext()->Map(aCBufferManager.GetLightIndexListTexture().GetTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped1DTex);
//		if (FAILED(result)) { assert(false && "Failed to Unmap LightIndex Texture"); }
//		else
//		{
//			pLights = (uint16_t*)mapped1DTex.pData;
//			memset(&pLights[0], 10, sizeof(uint16_t) * (MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS) * 2);
//		}
//
//		uint32_t latestOffset = 0;
//		//uint32_t widthMod = 0;
//		//uint32_t widthCounter = 0;
//		for (uint32_t cluster = 0; cluster < size; cluster++)
//		{
//			uint32_t index3DX = cluster * 2;	// == .x
//			uint32_t index3DY = cluster * 2 + 1; // + 1 == .y
//		
//			uint16_t spotlightAmount = clusterData[index3DY] >> 16;
//			uint16_t pointlightAmount = clusterData[index3DY] & 0xffff;
//			clusterData[index3DX] = latestOffset;
//			for (uint16_t pLight = 0; pLight < pointlightAmount; pLight++)
//			{
//				assert(latestOffset + pLight  < (MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS) * 2 && "OutofPixelBounds");
//				uint16_t index = pointLightClusterList[cluster][pLight];
//				pLights[latestOffset + pLight] = index;
//			}
//			for (uint16_t sLight = 0; sLight < spotlightAmount; sLight++)
//			{
//				assert(latestOffset + pointlightAmount + sLight < (MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS) * 2 && "OutofPixelBounds");
//				pLights[latestOffset + pointlightAmount + sLight] = spotlightClusterList[cluster][sLight];
//			}
//			latestOffset += pointlightAmount + spotlightAmount;
//		}
//		aCBufferManager.GetContext()->Unmap(aCBufferManager.GetLightIndexListTexture().GetTexture(), 0);
//		aCBufferManager.GetLightIndexListTexture().SetAsResourceOnSlot(29);
//		aCBufferManager.GetContext()->Unmap(aCBufferManager.GetLightClusterTexture().GetTexture(), 0);
//		aCBufferManager.GetLightClusterTexture().SetAsResourceOnSlot(30);
//	}


	//constants
	//const float preNumerator = logf(aCamera.GetFar() / aCamera.GetNear());
	//const float preDenominator = CLUSTER_DEPTH * logf(aCamera.GetNear());

	//float Z = 4010.f;
	//unsigned int slice = (unsigned int)(log(Z) * CLUSTER_DEPTH / preNumerator - preDenominator / preNumerator);
	//float t = (Z - aCamera.GetNear()) / (aCamera.GetFar() - aCamera.GetNear());
	//v4f pos(100, 100, Z, 1);

	//pos = pos * VP;
	//pos.x /= Z;
	//pos.y /= Z;
	//pos.z /= Z;
//}

void Engine::DX::FillClusterCS(ConstantBufferManager& aCBufferManager, bool aFillClusterOnlyLights)
{
	ID3D11DeviceContext* context = aCBufferManager.GetContext();
	ID3D11ShaderResourceView* NULLSRV = nullptr;
	context->PSSetShaderResources(29, 1, &NULLSRV);
	context->PSSetShaderResources(30, 1, &NULLSRV);

	ID3D11UnorderedAccessView* boundsUAV = aCBufferManager.GetClusterBoundUAV();
	context->CSSetUnorderedAccessViews(0, 1, &boundsUAV, 0);
	context->CSSetShader(aCBufferManager.GetClusterBoundsCreationCS(), nullptr, 0);
	context->Dispatch(CLUSTER_WIDTH_GPU, CLUSTER_HEIGTH_GPU, CLUSTER_DEPTH_GPU);

	Texture3D& ClusterTexture = aCBufferManager.GetClusterTexture();
	ID3D11UnorderedAccessView* clusterUAV = ClusterTexture.GetUAV();
	context->CSSetUnorderedAccessViews(0, 1, &clusterUAV, 0);
	FullScreenTexture& clusterIndex = aCBufferManager.GetClusterIndexTexture();
	ID3D11UnorderedAccessView* indexUAV = clusterIndex.GetUAV();
	context->CSSetUnorderedAccessViews(1, 1, &indexUAV, 0);

	ID3D11ShaderResourceView* boundsSRV = aCBufferManager.GetClusterBoundSRV();
	context->CSSetShaderResources(8, 1, &boundsSRV);

	if (aFillClusterOnlyLights)
	{
		context->CSSetShader(aCBufferManager.GetClusterFillCSNoHarmonicsOrProbes(), nullptr, 0);
	}
	else
	{
		context->CSSetShader(aCBufferManager.GetClusterFillCS(), nullptr, 0);
	}
	context->Dispatch(CLUSTER_WIDTH_GPU, CLUSTER_HEIGTH_GPU, CLUSTER_DEPTH_GPU);

	context->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* NULLUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &NULLUAV, 0);
	context->CSSetUnorderedAccessViews(1, 1, &NULLUAV, 0);
	aCBufferManager.GetClusterIndexTexture().SetAsResourceOnSlot(29);
	aCBufferManager.GetClusterTexture().SetAsResourceOnSlot(30);
}

