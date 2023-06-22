#include "stdafx.h"
#include "GBuffer.h"
#include <d3d11.h>
#include "FullScreenTexture.h"

Engine::GBuffer::GBuffer()
{
	myDevice = nullptr;
	myContext = nullptr;
	myViewport = nullptr;
	for (unsigned int i = 0; i < EGBufferTexture::COUNT; i++)
	{
		myTextures[i] = nullptr;
		myRTVs[i] = nullptr;
		mySRVs[i] = nullptr;
	}
	myIsInited = false;
}

Engine::GBuffer::GBuffer(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myViewport = nullptr;
	for (unsigned int i = 0; i < EGBufferTexture::COUNT; i++)
	{
		myTextures[i] = nullptr;
		myRTVs[i] = nullptr;
		mySRVs[i] = nullptr;
	}
	myIsInited = false;
}

Engine::GBuffer::~GBuffer()
{
	assert(myIsInited != true && "Must Release resources before Deleting GBuffer");
	myDevice = nullptr;
	myContext = nullptr;
	myViewport = nullptr;
	for (unsigned int i = 0; i < EGBufferTexture::COUNT; i++)
	{
		SAFE_RELEASE(myTextures[i]);
		SAFE_RELEASE(myRTVs[i]);
		SAFE_RELEASE(mySRVs[i]);
	}
}

void Engine::GBuffer::Init()
{
	myIsInited = true;
}

void Engine::GBuffer::ClearTextures(CU::Color aColor)
{
	for (unsigned int i = 0; i < EGBufferTexture::COUNT; i++)
	{
		if (i == EGBufferTexture::DEPTH)
		{
			ClearTexture(i, {255, 255, 255, 255});
		}
		else
		{
			ClearTexture(i, aColor);
		}
	}
}

void Engine::GBuffer::SetAsActiveTarget(FullScreenTexture* aDepth)
{
	if (aDepth)
	{
		myContext->OMSetRenderTargets(EGBufferTexture::COUNT, &myRTVs[0], aDepth->myDepth);
	}
	else
	{
		myContext->OMSetRenderTargets(EGBufferTexture::COUNT, &myRTVs[0], nullptr);
	}
	myContext->RSSetViewports(1, myViewport);
}

void Engine::GBuffer::SetAsActiveTarget(EGBufferTexture aTextureToExclude, FullScreenTexture* aDepth)
{
	if (aDepth)
	{
		myContext->OMSetRenderTargets(aTextureToExclude, &myRTVs[0], aDepth->myDepth);
	}
	else
	{
		myContext->OMSetRenderTargets(aTextureToExclude, &myRTVs[0], nullptr);
	}
	myContext->RSSetViewports(1, myViewport);
}

void Engine::GBuffer::SetDecalTargetsAsTarget(FullScreenTexture* aDepth)
{
	ID3D11RenderTargetView* viewsToMap[4]{ myRTVs[EGBufferTexture::ALBEDO_AO], myRTVs[EGBufferTexture::NORMAL_METAL], myRTVs[EGBufferTexture::VERTEXNORMAL_ROUGHNESS], myRTVs[EGBufferTexture::EMISSIVE] };
	if (aDepth)
	{
		myContext->OMSetRenderTargets(3, &viewsToMap[0], aDepth->myDepth);
	}
	else
	{
		myContext->OMSetRenderTargets(3, &viewsToMap[0], nullptr);
	}
	myContext->RSSetViewports(1, myViewport);
}

void Engine::GBuffer::SetAsResourceOnSlot(EGBufferTexture aTexture, unsigned int aSlot)
{
	myContext->PSSetShaderResources(aSlot, 1, &mySRVs[aTexture]);
}

void Engine::GBuffer::SetAllAsResources()
{
	myContext->PSSetShaderResources(21, EGBufferTexture::COUNT, &mySRVs[0]);
}



void Engine::GBuffer::ReleaseResources()
{
	for (unsigned short i = 0; i < EGBufferTexture::COUNT; i++)
	{
		SAFE_RELEASE(myTextures[i]);
		SAFE_RELEASE(myRTVs[i]);
		SAFE_RELEASE(myTextures[i]);
	}
	SAFE_DELETE(myViewport);
	myIsInited = false;
}

void Engine::GBuffer::SetResolution(const v2ui aResolution)
{
	myResolution = aResolution;
}

void Engine::GBuffer::ClearTexture(const unsigned int aTextureSlot, const CU::Color aClearColor)
{
	myContext->ClearRenderTargetView(myRTVs[aTextureSlot], &aClearColor.GetArray()[0]);
}