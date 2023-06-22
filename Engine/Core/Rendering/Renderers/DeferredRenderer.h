#pragma once
#include <Core\DirectXFramework.h>
#include "GameObjects/Camera.h"
#include "RenderData.h"
#include "RenderConstants.hpp"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11SamplerState;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11PixelShader;
struct ID3D11ShaderResourceView;
struct ID3D11VertexShader;
struct ID3D11BlendState;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
class ModelAnimated;
struct MeshesToRender;
class Camera;

namespace Engine
{
	struct ShadowMapsWithCameras;
	struct RenderData;
	class FullScreenTexture;
	class GBuffer;
	class ModelManager;
	class ConstantBufferManager;
	struct RenderStates;
	class FullScreenRenderer;
	class ParticleManager;
	class DeferredRenderer
	{
	private:
	
		struct Dynamic_PointLightBufferData
		{
			PointLightRenderCommand myPoint;
		}myDynamic_PointLightData[10];

		struct Dynamic_SpotLightBufferData
		{
			SpotLightRenderCommand mySpot;
			CameraData myCamera;
		}myDynamic_SpotLightData[10];


	public:

		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ModelManager* aModelManager);

		//void FillFrameBuffer(Camera* aCamera, size_t aNumbOfPointLights, size_t aNumbOfSpotLights, const CU::Vector2ui aWindowResolution, const v2ui aRenderSourceResolution, RenderData* someSceneData);
		void GenerateGBuffer(MeshesToRender& meshes, GBuffer* aGBuffer, 
			FullScreenTexture* aDepthTexture, 
			RenderData& decals, ConstantBufferManager& aConstantBufferManager, 
			Camera& aRenderCamera, RenderStates* someRenderStates, FullScreenRenderer* aFullscreenRenderer,
			std::atomic<unsigned int>& aDrawcallCounter);

		void RenderLights(RenderData* someSceneData, std::atomic<unsigned int>& aDrawcallCounter, bool aRenderOnlyLight);
		void RenderBakedLights(RenderData* someSceneData, std::atomic<unsigned int>& aDrawcallCounter);
		void RenderIrradiantLight(RenderData* someSceneData, std::atomic<unsigned int>& aDrawcallCounter);

		void RenderStaticMap(Camera& aCameraToRenderThrough, CU::GrowingArray<SortedModelDataForBuffers> someStaticMeshes);
	private:

		ID3D11Device* myDevice = nullptr;
		ID3D11DeviceContext* myContext = nullptr;
	

		ID3D11VertexShader* myFullScreenShader = nullptr;
		ID3D11PixelShader* myBakedLightShader = nullptr;
		ID3D11PixelShader* myDeferredLightShader = nullptr;
		ID3D11PixelShader* myIrradiantLightShader = nullptr;
		ID3D11PixelShader* myBakeCubeLightShader = nullptr;

		ID3D11VertexShader* myStaticVS = nullptr;
		ID3D11PixelShader* myStaticPS = nullptr;


		ID3D11VertexShader* myDecalVS = nullptr;
		Model* myDecalBox = nullptr;
		FullScreenTexture* myIntermediateVertexNormals;
		ModelManager* myModelManager = nullptr;
	};
}


