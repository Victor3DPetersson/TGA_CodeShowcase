#pragma once
#include "../CommonUtilities/CU\Math\Color.hpp"
#include "DX_Includes.h"
#include "RenderFunctions.h"
struct ID3D11Texture1D;
struct ID3D11ShaderResourceView;
struct D3D11_VIEWPORT;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;


namespace Engine
{
	class Texture1D
	{
	public:
		Texture1D();
		Texture1D(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
		Texture1D(const Texture1D& aCopy);
		bool Init(const unsigned int aResolution, DXGI_FORMAT aFormat, ETextureUsageFlags aTextureFlag, bool aCpuAccess, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aCPUWrite = true);
		~Texture1D() = default;
		void SetAsResourceOnSlot(const unsigned int aSlot);
		void SetSRV(ID3D11ShaderResourceView* aSRV);
		void SetTexture(ID3D11Texture1D* aTexture);

		void ReleaseResources();
		ID3D11ShaderResourceView* GetSRV() { return mySRV; }
		ID3D11Texture1D* GetTexture() { return myTexture; }

		void SetResolution(const unsigned int aResolution);
		unsigned int GetResolution() const { return myResolution; }
	private:
		unsigned int myResolution;
		ID3D11Texture1D* myTexture;
		ID3D11ShaderResourceView* mySRV;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
	};
}


