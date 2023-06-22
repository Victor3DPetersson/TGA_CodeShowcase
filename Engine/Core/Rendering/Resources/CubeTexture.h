#pragma once
#include "DX_Includes.h"
#include "RenderFunctions.h"

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct D3D11_VIEWPORT;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
namespace Engine
{
	class CubeTexture
	{
	public:
		CubeTexture();
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite);
		bool InitWithFullMip(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite);
		ID3D11ShaderResourceView* GetSRV() { return mySRV; }
		ID3D11Texture2D* GetTexture() { return myTexture; }
		void ReleaseResources();
		void SetAsResourceOnSlot(const unsigned int aSlot);
		void SetAsCSResourceOnSlot(const unsigned int aSlot);

	private:
		ID3D11Texture2D* myTexture;
		ID3D11ShaderResourceView* mySRV;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		//ID3D11DepthStencilView* myDepth;
		ID3D11RenderTargetView* myRTV;
	};
	class CubeTextureArray
	{
	public:
		CubeTextureArray();
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite, unsigned int aArrayLength);
		bool InitWithFullMip(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite, unsigned int aArrayLength);
		ID3D11ShaderResourceView* GetSRV() { return mySRV; }
		ID3D11Texture2D* GetTexture() { return myTexture; }
		void ReleaseResources();
		void SetAsResourceOnSlot(const unsigned int aSlot);
		void SetAsCSResourceOnSlot(const unsigned int aSlot);
		bool GetIsInited() { return myIsInited; }
	private:
		bool myIsInited = false;
		ID3D11Texture2D* myTexture;
		ID3D11ShaderResourceView* mySRV;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		//ID3D11DepthStencilView* myDepth;
		ID3D11RenderTargetView* myRTV;
	};
}


