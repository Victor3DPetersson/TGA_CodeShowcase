#include "stdafx.h"
#include "FullScreenTexture.h"
#include <d3d11.h>

unsigned int Engine::FullScreenTexture::myIDCounter = UINT_MAX;
Engine::FullScreenTexture::FullScreenTexture()
{
	myIDCounter++;
	myId = myIDCounter;
	myDevice = nullptr;
	myContext = nullptr;
	myTexture = nullptr;
	mySRV = nullptr;
	myViewPort = nullptr;
	myDepth = nullptr;
	myUAV = nullptr;
	myIsInited = false;
}

Engine::FullScreenTexture::FullScreenTexture(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myIDCounter++;
	myId = myIDCounter;
	myTexture = nullptr;
	mySRV = nullptr;
	myViewPort = nullptr;
	myDepth = nullptr;
	myRenderTarget = nullptr;
	myUAV = nullptr;
	myIsInited = false;
}

Engine::FullScreenTexture::FullScreenTexture(const FullScreenTexture& aCopy)
{
	myDevice = aCopy.myDevice;
	myContext = aCopy.myContext;

	myId = aCopy.myId;

	myTexture = aCopy.myTexture;
	mySRV = aCopy.mySRV;
	myViewPort = aCopy.myViewPort;
	myRenderTarget = aCopy.myRenderTarget;
	myDepth = aCopy.myDepth;
	myUAV = aCopy.myUAV;
	myIsInited = aCopy.myIsInited;
}

Engine::FullScreenTexture::~FullScreenTexture()
{
	assert(myIsInited != true && "Must Release Resources before deleting data");
	myTexture = nullptr;
	mySRV = nullptr;
	SAFE_DELETE(myViewPort);
	myDevice = nullptr;
	myContext = nullptr;
	myDepth = nullptr;
	myRenderTarget = nullptr;
	myUAV = nullptr;
}

void Engine::FullScreenTexture::Init()
{
	assert(myIsInited != true && "Must Release Resources before Initing again");
	myIsInited = true;
}

bool Engine::FullScreenTexture::operator==(const FullScreenTexture& aCopy)
{
	return (myId == aCopy.GetID());
}

void Engine::FullScreenTexture::ClearTexture(const CU::Color aClearColor)
{
	myContext->ClearRenderTargetView(myRenderTarget, &aClearColor.GetArray()[0]);
}

void Engine::FullScreenTexture::ClearDepth(float aClearDepthValue, unsigned int aClearStencilValue)
{
	myContext->ClearDepthStencilView(myDepth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, (UINT8)aClearDepthValue, (UINT8)aClearStencilValue);
}

void Engine::FullScreenTexture::SetAsActiveTarget(FullScreenTexture* aDepth)
{
	if (aDepth)
	{
		myContext->OMSetRenderTargets(1, &myRenderTarget, aDepth->myDepth);
	}
	else
	{
		myContext->OMSetRenderTargets(1, &myRenderTarget, nullptr);
	}
	myContext->RSSetViewports(1, myViewPort);
}


void Engine::FullScreenTexture::SetAsResourceOnSlot(const unsigned int aSlot)
{
	myContext->PSSetShaderResources(aSlot, 1, &mySRV);
}

void Engine::FullScreenTexture::SetAsCSResourceOnSlot(const unsigned int aSlot)
{
	myContext->CSSetShaderResources(aSlot, 1, &mySRV);
}

void Engine::FullScreenTexture::SetAsActiveDepth()
{
	ID3D11RenderTargetView* nullRenderTarget = { nullptr };
	myContext->OMSetRenderTargets(0, &nullRenderTarget, myDepth);
	myContext->RSSetViewports(1, myViewPort);
}

void Engine::FullScreenTexture::SetSRV(ID3D11ShaderResourceView* aSRV)
{
	mySRV = aSRV;
}

void Engine::FullScreenTexture::SetTexture(ID3D11Texture2D* aTexture)
{
	myTexture = aTexture;
}

void Engine::FullScreenTexture::SetViewPort(D3D11_VIEWPORT* aViewPort)
{
	myViewPort = aViewPort;
}

void Engine::FullScreenTexture::SetRenderTarget(ID3D11RenderTargetView* aRenderTargetView)
{
	myRenderTarget = aRenderTargetView;
}

void Engine::FullScreenTexture::SetDepthStencil(ID3D11DepthStencilView* aDepthStencilView)
{
	myDepth = aDepthStencilView;
}

void Engine::FullScreenTexture::SetUAV(ID3D11UnorderedAccessView* aUAV)
{
	myUAV = aUAV;
}

void Engine::FullScreenTexture::ReleaseResources()
{
	SAFE_RELEASE(myTexture);
	SAFE_RELEASE(mySRV);
	SAFE_RELEASE(myDepth);
	SAFE_RELEASE(myRenderTarget);
	SAFE_RELEASE(myUAV);
	myIsInited = false;
}

void Engine::FullScreenTexture::SetAsCSOutput(unsigned int aOutputSlot)
{
	if (myUAV)
	{
		myContext->CSSetUnorderedAccessViews(aOutputSlot, 1, &myUAV, 0);
	}
}

void Engine::FullScreenTexture::SetResolution(const v2ui aResolution)
{
	myResolution = aResolution;
}

