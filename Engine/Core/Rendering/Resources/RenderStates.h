#pragma once
#include "RenderFunctions.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11SamplerState;
namespace Engine
{
	struct RenderStates
	{
		ID3D11BlendState* blendStates[BLENDSTATE_COUNT];
		ID3D11DepthStencilState* depthStencilStates[DEPTHSTENCILSTATE_COUNT];
		ID3D11RasterizerState* rasterizerStates[RASTERIZERSTATE_COUNT];
		ID3D11SamplerState* samplerStates[SAMPLERSTATE_COUNT];
	};
	bool InitRenderStates(ID3D11Device* aDevice, RenderStates& someStatesToInit);
	void SetBlendState(ID3D11DeviceContext* aContext, RenderStates& someStates, BlendState aBlendState);
	void SetSampleState(ID3D11DeviceContext* aContext, RenderStates& someStates, SamplerState aSampleState, unsigned int aSampleSlot = 0);
	void SetRasterizerState(ID3D11DeviceContext* aContext, RenderStates& someStates, RasterizerState aRasterizerState);
	void SetDepthStencilState(ID3D11DeviceContext* aContext, RenderStates& someStates, DepthStencilState aDepthStencilState);
}