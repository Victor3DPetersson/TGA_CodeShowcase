#pragma once
#include "../CommonUtilities/CU\Math\Color.hpp"
#include "DX_Includes.h"
#include "RenderFunctions.h"

struct ID3D11Texture3D;
struct ID3D11ShaderResourceView;
struct D3D11_VIEWPORT;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11UnorderedAccessView;
namespace Engine
{
	class Texture3D
	{
	public:
		Texture3D();
		Texture3D(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
		Texture3D(const Texture3D& aCopy);
		~Texture3D() = default;
		bool Init(const v3ui aResolution, DXGI_FORMAT aFormat, ETextureUsageFlags aTextureFlag, bool aCpuAccess, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aCPUWrite, bool aUAV);
		void SetAsResourceOnSlot(const unsigned int aSlot);
		void SetSRV(ID3D11ShaderResourceView* aSRV);
		void SetTexture(ID3D11Texture3D* aTexture);

		void ReleaseResources();
		ID3D11ShaderResourceView* GetSRV() { return mySRV; }
		ID3D11Texture3D* GetTexture() { return myTexture; }
		void SetAsCSOutput(unsigned int aOutputSlot);

		void SetResolution(const v3ui aResolution);
		v3ui GetResolution() const { return myResolution; }
		ID3D11UnorderedAccessView* GetUAV() { return myUAV; }
	private:
		v3ui myResolution;
		ID3D11Texture3D* myTexture;
		ID3D11ShaderResourceView* mySRV;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		ID3D11UnorderedAccessView* myUAV;
	};
}


