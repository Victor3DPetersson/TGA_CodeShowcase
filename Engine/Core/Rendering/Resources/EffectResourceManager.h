#pragma once
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11SamplerState;
struct ID3D11Buffer;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

namespace Engine
{
	class FullScreenTexture;
	class FullScreenTextureArray;
	class ConstantBufferManager;
	class FullScreenRenderer;
	class GBuffer;
	class Renderer;
	struct RenderTarget;


	class EffectResourceManager
	{
	public:
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, FullScreenRenderer* aFullscreenRenderer, ConstantBufferManager* aCBufferManager, Renderer* aRenderer, v2ui aRenderResolution);
		//// ----------- Depth of field --------------- ///
		void ResizeScreenTextures(v2ui aRenderResolution);
		void RenderDoF(FullScreenTexture* aMainTarget, FullScreenTexture* aIntermediateTarget, std::atomic<unsigned int>& aDrawcallCounter);
		//// ----------- Icon rendering --------------- ///
		bool Icon_InitResources();
		void Icon_ReleaseResources();
		//void Icon_Render(void* aIconResourceToRender, unsigned int aIconType);

	private:
		//// ----------- Depth of field --------------- ///
		FullScreenTextureArray* myDoF_SeperatedColorArray = nullptr;
		FullScreenTexture* myDoF_Near = nullptr;
		FullScreenTexture* myDoF_Far = nullptr;
		FullScreenTexture* myDoF_Intermediary = nullptr;
		FullScreenTexture* myDOF_DepthMasks = nullptr;

		//// ----------- Icon rendering --------------- ///
		FullScreenTexture* myIcon_depth = nullptr;
		FullScreenTexture* myIcon_intermediate = nullptr;
		GBuffer* myIcon_gBuffer = nullptr;
		RenderTarget* myIcon_RT = nullptr;

		ID3D11Device* myDevice = nullptr;
		ID3D11DeviceContext* myContext = nullptr;
		ConstantBufferManager* myCBM = nullptr;
		FullScreenRenderer* myFullscreenRenderer = nullptr;
		Renderer* myRenderer = nullptr;
	};

}