#pragma once
#include "stdafx.h"
#include "DirectXFramework.h"
#include "WindowHandler.h"
#include "CU/Math/Color.hpp"
#include "Includes.h"
#include <d3d11.h>
#include <assert.h>
#include <dxgi.h>


Engine::DirectXFramework::DirectXFramework()
{
	mySwapChain = nullptr;
	myDevice = nullptr;
	myDeviceContext = nullptr;
	myBackBuffer = nullptr;

	//myDepthBuffer = nullptr;
	//myRasterizerStates.mySolidNoCullRS	= nullptr;
	//myRasterizerStates.mySolidRS		= nullptr;
	//myRasterizerStates.myWireFrameRS	= nullptr;
}

void Engine::DirectXFramework::ShutDownDX()
{
	//SAFE_RELEASE(myRasterizerStates.myWireFrameRS);
	//SAFE_RELEASE(myRasterizerStates.mySolidRS);
	//SAFE_RELEASE(myRasterizerStates.mySolidNoCullRS);

	SAFE_RELEASE(mySwapChain);
	//SAFE_RELEASE(myDepthBuffer);
	SAFE_RELEASE(myBackBuffer);
	SAFE_RELEASE(myDeviceContext);
	SAFE_RELEASE(myDevice);
	myWindowHandler = nullptr;
}

bool Engine::DirectXFramework::Init(WindowHandler& aWindowHandler)
{
	v2ui numDenum;
	IDXGIAdapter* adapter = nullptr;
	CollectAdapters(aWindowHandler.GetWindowResolution(), numDenum, adapter);
	myWindowHandler = &aWindowHandler;
	HRESULT result;
	DXGI_SWAP_CHAIN_DESC swapchainDescription = {};
	swapchainDescription.BufferCount = 1;
	swapchainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	swapchainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDescription.OutputWindow = aWindowHandler.GetWindowHandle();
	swapchainDescription.SampleDesc.Count = 1;
	swapchainDescription.Windowed = true;

	result = D3D11CreateDeviceAndSwapChain(
		adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT || D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION,
		&swapchainDescription, &mySwapChain, &myDevice, nullptr, &myDeviceContext);
	//D3D11_CREATE_DEVICE_DEBUG
	if (FAILED(result))
	{
		assert(false && "Failed to create Device and Swap Chain");
		return false;
	}

	ID3D11Texture2D* backbufferTexture;
	result = mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbufferTexture);
	if (FAILED(result))
	{
		assert("Failed to Get Buffer");
		return false;
	}
	result = myDevice->CreateRenderTargetView(backbufferTexture, nullptr, &myBackBuffer);
	if (FAILED(result))
	{
		assert("Failed to create Render Target View");
		return false;
	}
	result = backbufferTexture->Release();
	if (FAILED(result))
	{
		assert("Failed to Release Backbuffer");
		return false;
	}

	ID3D11Texture2D* depthBufferTexture;
	D3D11_TEXTURE2D_DESC depthBufferDescription = { 0 };
	depthBufferDescription.Width = static_cast<unsigned int>(aWindowHandler.GetWindowResolution().x);
	depthBufferDescription.Height = static_cast<unsigned int>(aWindowHandler.GetWindowResolution().y);
	depthBufferDescription.ArraySize = 1;
	depthBufferDescription.Format = DXGI_FORMAT_D32_FLOAT;
	depthBufferDescription.SampleDesc.Count = 1;
	depthBufferDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	result = myDevice->CreateTexture2D(&depthBufferDescription, nullptr, &depthBufferTexture);
	if (FAILED(result))
	{
		assert(false == true && "Failed to Create DepthBuffer");
		return false;
	}

	/*if (!InitRasterizerStates())
	{
		assert("Failed to create RasterizerStates");
		return false;
	}*/
	return true;
}

void Engine::DirectXFramework::BeginFrame(CU::Color& aColor)
{
	myDeviceContext->ClearRenderTargetView(myBackBuffer, &aColor.GetArray()[0]);
}

void Engine::DirectXFramework::EndFrame()
{
	mySwapChain->Present(0, 0);
}

void Engine::DirectXFramework::OnResize(v2ui aWindowRes)
{
	HRESULT result;
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	myDeviceContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	myBackBuffer->Release();
	myDeviceContext->Flush();
	result = mySwapChain->ResizeBuffers(0, aWindowRes.x, aWindowRes.y, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result))
	{
		assert(false && "JESUS SMOKED CANNABUS!!!! REDO DX11");
	}

	ID3D11Texture2D* backbufferTexture = nullptr;
	result = mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbufferTexture);
	if (FAILED(result))
	{
		assert(false && "Failed to Get Buffer");
	}
	result = myDevice->CreateRenderTargetView(backbufferTexture, nullptr, &myBackBuffer);
	if (FAILED(result))
	{
		assert(false && "Failed to create Render Target View");
	}
	result = backbufferTexture->Release();
	if (FAILED(result))
	{
		assert(false && "Failed to Release Backbuffer");
	}
	D3D11_VIEWPORT vp;
	vp.Width = (float)aWindowRes.x;
	vp.Height = (float)aWindowRes.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	myDeviceContext->RSSetViewports(1, &vp);

}

v2ui Engine::DirectXFramework::GetWindowResolution()
{
	return myWindowHandler->GetWindowResolution();
}

