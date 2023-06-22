#include "stdafx.h"
#include "Texture3D.h"
#include <d3d11.h>

Engine::Texture3D::Texture3D()
{
	myTexture = nullptr;
	mySRV = nullptr;
	myDevice = nullptr;
	myContext = nullptr;
	myUAV = nullptr;
}

Engine::Texture3D::Texture3D(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myTexture = nullptr;
	mySRV = nullptr;
	myUAV = nullptr;
}

Engine::Texture3D::Texture3D(const Texture3D& aCopy)
{
	myDevice = aCopy.myDevice;
	myContext = aCopy.myContext;
	myTexture = aCopy.myTexture;
	mySRV = aCopy.mySRV;
	myResolution = aCopy.myResolution;
	myUAV = aCopy.myUAV;
}


bool Engine::Texture3D::Init(const v3ui aResolution, DXGI_FORMAT aFormat, ETextureUsageFlags aTextureFlag, bool aCpuAccess, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aCPUWrite, bool aUAV)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;
	D3D11_TEXTURE3D_DESC desc = { 0 };
	desc.Width = aResolution.x;
	desc.Height = aResolution.y;
	desc.Depth = aResolution.z;
	desc.MipLevels = 1;
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
	if (aUAV)
	{
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}
	desc.MiscFlags = 0;

	ID3D11Texture3D* texture;
	result = aDevice->CreateTexture3D(&desc, nullptr, &texture);
	assert(SUCCEEDED(result));

	ID3D11ShaderResourceView* SRV;
	result = aDevice->CreateShaderResourceView(texture, nullptr, &SRV);
	assert(SUCCEEDED(result));

	if (aUAV)
	{
		result = aDevice->CreateUnorderedAccessView(texture, NULL, &myUAV);
		assert(SUCCEEDED(result));
	}

	SetResolution(aResolution);
	SetTexture(texture);
	SetSRV(SRV);

	return true;
}

void Engine::Texture3D::SetAsResourceOnSlot(const unsigned int aSlot)
{
	myContext->PSSetShaderResources(aSlot, 1, &mySRV);
}

void Engine::Texture3D::SetSRV(ID3D11ShaderResourceView* aSRV)
{
	mySRV = aSRV;
}

void Engine::Texture3D::SetTexture(ID3D11Texture3D* aTexture)
{
	myTexture = aTexture;
}

void Engine::Texture3D::ReleaseResources()
{
	SAFE_RELEASE(myTexture);
	SAFE_RELEASE(mySRV);
	SAFE_RELEASE(myUAV);
	myDevice = nullptr;
	myContext = nullptr;
}

void Engine::Texture3D::SetAsCSOutput(unsigned int aOutputSlot)
{
	if (myUAV)
	{
		myContext->CSSetUnorderedAccessViews(aOutputSlot, 1, &myUAV, 0);
	}
}

void Engine::Texture3D::SetResolution(const v3ui aResolution)
{
	myResolution = aResolution;
}
