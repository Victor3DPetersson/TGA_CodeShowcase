#include "stdafx.h"
#include "FullScreenTexture_Factory.h"
#include <d3d11.h>
#include "FullScreenTexture.h"
#include "GBuffer.h"
Engine::FullScreenTexture* Engine::CreateDepthTexture(const CU::Vector2ui aSize, DXGI_FORMAT aDepthFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, EDepthStencilSRV aSRVCreationFlag, EDepthStencilFlag aStencilCreationMode)
{
	HRESULT result;
	D3D11_TEXTURE2D_DESC texDesc = { 0 };
	texDesc.Width = aSize.x;
	texDesc.Height = aSize.y;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;

	switch (aStencilCreationMode)
	{
	case Engine::EDepthStencilFlag::BOTH:
		texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case Engine::EDepthStencilFlag::DEPTH_ONLY:
		texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		break;
	case Engine::EDepthStencilFlag::STENCIL_ONLY:
		texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		break;
	default:
		break;
	}
	if (aSRVCreationFlag == EDepthStencilSRV::CREATE)
	{
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	}
	else
	{
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	}
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* texture;
	result = aDevice->CreateTexture2D(&texDesc, nullptr, &texture);
	assert(SUCCEEDED(result));


	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = 0;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	

	switch (aStencilCreationMode)
	{
	case Engine::EDepthStencilFlag::BOTH:
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	case Engine::EDepthStencilFlag::DEPTH_ONLY:
		dsvDesc.Format = aDepthFormat;
		break;
	case Engine::EDepthStencilFlag::STENCIL_ONLY:
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		break;
	}
	ID3D11DepthStencilView* DSV;
	result = aDevice->CreateDepthStencilView(texture, &dsvDesc, &DSV);
	assert(SUCCEEDED(result));



	ID3D11ShaderResourceView* SRV = nullptr;
	if (aSRVCreationFlag == EDepthStencilSRV::CREATE)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		switch (aStencilCreationMode)
		{
		case Engine::EDepthStencilFlag::BOTH:
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		case Engine::EDepthStencilFlag::DEPTH_ONLY:
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			break;
		case Engine::EDepthStencilFlag::STENCIL_ONLY:
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		default:
			break;
		}
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
		result = aDevice->CreateShaderResourceView(texture, &srvDesc, &SRV);
		assert(SUCCEEDED(result));
	}

	D3D11_VIEWPORT* viewport = new D3D11_VIEWPORT(
		{
			0,
			0,
			(float)aSize.x,
			(float)aSize.y,
			0,
			1
		});

	FullScreenTexture* textureResult = new FullScreenTexture(aDevice, aDeviceContext);
	textureResult->SetTexture(texture);
	textureResult->SetViewPort(viewport);
	textureResult->SetDepthStencil(DSV);
	textureResult->SetResolution(aSize);
	if (aSRVCreationFlag == EDepthStencilSRV::CREATE)
	{
		textureResult->SetSRV(SRV);
	}
	textureResult->Init();
	return textureResult;
}

