#pragma once
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;
struct D3D11_VIEWPORT;
struct ID3D11Device;
struct ID3D11DeviceContext;


namespace Engine
{
	enum EGBufferTexture
	{
		POSITION_NMSTRENGTH,
		ALBEDO_AO,
		NORMAL_METAL,
		VERTEXNORMAL_ROUGHNESS,
		EMISSIVE,
		DEPTH,
		COUNT
	};
	class Renderer;
	class FullScreenTexture;
	class GBuffer
	{
		friend class Renderer;
	public:
	
	public:
		GBuffer();
		GBuffer(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
		~GBuffer();

		void Init();
		void ClearTextures(CU::Color aColor = {0, 0, 0, 0});
		void SetAsActiveTarget(FullScreenTexture* aDepth = nullptr);
		void SetAsActiveTarget(EGBufferTexture aTextureToExclude, FullScreenTexture* aDepth = nullptr);
		void SetDecalTargetsAsTarget(FullScreenTexture* aDepth = nullptr);
		void SetAsResourceOnSlot(EGBufferTexture aTexture, unsigned int aSlot);
		void SetAllAsResources();

		ID3D11Texture2D** GetTexture(unsigned int aIndex) { return &myTextures[aIndex]; }
		ID3D11RenderTargetView** GetRTV(unsigned int aIndex) { return &myRTVs[aIndex]; }
		ID3D11ShaderResourceView** GetSRV(unsigned int aIndex) { return &mySRVs[aIndex]; }
		D3D11_VIEWPORT* GetViewPort() { return myViewport; }
		void SetViewPort(D3D11_VIEWPORT* aViewPort) { myViewport = aViewPort; }
		void ReleaseResources();
		void SetResolution(const v2ui aResolution);
		const v2ui GetResolution() const { return myResolution; }

	private:
		void ClearTexture(const unsigned int aTextureSlot, const CU::Color aClearColor);
		v2ui myResolution;
		ID3D11Texture2D* myTextures[EGBufferTexture::COUNT];
		ID3D11RenderTargetView* myRTVs[EGBufferTexture::COUNT];
		ID3D11ShaderResourceView* mySRVs[EGBufferTexture::COUNT];
		D3D11_VIEWPORT* myViewport;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		bool myIsInited;

	};
}


