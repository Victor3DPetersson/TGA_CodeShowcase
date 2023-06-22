#include "stdafx.h"
#include "RenderStates.h"
#include <d3d11.h>
bool Engine::InitRenderStates(ID3D11Device* aDevice, RenderStates& someStatesToInit)
{
	HRESULT res;

	D3D11_BLEND_DESC alphaBlendDesc = {};
	alphaBlendDesc.RenderTarget[0].BlendEnable = true;
	alphaBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	alphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	alphaBlendDesc.AlphaToCoverageEnable = true;

	res = aDevice->CreateBlendState(&alphaBlendDesc, &someStatesToInit.blendStates[BLENDSTATE_ALPHABLEND]);
	if (FAILED(res))
	{
		return false;
	}


	D3D11_BLEND_DESC alphaBlendDescNoCoverage = {};
	alphaBlendDescNoCoverage.RenderTarget[0].BlendEnable = true;
	alphaBlendDescNoCoverage.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescNoCoverage.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlendDescNoCoverage.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlendDescNoCoverage.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescNoCoverage.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescNoCoverage.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	alphaBlendDescNoCoverage.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	alphaBlendDescNoCoverage.AlphaToCoverageEnable = false;

	res = aDevice->CreateBlendState(&alphaBlendDescNoCoverage, &someStatesToInit.blendStates[BLENDSTATE_ALPHABLEND_NOCOVERAGE]);
	if (FAILED(res))
	{
		return false;
	}


	D3D11_BLEND_DESC additiveBlendDesc = {};
	additiveBlendDesc.RenderTarget[0].BlendEnable = true;
	additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	res = aDevice->CreateBlendState(&additiveBlendDesc, &someStatesToInit.blendStates[BLENDSTATE_ADDITIVE]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_BLEND_DESC alphaBlendDescDecal = {};
	alphaBlendDescDecal.RenderTarget[0].BlendEnable = true;
	alphaBlendDescDecal.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescDecal.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlendDescDecal.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlendDescDecal.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescDecal.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescDecal.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
	alphaBlendDescDecal.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	alphaBlendDescDecal.RenderTarget[1].BlendEnable = true;
	alphaBlendDescDecal.RenderTarget[1].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescDecal.RenderTarget[1].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlendDescDecal.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlendDescDecal.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescDecal.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescDecal.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	alphaBlendDescDecal.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	alphaBlendDescDecal.RenderTarget[2].BlendEnable = true;
	alphaBlendDescDecal.RenderTarget[2].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescDecal.RenderTarget[2].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlendDescDecal.RenderTarget[2].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlendDescDecal.RenderTarget[2].SrcBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescDecal.RenderTarget[2].DestBlendAlpha = D3D11_BLEND_ONE;
	alphaBlendDescDecal.RenderTarget[2].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	alphaBlendDescDecal.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	alphaBlendDescDecal.AlphaToCoverageEnable = false;

	res = aDevice->CreateBlendState(&alphaBlendDescDecal, &someStatesToInit.blendStates[BLENDSTATE_ALPHABLEND_DECALS]);
	if (FAILED(res))
	{
		return false;
	}

	someStatesToInit.blendStates[BLENDSTATE_DISABLE] = nullptr;


	D3D11_DEPTH_STENCIL_DESC readOnlyDepthDesc = {};
	readOnlyDepthDesc.DepthEnable = true;
	readOnlyDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	readOnlyDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	readOnlyDepthDesc.StencilEnable = false;

	res = aDevice->CreateDepthStencilState(&readOnlyDepthDesc, &someStatesToInit.depthStencilStates[DEPTHSTENCILSTATE_READONLY]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthOff = {};
	depthOff.DepthEnable = false;
	depthOff.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthOff.DepthFunc = D3D11_COMPARISON_NEVER;
	depthOff.StencilEnable = false;

	res = aDevice->CreateDepthStencilState(&depthOff, &someStatesToInit.depthStencilStates[DEPTHSTENCILSTATE_OFF]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthReadOnly_StencilWrite = {};
	depthReadOnly_StencilWrite.DepthEnable = true;
	depthReadOnly_StencilWrite.StencilEnable = true;
	depthReadOnly_StencilWrite.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthReadOnly_StencilWrite.DepthFunc = D3D11_COMPARISON_LESS;
	depthReadOnly_StencilWrite.StencilWriteMask = 0xFF;
	depthReadOnly_StencilWrite.BackFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER;
	depthReadOnly_StencilWrite.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWrite.BackFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWrite.BackFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWrite.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS;
	depthReadOnly_StencilWrite.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWrite.FrontFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWrite.FrontFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE;

	res = aDevice->CreateDepthStencilState(&depthReadOnly_StencilWrite, &someStatesToInit.depthStencilStates[DEPTHSTENCILSTATE_READONLY_STENCIL_WRITE]);
	if (FAILED(res))
	{
		return false;
	}
	D3D11_DEPTH_STENCIL_DESC depthReadOnly_StencilWriteDiscarded = {};
	depthReadOnly_StencilWriteDiscarded.DepthEnable = true;
	depthReadOnly_StencilWriteDiscarded.StencilEnable = true;
	depthReadOnly_StencilWriteDiscarded.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthReadOnly_StencilWriteDiscarded.DepthFunc = D3D11_COMPARISON_LESS;
	depthReadOnly_StencilWriteDiscarded.StencilWriteMask = 0xFF;
	depthReadOnly_StencilWriteDiscarded.BackFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER;
	depthReadOnly_StencilWriteDiscarded.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWriteDiscarded.BackFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWriteDiscarded.BackFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWriteDiscarded.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS;
	depthReadOnly_StencilWriteDiscarded.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE;
	depthReadOnly_StencilWriteDiscarded.FrontFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilWriteDiscarded.FrontFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE;

	res = aDevice->CreateDepthStencilState(&depthReadOnly_StencilWriteDiscarded, &someStatesToInit.depthStencilStates[DEPTHSTENCILSTATE_READONLY_STENCIL_WRITEDISCARDED]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthReadOnly_StencilRead = {};
	depthReadOnly_StencilRead.DepthEnable = true;
	depthReadOnly_StencilRead.StencilEnable = true;
	depthReadOnly_StencilRead.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthReadOnly_StencilRead.DepthFunc = D3D11_COMPARISON_LESS;
	depthReadOnly_StencilRead.StencilReadMask = 0xFF;
	depthReadOnly_StencilRead.BackFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER;
	depthReadOnly_StencilRead.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilRead.BackFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilRead.BackFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilRead.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NOT_EQUAL;
	depthReadOnly_StencilRead.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilRead.FrontFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	depthReadOnly_StencilRead.FrontFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;

	res = aDevice->CreateDepthStencilState(&depthReadOnly_StencilRead, &someStatesToInit.depthStencilStates[DEPTHSTENCILSTATE_READONLY_STENCIL_READ]);
	if (FAILED(res))
	{
		return false;
	}


	D3D11_DEPTH_STENCIL_DESC noDepth_StencilWrite = {};
	noDepth_StencilWrite.DepthEnable = false;
	noDepth_StencilWrite.StencilEnable = true;
	noDepth_StencilWrite.StencilWriteMask = 128;
	noDepth_StencilWrite.BackFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER;
	noDepth_StencilWrite.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilWrite.BackFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilWrite.BackFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilWrite.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS;
	noDepth_StencilWrite.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilWrite.FrontFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilWrite.FrontFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE;

	res = aDevice->CreateDepthStencilState(&noDepth_StencilWrite, &someStatesToInit.depthStencilStates[DEPTHSTENCILSTATE_STENCILONLY_WRITE]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC noDepth_StencilRead = {};
	noDepth_StencilRead.DepthEnable = false;
	noDepth_StencilRead.StencilEnable = true;
	noDepth_StencilRead.StencilReadMask = 0xFF;
	noDepth_StencilRead.BackFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER;
	noDepth_StencilRead.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilRead.BackFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilRead.BackFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilRead.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NOT_EQUAL;
	noDepth_StencilRead.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilRead.FrontFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
	noDepth_StencilRead.FrontFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;

	res = aDevice->CreateDepthStencilState(&noDepth_StencilRead, &someStatesToInit.depthStencilStates[DEPTHSTENCILSTATE_STENCILONLY_READ]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC defaultRasterizerState = {};
	defaultRasterizerState.FillMode = D3D11_FILL_SOLID;
	defaultRasterizerState.CullMode = D3D11_CULL_BACK;
	defaultRasterizerState.DepthClipEnable = true;

	res = aDevice->CreateRasterizerState(&defaultRasterizerState, &someStatesToInit.rasterizerStates[RASTERIZERSTATE_DEFAULT]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC counterClockwiseState = {};
	counterClockwiseState.FillMode = D3D11_FILL_SOLID;
	counterClockwiseState.CullMode = D3D11_CULL_BACK;
	counterClockwiseState.FrontCounterClockwise = true;
	counterClockwiseState.DepthClipEnable = true;

	res = aDevice->CreateRasterizerState(&counterClockwiseState, &someStatesToInit.rasterizerStates[RASTERIZERSTATE_COUNTERCLOCKWISE]);
	if (FAILED(res))
	{
		return false;
	}
	D3D11_RASTERIZER_DESC counterClockwiseStateNoDepth = {};
	counterClockwiseStateNoDepth.FillMode = D3D11_FILL_SOLID;
	counterClockwiseStateNoDepth.CullMode = D3D11_CULL_BACK;
	counterClockwiseStateNoDepth.FrontCounterClockwise = true;
	counterClockwiseStateNoDepth.DepthClipEnable = false;

	res = aDevice->CreateRasterizerState(&counterClockwiseStateNoDepth, &someStatesToInit.rasterizerStates[RASTERIZERSTATE_COUNTERCLOCKWISE_NODEPTHPASS]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC wireframeRasterizerDesc = {};
	wireframeRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeRasterizerDesc.CullMode = D3D11_CULL_NONE;
	wireframeRasterizerDesc.DepthClipEnable = true;

	res = aDevice->CreateRasterizerState(&wireframeRasterizerDesc, &someStatesToInit.rasterizerStates[RASTERIZERSTATE_WIREFRAME]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC doubleSidedRasterizerDesc = {};
	doubleSidedRasterizerDesc.FillMode = D3D11_FILL_SOLID;
	doubleSidedRasterizerDesc.CullMode = D3D11_CULL_NONE;
	doubleSidedRasterizerDesc.DepthClipEnable = true;

	res = aDevice->CreateRasterizerState(&doubleSidedRasterizerDesc, &someStatesToInit.rasterizerStates[RASTERIZERSTATE_DOUBLESIDED]);
	if (FAILED(res))
	{
		return false;
	}
	someStatesToInit.rasterizerStates[RASTERIZERSTATE_DEFAULT] = nullptr;


	D3D11_SAMPLER_DESC pointSampleDesc = {};
	pointSampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	pointSampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointSampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointSampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointSampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	pointSampleDesc.MinLOD = -FLT_MAX;
	pointSampleDesc.MaxLOD = FLT_MAX;
	res = aDevice->CreateSamplerState(&pointSampleDesc, &someStatesToInit.samplerStates[SAMPLERSTATE_POINT]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_SAMPLER_DESC trilWrapSampleDesc = {};
	trilWrapSampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	trilWrapSampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	trilWrapSampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	trilWrapSampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	trilWrapSampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	trilWrapSampleDesc.MinLOD = -FLT_MAX;
	trilWrapSampleDesc.MaxLOD = FLT_MAX;
	res = aDevice->CreateSamplerState(&trilWrapSampleDesc, &someStatesToInit.samplerStates[SAMPLERSTATE_TRILINEARWRAP]);
	if (FAILED(res))
	{
		return false;
	}

	D3D11_SAMPLER_DESC clampCompareSampleDesc = {};
	clampCompareSampleDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	clampCompareSampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampCompareSampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampCompareSampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampCompareSampleDesc.ComparisonFunc = D3D11_COMPARISON_GREATER;
	clampCompareSampleDesc.MinLOD = -FLT_MAX;
	clampCompareSampleDesc.MaxLOD = FLT_MAX;
	res = aDevice->CreateSamplerState(&clampCompareSampleDesc, &someStatesToInit.samplerStates[SAMPLERSTATE_CLAMP_COMPARISON]);
	if (FAILED(res))
	{
		return false;
	}

	someStatesToInit.samplerStates[SAMPLERSTATE_TRILINEAR] = nullptr;

	return true;
}

void Engine::SetBlendState(ID3D11DeviceContext* aContext, RenderStates& someStates, BlendState aBlendState)
{
	UINT sampleMask = 0xffffffff;
	float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	aContext->OMSetBlendState(someStates.blendStates[aBlendState], blendFactor, sampleMask);

}

void Engine::SetSampleState(ID3D11DeviceContext* aContext, RenderStates& someStates, SamplerState aSampleState, unsigned int aSampleSlot)
{
	aContext->CSSetSamplers(aSampleSlot, 1, &someStates.samplerStates[aSampleState]);
	aContext->PSSetSamplers(aSampleSlot, 1, &someStates.samplerStates[aSampleState]);
	aContext->GSSetSamplers(aSampleSlot, 1, &someStates.samplerStates[aSampleState]);
	aContext->VSSetSamplers(aSampleSlot, 1, &someStates.samplerStates[aSampleState]);
}

void Engine::SetRasterizerState(ID3D11DeviceContext* aContext, RenderStates& someStates, RasterizerState aRasterizerState)
{
	aContext->RSSetState(someStates.rasterizerStates[aRasterizerState]);
}

void Engine::SetDepthStencilState(ID3D11DeviceContext* aContext, RenderStates& someStates, DepthStencilState aDepthStencilState)
{
	aContext->OMSetDepthStencilState(someStates.depthStencilStates[aDepthStencilState], 0xFF);
}
