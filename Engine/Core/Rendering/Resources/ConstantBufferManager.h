#pragma once
#include <../CommonUtilities/CU/Math/Matrix4x4.hpp>
#include <../CommonUtilities/CU/Math/Vector2.hpp>
#include <../CommonUtilities/CU/Containers/VectorOnStack.h>
#include <../CommonUtilities/CU/Collision/Plane.hpp>
#include <../CommonUtilities/CU/Collision/AABB3D.hpp>
#include <../CommonUtilities/CU/Collision/Sphere.hpp>
#include <../Engine/GameObjects/Lights.h>
#include <../Engine/ECS/Systems/RenderCommands.h>
#include "Texture1D.h"
#include "Texture3D.h"
#include "LightCamera.h"

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
struct ID3D11UnorderedAccessView;
struct MeshBuffererCommand;
struct AnimatedMeshRenderCommand;
struct ID3D11ComputeShader;
class Camera;

namespace CU
{
	class Timer;
}

namespace Engine
{
	class FullScreenTexture;
	struct RenderCameraData
	{
		CU::Matrix4x4f toCTransform;
		m4f fromCamera;
		CU::Matrix4x4f CProjection;
		float myNear = 50;
		float myFar = 10000;
		float clusterPreNumerator = 0;
		float clusterPreDenominator = 0;
		CU::Matrix4x4f CInvProjection;
	};
	

	//Register b(0)
	struct ScreenResBufferData
	{
		float windowResolutionX;
		float windowResolutionY;
		float frameRenderResolutionX;
		float frameRenderResolutionY;
	};

	//Register b(1)
	struct GlobalFrameBufferData
	{
		v4f directionalColor;

		v3f directionalDirection;
		float deltaTime;

		CU::Color ambientFogColor;
		CU::Color ambientLightColor;

		float nearFogDistance;
		float farFogDistance;
		float fogExponent;
		float SH_GridSpacing = 1000.f;

		v3f levelMiddle;
		float totalTime;
		v3f levelHalfSize;
		float DoF_focusDistance;

		unsigned int pointLightCount = 0;
		unsigned int spotLightCount = 0;
		unsigned int reflectionProbeAmount = 0;
		unsigned int SHGridAmount = 0;
	};

	//Register b(2) this buffer is only mapped at Init, very special boy
	struct DefinesBufferData
	{
		unsigned int clusterWidth;
		unsigned int clusterHeigth;
		unsigned int clusterDepth;
		unsigned int lightIndexListSize;

		unsigned int shadowMapSize;
		unsigned int shadowMapTileSize;
		unsigned int numberOfShadowmapTiles;
		unsigned int numberOfShadowmapTilesTotal;

		unsigned int NOISE_RotationalTextureSize;
		unsigned int NumberOfMipsReflectionProbe;
		v2ui DefinesPadding;
	};

	//Register b(3)
	struct ObjectBufferData
	{
		m4f fromOB_toWorld;
	};

	//Register b(4)
	struct ObjectBoneBufferData
	{
		m4f bones[MAX_BONECOUNT];
	};

	//Register b(6)
	struct DecalObjectBufferData
	{
		m4f fromOB_toWorld;
		m4f fromWorld_toOB;
	};
	//Register b(7)
	struct RenderCameraBuffer
	{
		RenderCameraData renderCam;
		v3f renderCameraPosition;
		float padding;
	};

	//Register b(8)
	struct NoiseBufferData
	{
		v4f halfNoiseHemisphere[NOISE_HALFHEMISPHEREAMOUNT];
		v2f poissonKernel[32];
	};

	//Register b(10)
	//PostProcessingData is in RenderCommands

	class ConstantBufferManager
	{
	public:

		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, CU::Timer* aTimer, unsigned int aNumberOfMips);

		ScreenResBufferData myScreenBufferData;
		GlobalFrameBufferData myGlobalFrameBufferData;
		DefinesBufferData myDefinesBufferData;
		ObjectBufferData myObjectBufferData;
		ObjectBoneBufferData myObjectBoneBufferData;
		ObjectEffectData objectEffectBufferData; 		//Register b(5)
		DecalObjectBufferData myDecalObjectBufferData;
		RenderCameraBuffer myRenderCameraBufferData;
		PostProcessingData myPostProcessingData;

		v4ui myPointlightShadowIndex[MAX_POINT_LIGHTS];
		v4ui mySpotlightShadowIndex[MAX_SPOT_LIGHTS];

		bool MapUnMapScreenBuffer(v2f aWindowResolution, v2f aRenderResolution);
		bool MapUnMapGlobalFrameBuffer();
		void MapUnmapLightBuffer(RenderData& aRenderBuffer);
		void MapUnmapShadowCameraBuffer(LightShadowCamera* someLightCams, size_t aAmountOfCameras);
		void MapUnMapObjectBuffer();
		void MapUnMapObjectToBuffers(MeshRenderCommand& aRenderCommand);
		void MapUnMapAnimatedObjectToBuffers(AnimatedMeshRenderCommand& aRenderCommand);
		void MapUnMapDecalToBuffers(m4f aDecalMatrix);
		void MapUnMapCameraBuffer(Camera& aCamera);
		void MapUnMapEmptyEffectBuffer();
		void MapUnMapEffectBuffer();
		void MapUnmapSHSet(SH* aSHSet, const unsigned int aNumberOfSH);
		void MapUnmapPostProcessing();

