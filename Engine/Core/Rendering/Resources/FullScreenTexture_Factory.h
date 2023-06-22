#pragma once
#include "DX_Includes.h"
#include "GBuffer.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11DepthStencilState;
struct CD3D11_DEPTH_STENCIL_DESC;
namespace Engine
{
	class FullScreenTexture;
	class FullScreenTextureArray;
	class GBuffer;
	
	enum class EDepthStencilFlag
	{
		BOTH,
		DEPTH_ONLY,
		STENCIL_ONLY,
	};
	enum class EDepthStencilSRV
	{
		NONE,
		CREATE
	};
	
	FullScreenTexture* CreateDepthTexture(const CU::Vector2ui aSize, DXGI_FORMAT aDepthFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, EDepthStencilSRV aSRVCreationFlag = EDepthStencilSRV::NONE, EDepthStencilFlag aStencilCreationMode = EDepthStencilFlag::DEPTH_ONLY);
	FullScreenTexture* CreateFullScreenTexture(const CU::Vector2ui aSize, DXGI_FORMAT aFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aAllowCPUAcess = false, bool aCPUWrite = false, bool aCreateRenderTarget = true, bool aCreateUAV = false);
	FullScreenTexture* CreateFullScreenTextureWithMips(const CU::Vector2ui aSize, DXGI_FORMAT aFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool aAllowCPUAcess = false, bool aCPUWrite = false, bool aCreateRenderTarget = true, bool aCreateUAV = false);
	FullScreenTextureArray* CreateFullScreenTextureArray(const CU::Vector2ui aSize, DXGI_FORMAT aFormat, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, unsigned int aMipLevel, unsigned int aArrayLength, bool aAllowCPUAcess = false, bool aCPUWrite = false, bool aCreateRenderTarget = true, bool aCreateUAV = false);
	FullScreenTexture* CreateFullScreenTextureFromTexture(ID3D11Texture2D* aTextureTemplate, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, bool createRenderTarget);
	GBuffer* CreateGBuffer(const CU::Vector2ui aSize, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
	FullScreenTexture* CreateShadowMap(const CU::Vector2ui aSize, ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);

}

