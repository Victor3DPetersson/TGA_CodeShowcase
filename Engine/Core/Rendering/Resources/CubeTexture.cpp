#include "stdafx.h"
#include "CubeTexture.h"
#include <d3d11.h>

Engine::CubeTexture::CubeTexture()
{
	myDevice = nullptr;
	myContext = nullptr;
	myTexture = nullptr;
	mySRV = nullptr;
	myRTV = nullptr;
}

bool Engine::CubeTexture::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aTextureResolution.x;
	desc.Height = aTextureResolution.y;
	desc.MipLevels = 1;
	desc.ArraySize = 6;
	desc.Format = aFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	bool createRenderTarget = false;
	if (aCpuAccess)
	{
		if (aCPUWrite)
		{
			createRenderTarget = false;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		}
	}
	else
	{
		desc.CPUAccessFlags = 0;
	}
	
	result = myDevice->CreateTexture2D(&desc, nullptr, &myTexture);
	assert(SUCCEEDED(result));

	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = desc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBE;
	SMViewDesc.TextureCube.MipLevels = desc.MipLevels;
	SMViewDesc.TextureCube.MostDetailedMip = 0;

	result = myDevice->CreateShaderResourceView(myTexture, &SMViewDesc, &mySRV);
	assert(SUCCEEDED(result));

	if (createRenderTarget)
	{
		result = aDevice->CreateRenderTargetView(
			myTexture,
			nullptr,
			&myRTV);
		assert(SUCCEEDED(result));
	}


	return true;
}

bool Engine::CubeTexture::InitWithFullMip(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aTextureResolution.x;
	desc.Height = aTextureResolution.y;
	desc.MipLevels = 0;
	desc.ArraySize = 6;
	desc.Format = aFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	bool createRenderTarget = true;
	if (aCpuAccess)
	{
		if (aCPUWrite)
		{
			createRenderTarget = false;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		}
	}
	else
	{
		desc.CPUAccessFlags = 0;
	}
	result = myDevice->CreateTexture2D(&desc, nullptr, &myTexture);
	assert(SUCCEEDED(result));

	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = desc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBE;
	SMViewDesc.TextureCube.MipLevels = (unsigned int) - 1;
	SMViewDesc.TextureCube.MostDetailedMip = 0;

	result = myDevice->CreateShaderResourceView(myTexture, &SMViewDesc, &mySRV);
	assert(SUCCEEDED(result));

	if (createRenderTarget)
	{
		result = aDevice->CreateRenderTargetView(
			myTexture,
			nullptr,
			&myRTV);
		assert(SUCCEEDED(result));
	}
	return true;
}

void Engine::CubeTexture::ReleaseResources()
{
	SAFE_RELEASE(myTexture);
	SAFE_RELEASE(mySRV);
	SAFE_RELEASE(myRTV);
	myDevice = nullptr;
	myContext = nullptr;
	myTexture = nullptr;
	mySRV = nullptr;
	myRTV = nullptr;
}

void Engine::CubeTexture::SetAsResourceOnSlot(const unsigned int aSlot)
{
	myContext->PSSetShaderResources(aSlot, 1, &mySRV);
}

void Engine::CubeTexture::SetAsCSResourceOnSlot(const unsigned int aSlot)
{
	myContext->CSSetShaderResources(aSlot, 1, &mySRV);
}

Engine::CubeTextureArray::CubeTextureArray()
{
	myDevice = nullptr;
	myContext = nullptr;
	myTexture = nullptr;
	mySRV = nullptr;
	//myDepth = nullptr;
	myRTV = nullptr;
}

bool Engine::CubeTextureArray::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite, unsigned int aArrayLength)
{
	if (myIsInited)
	{
		return false;
	}
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aTextureResolution.x;
	desc.Height = aTextureResolution.y;
	desc.MipLevels = 1;
	desc.ArraySize = 6 * aArrayLength;
	desc.Format = aFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	bool createRenderTarget = false;
	if (aCpuAccess)
	{
		if (aCPUWrite)
		{
			createRenderTarget = false;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		}
	}
	else
	{
		desc.CPUAccessFlags = 0;
	}

	result = myDevice->CreateTexture2D(&desc, nullptr, &myTexture);
	assert(SUCCEEDED(result));

	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = desc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	SMViewDesc.TextureCubeArray.MipLevels = desc.MipLevels;
	SMViewDesc.TextureCubeArray.MostDetailedMip = 0;
	SMViewDesc.TextureCubeArray.NumCubes = aArrayLength;
	result = myDevice->CreateShaderResourceView(myTexture, &SMViewDesc, &mySRV);
	assert(SUCCEEDED(result));

	if (createRenderTarget)
	{
		result = aDevice->CreateRenderTargetView(
			myTexture,
			nullptr,
			&myRTV);
		assert(SUCCEEDED(result));
	}

	myIsInited = true;
	return true;
}

bool Engine::CubeTextureArray::InitWithFullMip(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, const v2ui& aTextureResolution, DXGI_FORMAT aFormat, bool aCpuAccess, bool aCPUWrite, unsigned int aArrayLength)
{
	if (myIsInited)
	{
		return false;
	}
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aTextureResolution.x;
	desc.Height = aTextureResolution.y;
	desc.MipLevels = 0;
	desc.ArraySize = 6 * aArrayLength;
	desc.Format = aFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	bool createRenderTarget = true;
	if (aCpuAccess)
	{
		if (aCPUWrite)
		{
			createRenderTarget = false;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		}
	}
	else
	{
		desc.CPUAccessFlags = 0;
	}
	result = myDevice->CreateTexture2D(&desc, nullptr, &myTexture);
	assert(SUCCEEDED(result));
	
	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = desc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	SMViewDesc.TextureCubeArray.MipLevels = (unsigned int)-1;
	SMViewDesc.TextureCubeArray.MostDetailedMip = 0;
	SMViewDesc.TextureCubeArray.NumCubes = aArrayLength;
	SMViewDesc.TextureCubeArray.First2DArrayFace = 0;
	result = myDevice->CreateShaderResourceView(myTexture, &SMViewDesc, &mySRV);
	assert(SUCCEEDED(result));

	if (createRenderTarget)
	{
		result = aDevice->CreateRenderTargetView(
			myTexture,
			nullptr,
			&myRTV);
		assert(SUCCEEDED(result));
	}
	myIsInited = true;
	return true;
}

void Engine::CubeTextureArray::ReleaseResources()
{
	if (myIsInited)
	{
		myTexture->Release();
		mySRV->Release();
		myRTV->Release();
	}
	myDevice = nullptr;
	myContext = nullptr;
	myTexture = nullptr;
	mySRV = nullptr;
	myRTV = nullptr;
	myIsInited = false;
}

void Engine::CubeTextureArray::SetAsResourceOnSlot(const unsigned int aSlot)
{
	myContext->PSSetShaderResources(aSlot, 1, &mySRV);
}

void Engine::CubeTextureArray::SetAsCSResourceOnSlot(const unsigned int aSlot)
{
	myContext->CSSetShaderResources(aSlot, 1, &mySRV);
}
