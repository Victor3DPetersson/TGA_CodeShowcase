#pragma once
#include "../CommonUtilities/CU\Math\Color.hpp"
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct D3D11_VIEWPORT;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11UnorderedAccessView;
namespace Engine
{
	class GBuffer;
	class FullScreenTexture
	{
		friend class GBuffer;
		friend class FullScreenTextureArray;
	public:
		FullScreenTexture();
		FullScreenTexture(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
		FullScreenTexture(const FullScreenTexture& aCopy);
		~FullScreenTexture();
		void Init();
		bool operator==(const FullScreenTexture& aCopy);
		void ClearTexture(const CU::Color aClearColor = { 0, 0, 0, 0 });
		void ClearDepth(float aClearDepthValue = 1.0f, unsigned int aClearStencilValue = 0);
		void SetAsActiveTarget(FullScreenTexture* aDepth = nullptr);
		void SetAsResourceOnSlot(const unsigned int aSlot);
		void SetAsCSResourceOnSlot(const unsigned int aSlot);
		void SetAsActiveDepth();
		inline const unsigned int GetID() const { return myIDCounter; }
		void SetSRV(ID3D11ShaderResourceView* aSRV);
		void SetTexture(ID3D11Texture2D* aTexture);
		void SetViewPort(D3D11_VIEWPORT* aViewPort);
		void SetRenderTarget(ID3D11RenderTargetView* aRenderTargetView);
		void SetDepthStencil(ID3D11DepthStencilView* aDepthStencilView);
		void SetUAV(ID3D11UnorderedAccessView* aUAV);
		void ReleaseResources();
		void SetAsCSOutput(unsigned int aOutputSlot);
		ID3D11ShaderResourceView* GetSRV() { return mySRV; }
		ID3D11UnorderedAccessView* GetUAV() { return myUAV; }
		ID3D11Texture2D* GetTexture() { return myTexture; }
		ID3D11RenderTargetView* GetRTV() { return myRenderTarget; }
		D3D11_VIEWPORT* GetViewPort() { return myViewPort; }
		void SetResolution(const v2ui aResolution);
		v2ui GetResolution() const { return myResolution; }

	private:
		union
		{
			ID3D11RenderTargetView* myRenderTarget;
			ID3D11DepthStencilView* myDepth;
		};
		unsigned int myId;
		static unsigned int myIDCounter;
		v2ui myResolution;
		ID3D11Texture2D* myTexture;
		ID3D11ShaderResourceView* mySRV;
		D3D11_VIEWPORT* myViewPort;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		ID3D11UnorderedAccessView* myUAV;
		bool myIsInited;

	};
	class FullScreenTextureArray
	{
	public:
		FullScreenTextureArray();
		FullScreenTextureArray(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, unsigned int aArrayLength);
		FullScreenTextureArray(const FullScreenTextureArray& aCopy);
		~FullScreenTextureArray();
		void Init();
		void InitRTVs();
		bool operator==(const FullScreenTextureArray& aCopy);
		void ClearTexture(const CU::Color aClearColor = { 0, 0, 0, 0 });
		void SetAsActiveTargets(FullScreenTexture* aDepth = nullptr);
		void SetAsResourceOnSlot(const unsigned int aSlot);
		void SetAsCSResourceOnSlot(const unsigned int aSlot);
		inline const unsigned int GetID() const { return myArrayIDCounter; }
		void SetSRV(ID3D11ShaderResourceView* aSRV);
		void SetTexture(ID3D11Texture2D* aTexture);
		void SetViewPort(D3D11_VIEWPORT* aViewPort);
		void SetRenderTarget(ID3D11RenderTargetView* aRenderTargetView, unsigned int aSlot);
		void SetUAV(ID3D11UnorderedAccessView* aUAV);
		void ReleaseResources();
		void SetAsCSOutput(unsigned int aOutputSlot);
		ID3D11ShaderResourceView* GetSRV() { return mySRV; }
		ID3D11UnorderedAccessView* GetUAV() { return myUAV; }
		ID3D11Texture2D* GetTexture() { return myTexture; }
		ID3D11RenderTargetView* GetRTV(unsigned int aIndex) { return myRenderTargets[aIndex]; }
		void SetResolution(const v2ui aResolution);
		v2ui GetResolution() const { return myResolution; }

	private:
		ID3D11RenderTargetView** myRenderTargets;
		unsigned int myId;
		static unsigned int myArrayIDCounter;
		v2ui myResolution;
		ID3D11Texture2D* myTexture;
		ID3D11ShaderResourceView* mySRV;
		D3D11_VIEWPORT* myViewPort;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		ID3D11UnorderedAccessView* myUAV;
		unsigned int myArrayLength;
		bool myIsInited;
	};
}