Engine::FullScreenTexture* Engine::CreateFullScreenTexture(const CU::Vector2ui aSize, DXGI_FORMAT aFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aAllowCPUAcess, bool aCPUWrite, bool aCreateRenderTarget, bool aCreateUAV)
{
	HRESULT result;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aSize.x;
	desc.Height = aSize.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = aFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (aAllowCPUAcess)
	{
		if (aCPUWrite)
		{
			aCreateRenderTarget = false;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
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
	if (aCreateUAV)
	{
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}
	desc.MiscFlags = 0;

	ID3D11Texture2D* texture;
	result = aDevice->CreateTexture2D(&desc, nullptr, &texture);
	assert(SUCCEEDED(result));

	FullScreenTexture* textureResult = CreateFullScreenTextureFromTexture(texture, aDevice, aDeviceContext, aCreateRenderTarget);

	ID3D11ShaderResourceView* SRV = nullptr;
	result = aDevice->CreateShaderResourceView(texture, nullptr, &SRV);
	assert(SUCCEEDED(result));
	textureResult->SetSRV(SRV); 
	ID3D11UnorderedAccessView* UAV = nullptr;
	if (aCreateUAV)
	{
		result = aDevice->CreateUnorderedAccessView(texture, NULL, &UAV);
		assert(SUCCEEDED(result));
		textureResult->SetUAV(UAV);
	}
	textureResult->SetResolution(aSize);
	textureResult->Init();

	return textureResult;
}
Engine::FullScreenTexture* Engine::CreateFullScreenTextureWithMips(const CU::Vector2ui aSize, DXGI_FORMAT aFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aAllowCPUAcess, bool aCPUWrite, bool aCreateRenderTarget, bool aCreateUAV)
{
	HRESULT result;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aSize.x;
	desc.Height = aSize.y;
	desc.MipLevels = 0;
	desc.ArraySize = 1;
	desc.Format = aFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (aAllowCPUAcess)
	{
		if (aCPUWrite)
		{
			aCreateRenderTarget = false;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
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
	if (aCreateUAV)
	{
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}
	desc.MiscFlags = 0;

	ID3D11Texture2D* texture;
	result = aDevice->CreateTexture2D(&desc, nullptr, &texture);
	assert(SUCCEEDED(result));

	FullScreenTexture* textureResult = CreateFullScreenTextureFromTexture(texture, aDevice, aDeviceContext, aCreateRenderTarget);

	ID3D11ShaderResourceView* SRV = nullptr;
	result = aDevice->CreateShaderResourceView(texture, nullptr, &SRV);
	assert(SUCCEEDED(result));
	textureResult->SetSRV(SRV);
	ID3D11UnorderedAccessView* UAV = nullptr;
	if (aCreateUAV)
	{
		result = aDevice->CreateUnorderedAccessView(texture, NULL, &UAV);
		assert(SUCCEEDED(result));
		textureResult->SetUAV(UAV);
	}
	textureResult->SetResolution(aSize);
	textureResult->Init();

	return textureResult;
}
Engine::FullScreenTextureArray* Engine::CreateFullScreenTextureArray(const CU::Vector2ui aSize, DXGI_FORMAT aFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, unsigned int aMipLevel, unsigned int aArrayLength, bool aAllowCPUAcess, bool aCPUWrite, bool aCreateRenderTarget, bool aCreateUAV)
{
	HRESULT result;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aSize.x;
	desc.Height = aSize.y;
	desc.MipLevels = aMipLevel;
	desc.ArraySize = aArrayLength;
	desc.Format = aFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (aAllowCPUAcess)
	{
		if (aCPUWrite)
		{
			aCreateRenderTarget = false;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
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
	if (aCreateUAV)
	{
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}
	desc.MiscFlags = 0;

	ID3D11Texture2D* texture;
	result = aDevice->CreateTexture2D(&desc, nullptr, &texture);
	assert(SUCCEEDED(result));

	ID3D11RenderTargetView** RTV = nullptr;
	D3D11_VIEWPORT* viewPort = nullptr;
	if (aCreateRenderTarget)
	{
		RTV = new ID3D11RenderTargetView* [aArrayLength];

		for (unsigned int RTVs = 0; RTVs < aArrayLength; RTVs++)
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.Format = aFormat;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.ArraySize = 1;
			rtvDesc.Texture2DArray.FirstArraySlice = RTVs;
			rtvDesc.Texture2DArray.MipSlice = 0;
			result = aDevice->CreateRenderTargetView(
				texture,
				&rtvDesc,
				&RTV[RTVs]);
			assert(SUCCEEDED(result));
		}
		viewPort = new D3D11_VIEWPORT(
			{
				0,
				0,
				(float)desc.Width,
				(float)desc.Height,
				0,
				1
			});
	}

	v2ui resolution;
	resolution.x = desc.Width;
	resolution.y = desc.Height;

	FullScreenTextureArray* textureResult = new FullScreenTextureArray(aDevice, aDeviceContext, aArrayLength);
	textureResult->SetTexture(texture);
	if (aCreateRenderTarget)
	{
		textureResult->InitRTVs();
		for (unsigned int RTVs = 0; RTVs < aArrayLength; RTVs++)
		{
			textureResult->SetRenderTarget(RTV[RTVs], RTVs);
		}
		delete[]RTV;
		RTV = nullptr;
		textureResult->SetViewPort(viewPort);
	}
	textureResult->SetResolution(resolution);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = desc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	D3D11_TEX2D_ARRAY_SRV arraySRVDesc;
	arraySRVDesc.ArraySize = desc.ArraySize;
	arraySRVDesc.MipLevels = desc.MipLevels;
	arraySRVDesc.FirstArraySlice = 0;
	arraySRVDesc.MostDetailedMip = 0;
	SRVDesc.Texture2DArray = arraySRVDesc;
	ID3D11ShaderResourceView* SRV = nullptr;
	result = aDevice->CreateShaderResourceView(texture, &SRVDesc, &SRV);
	assert(SUCCEEDED(result));
	textureResult->SetSRV(SRV);
	ID3D11UnorderedAccessView* UAV = nullptr;

	if (aCreateUAV)
	{
		D3D11_TEX2D_ARRAY_UAV arrayUAVDesc = {};
		arrayUAVDesc.ArraySize = desc.ArraySize;
		arrayUAVDesc.FirstArraySlice = 0;
		arrayUAVDesc.MipSlice = 0;
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.Texture2DArray.MipSlice = 0;
		UAVDesc.Texture2DArray = arrayUAVDesc;
		UAVDesc.Format = desc.Format;
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		result = aDevice->CreateUnorderedAccessView(texture, &UAVDesc, &UAV);
		assert(SUCCEEDED(result));
		textureResult->SetUAV(UAV);
	}
	textureResult->SetResolution(aSize);
	textureResult->Init();

	return textureResult;
}

Engine::FullScreenTexture* Engine::CreateFullScreenTextureFromTexture(ID3D11Texture2D* aTextureTemplate, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aCreateRenderTarget)
{
	HRESULT result;
	ID3D11RenderTargetView* RTV = nullptr;
	if (aCreateRenderTarget)
	{
		result = aDevice->CreateRenderTargetView(
			aTextureTemplate,
			nullptr,
			&RTV);
		assert(SUCCEEDED(result));
	}

	D3D11_VIEWPORT* viewport = nullptr;
	v2ui resolution;
	if (aTextureTemplate)
	{
		D3D11_TEXTURE2D_DESC desc;
		aTextureTemplate->GetDesc(&desc);
		viewport = new D3D11_VIEWPORT(
			{
				0,
				0,
				static_cast<float>(desc.Width),
				static_cast<float>(desc.Height),
				0,
				1
			}
		);
		resolution.x = desc.Width;
		resolution.y = desc.Height;
	}

	FullScreenTexture* textureResult = new FullScreenTexture(aDevice, aDeviceContext);
	textureResult->SetTexture(aTextureTemplate);
	if (aCreateRenderTarget)
	{
		textureResult->SetRenderTarget(RTV);
	}
	textureResult->SetViewPort(viewport);
	textureResult->SetResolution(resolution);
	return textureResult;
}

Engine::GBuffer* Engine::CreateGBuffer(const CU::Vector2ui aSize, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	HRESULT result;

	DXGI_FORMAT textureFormats[EGBufferTexture::COUNT]
	{
		DXGI_FORMAT_R32G32B32A32_FLOAT, // Position XYZ DetailNormal Strength W
		DXGI_FORMAT_R8G8B8A8_UNORM, // Albedo RGB : AO W
		DXGI_FORMAT_R16G16B16A16_SNORM, //Normal XYZ : Metalness W
		DXGI_FORMAT_R16G16B16A16_SNORM, //VertexNormal XYZ : Roughness W
		DXGI_FORMAT_R8G8B8A8_UNORM, //EMISSIVE RGB : EMISSIVE STRENGTH W
		DXGI_FORMAT_R32_FLOAT, //Depth
	};

	GBuffer* returnGBuffer = new GBuffer(aDevice, aDeviceContext);

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = aSize.x;
	desc.Height = aSize.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	for (unsigned int i = 0; i < EGBufferTexture::COUNT; i++)
	{
		desc.Format = textureFormats[i];
		result = aDevice->CreateTexture2D(&desc, nullptr, returnGBuffer->GetTexture(i));
		assert(SUCCEEDED(result));

		result = aDevice->CreateRenderTargetView(
			*returnGBuffer->GetTexture(i),
			nullptr,
			returnGBuffer->GetRTV(i)
		);
		assert(SUCCEEDED(result));

		result = aDevice->CreateShaderResourceView(
			*returnGBuffer->GetTexture(i),
			nullptr,
			returnGBuffer->GetSRV(i)
		);
		assert(SUCCEEDED(result));
	}
	D3D11_VIEWPORT * viewPort = new D3D11_VIEWPORT(
		{
			0,
			0,
			(float)desc.Width,
			(float)desc.Height,
			0,
			1
		});
	returnGBuffer->SetViewPort(viewPort);
	returnGBuffer->SetResolution(aSize);

	return returnGBuffer;
}

Engine::FullScreenTexture* Engine::CreateShadowMap(const CU::Vector2ui aSize, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	HRESULT result;

	D3D11_TEXTURE2D_DESC textureDesc = { 0 };
	textureDesc.Width = aSize.x;
	textureDesc.Height = aSize.y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	ID3D11Texture2D* texture;
	result = aDevice->CreateTexture2D(&textureDesc, nullptr, &texture);
	assert(SUCCEEDED(result));

	D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc = {};
	depthDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	depthDesc.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;

	ID3D11DepthStencilView* depth;
	aDevice->CreateDepthStencilView(texture, &depthDesc, &depth);
	assert(SUCCEEDED(result));

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = {};
	resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
	resourceDesc.Texture2D.MipLevels = textureDesc.MipLevels;

	ID3D11ShaderResourceView* shaderResource;
	aDevice->CreateShaderResourceView(texture, &resourceDesc, &shaderResource);
	assert(SUCCEEDED(result));

	D3D11_VIEWPORT* viewport = new D3D11_VIEWPORT(
		{
			0.0f, 0.0f,
			(float)aSize.x, 
			(float)aSize.y,
			0.0f, 1.0f
		});
	FullScreenTexture* returnTex = new FullScreenTexture(aDevice, aDeviceContext);
	returnTex->SetTexture(texture);
	returnTex->SetDepthStencil(depth);
	returnTex->SetSRV(shaderResource);
	returnTex->SetViewPort(viewport);
	returnTex->Init();
	return returnTex;
}