unsigned int Engine::FullScreenTextureArray::myArrayIDCounter = UINT_MAX;
Engine::FullScreenTextureArray::FullScreenTextureArray()
{
	myArrayIDCounter++;
	myId = myArrayIDCounter;
	myDevice = nullptr;
	myContext = nullptr;
	myTexture = nullptr;
	mySRV = nullptr;
	myViewPort = nullptr;
	myUAV = nullptr;
	myRenderTargets = nullptr;
	myArrayLength = 0;
	myIsInited = false;
}

Engine::FullScreenTextureArray::FullScreenTextureArray(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, unsigned int aArrayLength) : myArrayLength(aArrayLength)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myArrayIDCounter++;
	myId = myArrayIDCounter;
	myTexture = nullptr;
	mySRV = nullptr;
	myViewPort = nullptr;
	myRenderTargets = nullptr;
	myUAV = nullptr;
	myIsInited = false;
}

Engine::FullScreenTextureArray::FullScreenTextureArray(const FullScreenTextureArray& aCopy)
{
	myDevice = aCopy.myDevice;
	myContext = aCopy.myContext;

	myId = aCopy.myId;

	myTexture = aCopy.myTexture;
	mySRV = aCopy.mySRV;
	myViewPort = aCopy.myViewPort;
	myRenderTargets = aCopy.myRenderTargets;
	myUAV = aCopy.myUAV;
	myArrayLength = aCopy.myArrayLength;
	myIsInited = aCopy.myArrayLength;
}

Engine::FullScreenTextureArray::~FullScreenTextureArray()
{
	//ReleaseResources();
	assert(myIsInited != true && "Must Release Resources before deleting data");
	myTexture = nullptr;
	mySRV = nullptr;
	SAFE_DELETE(myViewPort);
	myDevice = nullptr;
	myContext = nullptr;
	SAFE_DELETE(myRenderTargets);
	myUAV = nullptr;
}

void Engine::FullScreenTextureArray::Init()
{
	assert(myIsInited != true && "Must Release Resources before Initing again");
	myIsInited = true;
}

void Engine::FullScreenTextureArray::InitRTVs()
{
	if (myArrayLength > 0 && myRenderTargets == nullptr)
	{
		myRenderTargets = new ID3D11RenderTargetView * [myArrayLength];
	}
}

bool Engine::FullScreenTextureArray::operator==(const FullScreenTextureArray& aCopy)
{
	return (myId == aCopy.GetID());
}

void Engine::FullScreenTextureArray::ClearTexture(const CU::Color aClearColor)
{
	for (unsigned int RTVs = 0; RTVs < myArrayLength; RTVs++)
	{
		myContext->ClearRenderTargetView(myRenderTargets[RTVs], &aClearColor.GetArray()[0]);
	}
}


void Engine::FullScreenTextureArray::SetAsActiveTargets(FullScreenTexture* aDepth)
{
	if (aDepth)
	{
		myContext->OMSetRenderTargets(myArrayLength, &myRenderTargets[0], aDepth->myDepth);
	}
	else
	{
		myContext->OMSetRenderTargets(myArrayLength, &myRenderTargets[0], nullptr);
	}
	myContext->RSSetViewports(1, myViewPort);
}


void Engine::FullScreenTextureArray::SetAsResourceOnSlot(const unsigned int aSlot)
{
	myContext->PSSetShaderResources(aSlot, 1, &mySRV);
}


void Engine::FullScreenTextureArray::SetAsCSResourceOnSlot(const unsigned int aSlot)
{
	myContext->CSSetShaderResources(aSlot, 1, &mySRV);
}

void Engine::FullScreenTextureArray::SetSRV(ID3D11ShaderResourceView* aSRV)
{
	mySRV = aSRV;
}

void Engine::FullScreenTextureArray::SetTexture(ID3D11Texture2D* aTexture)
{
	myTexture = aTexture;
}

void Engine::FullScreenTextureArray::SetViewPort(D3D11_VIEWPORT* aViewPort)
{
	myViewPort = aViewPort;
}

void Engine::FullScreenTextureArray::SetRenderTarget(ID3D11RenderTargetView* aRenderTargetView, unsigned int aSlot)
{
	assert(myArrayLength != 0 && "Can not Set Render target on an uninitialized array");
	assert(myArrayLength >= aSlot && "aSlot is out of range");
	if (myArrayLength == 0 || myArrayLength <= aSlot) { return; }

	myRenderTargets[aSlot] = aRenderTargetView;
}


void Engine::FullScreenTextureArray::SetUAV(ID3D11UnorderedAccessView* aUAV)
{
	myUAV = aUAV;
}

void Engine::FullScreenTextureArray::ReleaseResources()
{
	SAFE_RELEASE(myTexture);
	SAFE_RELEASE(mySRV);
	SAFE_RELEASE(myUAV);
	if (myRenderTargets)
	{
		for (unsigned int rtv = 0; rtv < myArrayLength; rtv++)
		{
			SAFE_RELEASE(myRenderTargets[rtv]);
		}
		delete[] myRenderTargets;
		myRenderTargets = nullptr;
	}
	myIsInited = false;
}

void Engine::FullScreenTextureArray::SetAsCSOutput(unsigned int aOutputSlot)
{
	if (myUAV)
	{
		myContext->CSSetUnorderedAccessViews(aOutputSlot, 1, &myUAV, 0);
	}
}

void Engine::FullScreenTextureArray::SetResolution(const v2ui aResolution)
{
	myResolution = aResolution;
}
