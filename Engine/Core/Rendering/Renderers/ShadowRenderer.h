#pragma once
#include "ECS\Systems\RenderCommands.h"
#include "..\Resources\FullScreenTexture.h"
#include <utility>
#include <atomic>
#include "..\Resources\LightCamera.h"
#include "GameObjects\Camera.h"
#include <CU\Containers\MinHeap.hpp>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
class ModelAnimated;
class Camera;
struct MeshesToRender;

static const unsigned int MAX_SHADOWS = 10;
namespace Engine
{
	class ConstantBufferManager;
	struct RenderTarget;
	class FullScreenRenderer;
	class FullScreenTexture;
	class ModelManager;
	class ShadowRenderer
	{
	public:
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ConstantBufferManager* aConstantBufferManager);

		void RenderShadowMaps(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, FullScreenRenderer& aFullScreenRenderer);
		void RenderDirectionalShadowMap(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, Camera& aMainCamera, std::atomic<unsigned int>& aDrawcallCounter);
		void SortLights(RenderData* aRenderBuffer, Camera& aMainCamera, bool onlyMainCamera = false);
		void SortForSpecificCamera(RenderData* aRenderBuffer, Camera& aMainCamera);
	private:
		void SortStaticLights(RenderData* aRenderBuffer);
		void RenderStaticLights(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, m4f& aCameraTransform);
		
		void RenderSpotLights(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, unsigned short& aShadowLightCount, m4f& aCameraTransform, unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped);
		void RenderPointLights(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, unsigned short& aShadowLightCount, m4f& aCameraTransform, unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped);

		void PointLightShadowAtlasJumpLogic(unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped, const unsigned short aTileAmount );
		void SpotLightShadowAtlasCatchUpLogic(unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, const unsigned short aTileAmount);
		void SpotLightShadowAtlasJumpLogic(unsigned short& aLastTakenIndex_H, unsigned short& aLastTakenIndex_V, unsigned short& aLastSkippedIndex_H, unsigned short& aLastSkippedIndex_V, bool& aHasJumped, const unsigned short aTileAmount);
		void RenderModels(MeshesToRender& meshesToFill, std::atomic<unsigned int>& aDrawcallCounter, bool aStaticRender);
		struct SortedLight
		{
			float distance = 0;
			unsigned short lightIndex = 0;
			unsigned short tileAmount = 0;
			bool isSpotLight = false; // if false it is a Point
			bool operator >(SortedLight& aLightToCompare) {
				return distance > aLightToCompare.distance;
			}
			SortedLight& operator=(const SortedLight& aCopy)
			{
				distance = aCopy.distance;
				lightIndex = aCopy.lightIndex;
				tileAmount = aCopy.tileAmount;
				isSpotLight = aCopy.isSpotLight;
				return (*this);
			}
		};

		ConstantBufferManager* myConstantBufferManager;
		LightShadowCamera myShadowCameras[NUMB_SHADOWMAP_TILETOTAL];
		bool myUsedIndices[NUMB_SHADOWMAP_TILES][NUMB_SHADOWMAP_TILES];

		Camera* myRenderTargetCameras[NUMBOF_RENDERTARGETS + 1];
		m4f myToCameras[NUMBOF_RENDERTARGETS + 1];
		size_t myNumberOfRenderTargets = 0;

		CU::MinHeap<SortedLight> mySortedShadowLights;
		CU::GrowingArray<SortedLight> mySortedStaticSpotLights;
		CU::GrowingArray<SortedLight> mySortedStaticPointLights;
		CU::VectorOnStack<SortedLight, NUMB_SHADOWMAP_TILETOTAL> mySpotLightsUntilRangeThreshold;
		CU::VectorOnStack<SortedLight, NUMB_SHADOWMAP_TILETOTAL> myPointLightsUntilRangeThreshold;

		unsigned short myAmountOfRenderedStaticSpots = 0;
		unsigned short myAmountOfRenderedStaticPoints = 0;
		unsigned short myStaticSpotTileSize = 0;
		unsigned short myStaticPointTileSize = 0;
		bool myWillReRenderStaticSpots = false;
		bool myWillReRenderStaticPoints = false;

		Camera myShadowCamera;

		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;

		CU::Matrix4x4f myPointsRotationTransform[6];

		D3D11_VIEWPORT* myViewPort;
		//Shadow Maps
		//ID3D11Texture2D* myAtlasTexture;
		//ID3D11ShaderResourceView* myAtlasSRV;
		//ID3D11DepthStencilView* myAtlasDepth;

		//ID3D11Texture2D* myStaticSpotTexture;
		//ID3D11ShaderResourceView* myStaticSpotSRV;
		//ID3D11DepthStencilView* myStaticSpotDepth;

		//ID3D11Texture2D* myStaticPointTexture;
		//ID3D11ShaderResourceView* myStaticPointSRV;
		//ID3D11DepthStencilView* myStaticPointDepth;

		FullScreenTexture* myFinalTexture = nullptr;
		FullScreenTexture* myAtlasTexture = nullptr;
		FullScreenTexture* mySpotTexture = nullptr;
		FullScreenTexture* myPointTexture = nullptr;

		//ID3D11Texture2D* myDirectionalTexture;
		//ID3D11ShaderResourceView* myDirectionalSRV;
		//ID3D11DepthStencilView* myDirectionalDepth;

		ModelManager* myModelManager = nullptr;
	};
}


