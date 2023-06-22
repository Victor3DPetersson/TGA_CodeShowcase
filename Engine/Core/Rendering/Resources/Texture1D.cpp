#include "stdafx.h"
#include "Texture1D.h"
#include <d3d11.h>

Engine::Texture1D::Texture1D()
{
	myTexture = nullptr;
	mySRV = nullptr;
	myDevice = nullptr;
	myContext = nullptr;
	myResolution = 0;
}

Engine::Texture1D::Texture1D(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myTexture = nullptr;
	mySRV = nullptr;
	myResolution = 0;
}

Engine::Texture1D::Texture1D(const Texture1D& aCopy)
{
	myDevice = aCopy.myDevice;
	myContext = aCopy.myContext;
	myTexture = aCopy.myTexture;
	mySRV = aCopy.mySRV;
	myResolution = aCopy.myResolution;
}

bool Engine::Texture1D::Init(const unsigned int aResolution, DXGI_FORMAT aFormat, ETextureUsageFlags aTextureFlag, bool aCpuAccess, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aCPUWrite)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;
	D3D11_TEXTURE1D_DESC desc = { 0 };
	desc.Width = aResolution;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = aFormat;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = (D3D11_USAGE)(unsigned int)aTextureFlag;
	if (aCpuAccess)
	{
		if (aCPUWrite)
		{
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		}
	}
	else
	{
		desc.CPUAccessFlags = 0;
	}
	desc.MiscFlags = 0;

	ID3D11Texture1D* texture;
	result = aDevice->CreateTexture1D(&desc, nullptr, &texture);
	assert(SUCCEEDED(result));

	ID3D11ShaderResourceView* SRV;
	result = aDevice->CreateShaderResourceView(texture, nullptr, &SRV);
	assert(SUCCEEDED(result));

	SetResolution(aResolution);
	SetTexture(texture);
	SetSRV(SRV);

	return true;
}


void Engine::Texture1D::SetAsResourceOnSlot(const unsigned int aSlot)
{
	myContext->PSSetShaderResources(aSlot, 1, &mySRV);
}

void Engine::Texture1D::SetSRV(ID3D11ShaderResourceView* aSRV)
{
	mySRV = aSRV;
}

void Engine::Texture1D::SetTexture(ID3D11Texture1D* aTexture)
{
	myTexture = aTexture;
}

void Engine::Texture1D::ReleaseResources()
{
	SAFE_RELEASE(myTexture);
	SAFE_RELEASE(mySRV);
	myDevice = nullptr;
	myContext = nullptr;
}

void Engine::Texture1D::SetResolution(const unsigned int aResolution)
{
	myResolution = aResolution;
}

