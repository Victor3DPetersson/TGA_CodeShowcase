#pragma once
#include <atomic>
#include "../CommonUtilities/CU/Containers/GrowingArray.hpp"
#include "../CommonUtilities/CU/Containers/VectorOnStack.h"

#include "../../ECS/Systems/RenderCommands.h"
#include "Resources\DX_Includes.h"
#include "../Engine/GameObjects/Lights.h"
#include "Resources\RenderStates.h"
#include "../Engine/RenderConstants.hpp"

#include "Resources\ConstantBufferManager.h"
#include "Resources\MeshStruct.h"
#include "Resources\EffectResourceManager.h"

class ModelInstance;
class ParticleEmitter_Instance;

struct ID3D11DeviceContext;
struct ID3D11Device;

struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11SamplerState;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;
struct ID3D11Buffer;
struct SpotLightRenderCommand;
struct PointLightRenderCommand;

struct MeshBuffererCommand;
struct AnimatedMeshRenderCommand;

namespace CU
{
	class Timer;
	class InputManager;
}

namespace Engine
{
	class SpriteRenderer;
	class CubemapRenderer;
	struct EngineManagers;
	struct Renderers;
	struct RenderTarget;
	class FullScreenTexture;
	class GBuffer;
	class ParticleManager;
	class ModelManager;
	class DirectXFramework;
	struct RenderData;

	class Renderer
	{
		enum EDebugState
		{
			EDEFAULT,
			EPOSITION,
			EALBEDO,
			ENORMAL,
			EVERTEXNORMAL,
			EAMBIENTOCCLUSION,
			EDEPTH,
		};

	public:
		Renderer();
		bool Init(DirectXFramework* aFrameworkPtr, CU::Timer* aTimer, const CU::Color aBackGroundColor, EngineManagers* someManagers, v2ui& aRenderResToFill);
		void Render(std::atomic<unsigned int>& aDrawcallCounter, bool aIgnoreShadowRendering = false);
		void RenderSpecifiedTarget(RenderTarget& aTargetToRender);
		void RenderModelToResource(Model* aModel, FullScreenTexture** aTextureToFill, RenderTarget& aTargetToRender, v3f dirLightRot);
		void RenderModelAnimatedToResource(ModelAnimated** aModel, FullScreenTexture** aTextureToFill, RenderTarget& aTargetToRender, m4f* aSkeleton, bool aDrawSkeleton, unsigned int aNumberOfModels, v3f dirLightRot, bool aRenderWithFloor = true, CU::Color aMeshColor = {255, 255, 255, 255});
		void RenderModelParticleToResource(ParticleRenderCommand* aCommand, size_t aNumbOfCommands, ParticleMeshRenderCommand* aMeshC, size_t aNumbOfMeshes, FullScreenTexture** aTextureToFill, RenderTarget& aTargetToRender, v3f dirLightRot, bool aRenderGround);
		void PrepareForResize();
		v2ui OnResize(v2ui aResolution, v2ui aWindowResolution);
		SpriteRenderer* GetSpriteRenderer();
		CubemapRenderer* GetCubeMapRenderer();
		void RenderSplatMap();
		void ShutDownRenderer();
		void SetFrameBuffersAreSwapped(bool aSwapState) { myBuffersAreSwapped = aSwapState; }
		void SetWaitForBuffersToSwap(bool aSwapState) { myWaitForFrameSwitch = aSwapState; }
		void SetPostProcessingData(PostProcessingData somePPData);

		__forceinline FullScreenTexture* GetIntermediateTexture()
		{
			return myIntermediateTexture;
		}
		void SetRenderWithHarmonics(bool aRenderWithSHState) { myRenderWithHarmonics = aRenderWithSHState; }
		void SetRenderIrradiantLightOnly(bool aRenderWithIrradianceState) { myRenderIrradianceOnly = aRenderWithIrradianceState; }
		void SetDebugState(unsigned int aDebugState) { myCurrentDebugState = aDebugState; }
		const unsigned int GetDebugState() { return myCurrentDebugState; }

	private:
		void RenderToTarget(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, GBuffer* aGbufferTarget, RenderFlag aRenderFlag, Camera& aRenderCamera, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender, bool aCustomModelBuffer, bool aIgnoreParticles);
		void PreFrame();
		void SetRenderDebugState();
		void RenderDeferredPasses(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, GBuffer* aGbufferTarget, Camera& aRenderCamera, RenderFlag aRenderFlag, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender, bool aIgnoreParticles);
		void RenderPostProcessingPass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender);
		void RenderUIPass(FullScreenTexture* aTarget, std::atomic<unsigned int>& aDrawcallCounter, Camera& aRenderCamera);
		void RenderIndexPass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, std::atomic<unsigned int>& aDrawcallCounter);
		void RenderRenderTargetpass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender);
		void RenderGBufferDebugPass(FullScreenTexture* aTarget, FullScreenTexture* aTargetDepth, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter, bool isMainRender);
		void ClearRenderDataAndModels();

		
		CU::Color myBackgroundColor;
		bool myRenderLevelTopDown = false;
		const float my19_9Ratio = 1920.0f / 1080.0f;
		friend class ForwardRenderer;
		friend class DeferredRenderer;

		ConstantBufferManager myConstantBufferManager;

		//Pointers to Engine objects
		DirectXFramework* myFramework = nullptr;
		CU::Timer* myTimer = nullptr;
		ParticleManager* myParticleManager = nullptr;
		ModelManager* myModelManager = nullptr;
		Renderers* myRenderers = nullptr;

		//D3D11 Stuff
		ID3D11Device* myDevice = nullptr;
		ID3D11DeviceContext* myContext = nullptr;

		GBuffer* myGBuffer = nullptr;

		CU::InputManager* myInputManager = nullptr;

		FullScreenTexture* myBackBuffer = nullptr;
		FullScreenTexture* myIntermediateDepth = nullptr;
		FullScreenTexture* myOutlineBuffer = nullptr;
		FullScreenTexture* myIntermediateTexture = nullptr;
		FullScreenTexture* myToneMapTexture = nullptr;
		FullScreenTexture* myHalfSizeTexture = nullptr;
		FullScreenTexture* myQuarterSizeTexture = nullptr;
		FullScreenTexture* myBlurTexture1 = nullptr;
		FullScreenTexture* myBlurTexture2 = nullptr;
		FullScreenTexture* myDefferedTexture = nullptr;
		FullScreenTexture* mySSAOTexture = nullptr;
		FullScreenTexture* myBlurredSSAOTexture = nullptr;

		RenderStates myRenderStates;
		EffectResourceManager myEffectManager;



		MeshesToRender myMeshesToRender;
		RenderData* myRenderData = nullptr;
		v2f myRenderResolution;
		v2f myWindowResolution;
		unsigned int myCurrentDebugState = 0;

		std::atomic<unsigned int> myJobCounter = 0;
		float myFrameTValue = 0;

		ID3D11PixelShader* myID_PS = nullptr;
		ID3D11ComputeShader* mySSAOBlur_CS = nullptr;
		ID3D11Buffer* myGBufferDebugBuffer = nullptr;
		bool myRenderWithHarmonics = true;
		bool myRenderIrradianceOnly = false;
		bool myWaitForFrameSwitch = false;
		bool myBuffersAreSwapped = false;

		Camera myDirectionalLightCamera;

	};
}