		void MapUnMapStructuredMeshBuffer(SortedModelDataForRendering& aMeshBuffer);
		void MapUnMapStructuredAnimatedMeshBuffer(SortedAnimationDataForBuffers& aAnimatedMeshBuffer);

		ID3D11UnorderedAccessView* GetClusterBoundUAV() { return myClusterVolumesUAV; }
		ID3D11ShaderResourceView* GetClusterBoundSRV() { return myClusterBoundingVolumesSRV; }
		ID3D11ComputeShader* GetClusterBoundsCreationCS() { return myClusterBoundsCreation; }
		ID3D11ComputeShader* GetClusterFillCS() { return myClusterFill; }
		ID3D11ComputeShader* GetClusterFillCSNoHarmonicsOrProbes() { return myClusterFillNoHarmonicsOrProbes; }
		FullScreenTexture& GetClusterIndexTexture() { return *myClusterLightIndexTexture; }
		Texture3D& GetClusterTexture() { return myClusterData; }
		
		//Texture1D& GetLightIndexListTexture() { return myLightIndexTexture; }
		//Texture3D& GetLightClusterTexture() { return myLightCluster; }
		ID3D11DeviceContext* GetContext() { return myContext; }
	private:

		ID3D11Buffer* myScreenResBuffer = nullptr;//(b0)
		ID3D11Buffer* myGlobalFrameBuffer = nullptr;//(b1)
		ID3D11Buffer* myDefinesBuffer = nullptr;//(b2)
		ID3D11Buffer* myObjectBuffer = nullptr;//(b3)
		ID3D11Buffer* myObjectBoneBuffer = nullptr;//(b4)
		ID3D11Buffer* myObjectEffectBuffer = nullptr;//(b5)
		ID3D11Buffer* myDecalObjectBuffer = nullptr;//(b6)
		ID3D11Buffer* myRenderCameraBuffer = nullptr;//(b7)
		ID3D11Buffer* myNoiseBuffer = nullptr;//(b8)
		ID3D11Buffer* myPostProcessingBuffer = nullptr;//(b10)



		//---------Structured Buffers--------------//
		FullScreenTexture* myClusterLightIndexTexture;
		Texture3D myClusterData;
		ID3D11ShaderResourceView* myClusterBoundingVolumesSRV = nullptr;
		ID3D11Buffer* myClusterBoundingVolumesBuffer = nullptr;
		ID3D11UnorderedAccessView* myClusterVolumesUAV = nullptr;

		ID3D11ComputeShader* myClusterBoundsCreation = nullptr;
		ID3D11ComputeShader* myClusterFill = nullptr;
		ID3D11ComputeShader* myClusterFillNoHarmonicsOrProbes = nullptr;
		//-----------Point Lights------------------//
		//t0
		ID3D11ShaderResourceView* myClusteredPointlightsSRV = nullptr;
		ID3D11Buffer* myClusteredPointlightBuffer = nullptr;
		//t1
		ID3D11ShaderResourceView* myClusteredPointlightsShadowIndexSRV = nullptr;
		ID3D11Buffer* myClusteredPointlightShadowIndexBuffer = nullptr;
		//-----------Spot Lights------------------//
		//t2
		ID3D11ShaderResourceView* myClusteredSpotlightsSRV = nullptr;
		ID3D11Buffer* myClusteredSpotlightBuffer = nullptr;
		//t3
		ID3D11ShaderResourceView* myClusteredSpotlightsShadowIndexSRV = nullptr;
		ID3D11Buffer* myClusteredSpotlightShadowIndexBuffer = nullptr;


		//-----------Light Cameras---------------//
		//t31
		ID3D11ShaderResourceView* myLightShadowCamerasSRV = nullptr;
		ID3D11Buffer* myLightShadowCameras = nullptr;

		//----------Model Buffers----------------//
		//t4
		ID3D11ShaderResourceView* myModelOBToWorldSRV = nullptr;
		ID3D11Buffer* myModelOBToWorldBuffer = nullptr;
		ID3D11ShaderResourceView* myAnimatedModelOBToWorldSRV = nullptr;
		ID3D11Buffer* myAnimatedModelOBToWorldBuffer = nullptr;
		//t6
		ID3D11ShaderResourceView* myModelEffectSRV = nullptr;
		ID3D11Buffer* myModelEffectBuffer = nullptr;
		ID3D11ShaderResourceView* myAnimatedModelEffectSRV = nullptr;
		ID3D11Buffer* myAnimatedModelEffectBuffer = nullptr;
		//t7
		ID3D11ShaderResourceView* myAnimatedModelSkeletonSRV = nullptr;
		ID3D11Buffer* myAnimatedModelSkeletonBuffer = nullptr;


		//------------Noise Data-------------//
		//t33
		FullScreenTexture* myRotationKernel = nullptr;
		
		//------------Light Probes------------//
		ID3D11ShaderResourceView* mySHGridSRV = nullptr;
		ID3D11Buffer* mySHGrid = nullptr;

		//-----Scattering Lut for new and sexier PBR------------//
		FullScreenTexture* myScatteringLUT = nullptr;


		ID3D11Device* myDevice = nullptr;
		ID3D11DeviceContext* myContext = nullptr;
		CU::Timer* myTimer;
	};


}