//bool Engine::DirectXFramework::InitRasterizerStates()
//{
//	HRESULT result;
//	D3D11_RASTERIZER_DESC rasterizerDesc;
//	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
//	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
//	rasterizerDesc.CullMode = D3D11_CULL_BACK;
//	rasterizerDesc.DepthClipEnable = true;
//
//	result = myDevice->CreateRasterizerState(&rasterizerDesc, &myRasterizerStates.myWireFrameRS);
//	if (FAILED(result))
//	{
//		assert(false == true && "Failed to create RasterizerState");
//		return false;
//	}
//
//	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
//	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
//	rasterizerDesc.CullMode = D3D11_CULL_NONE;
//	rasterizerDesc.DepthClipEnable = true;
//
//	result = myDevice->CreateRasterizerState(&rasterizerDesc, &myRasterizerStates.mySolidNoCullRS);
//	if (FAILED(result))
//	{
//		assert(false == true && "Failed to create RasterizerState");
//		return false;
//	}
//
//	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
//	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
//	rasterizerDesc.CullMode = D3D11_CULL_BACK;
//	rasterizerDesc.DepthClipEnable = true;
//
//	result = myDevice->CreateRasterizerState(&rasterizerDesc, &myRasterizerStates.mySolidRS);
//	if (FAILED(result))
//	{
//		assert(false == true && "Failed to create RasterizerState");
//		return false;
//	}
//
//	return true;
//}

bool Engine::DirectXFramework::CollectAdapters(v2ui aWindowSize, v2ui& aNumDenumerator, IDXGIAdapter*& outAdapter)
{
	HRESULT result = S_OK;
	IDXGIFactory* factory;

	DXGI_MODE_DESC* displayModeList = nullptr;
	unsigned int numModes = 0;
	//unsigned int i = 0;
	unsigned int denominator = 0;
	unsigned int numerator = 0;
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}
	// Use the factory to create an adapter for the primary graphics interface (video card).
	IDXGIAdapter* usingAdapter = nullptr;
	int adapterIndex = 0;
	std::vector<DXGI_ADAPTER_DESC> myAdapterDescs;
	std::vector<IDXGIAdapter*> myAdapters;
	while (factory->EnumAdapters(adapterIndex, &usingAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		usingAdapter->GetDesc(&adapterDesc);
		myAdapterDescs.push_back(adapterDesc);
		myAdapters.push_back(usingAdapter);
		++adapterIndex;
	}

	if (adapterIndex == 0)
	{
		return false;
	}

	for (DXGI_ADAPTER_DESC desc : myAdapterDescs)
	{
		int memory = (int)(desc.DedicatedVideoMemory / 1024 / 1024);
		memory;
	}

	DXGI_ADAPTER_DESC usingAdapterDesc = myAdapterDescs[0];
	usingAdapter = myAdapters[0];


	const std::wstring nvidia = L"NVIDIA";
	const std::wstring ati = L"ATI";

	int memory = (int)(usingAdapterDesc.DedicatedVideoMemory / 1024 / 1024);
	int mostMem = 0;

	for (unsigned int i = 0; i < myAdapterDescs.size(); i++)
	{
		DXGI_ADAPTER_DESC desc = myAdapterDescs[i];
		memory = (int)(desc.DedicatedVideoMemory / 1024 / 1024);
		std::wstring name = desc.Description;
		if (name.find(nvidia) != std::wstring::npos || name.find(ati) != std::wstring::npos)
		{
			if (memory > mostMem)
			{
				mostMem = memory;
				usingAdapterDesc = desc;
				usingAdapter = myAdapters[i];
			}
		}
	}

	// Enumerate the primary adapter output (monitor).
	IDXGIOutput* pOutput = nullptr;
	if (usingAdapter->EnumOutputs(0, &pOutput) != DXGI_ERROR_NOT_FOUND)
	{
		// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
		result = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
		if (!FAILED(result))
		{
			// Create a list to hold all the possible display modes for this monitor/video card combination.
			displayModeList = new DXGI_MODE_DESC[numModes];
			if (displayModeList)
			{
				// Now fill the display mode list structures.
				result = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
				if (!FAILED(result))
				{
					// Now go through all the display modes and find the one that matches the screen width and height.
					// When a match is found store the numerator and denominator of the refresh rate for that monitor.
					for (unsigned int i = 0; i < numModes; i++)
					{
						if (displayModeList[i].Width == (unsigned int)aWindowSize.x)
						{
							if (displayModeList[i].Height == (unsigned int)aWindowSize.y)
							{
								numerator = displayModeList[i].RefreshRate.Numerator;
								denominator = displayModeList[i].RefreshRate.Denominator;
							}
						}
					}
				}
			}
		}
		// Release the adapter output.
		pOutput->Release();
		pOutput = 0;
	}


	// Get the adapter (video card) description.
	result = usingAdapter->GetDesc(&usingAdapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	myVideoCardMemory = (int)(usingAdapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;



	// Release the factory.
	factory->Release();
	factory = 0;

	if (myEnableVSync == true)
	{
		aNumDenumerator.x = numerator;
		aNumDenumerator.y = denominator;
	}
	else
	{
		aNumDenumerator.x = 0;
		aNumDenumerator.y = 1;
	}

	outAdapter = usingAdapter;
	return true;
}
