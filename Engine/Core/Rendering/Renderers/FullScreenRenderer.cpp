#include "stdafx.h"
#include "FullScreenRenderer.h"
#include <d3d11.h>
#include <fstream>
#include "CU\Utility\Timer\Timer.h"
#include "..\DX_Functions\DX_RenderFunctions.h"

bool Engine::FullScreenRenderer::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	HRESULT result;

	std::ifstream vsFile;
	vsFile.open("Content/Shaders/FullscreenVS.cso", std::ios::binary);
	std::string vsData = { std::istreambuf_iterator<char>(vsFile), std::istreambuf_iterator<char>() };
	result = myDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &myVertexShader);
	vsFile.close();
	if (FAILED(result))
		return false;
	for (unsigned short i = 0; i < (unsigned short)EFullScreenShader::COUNT; i++)
	{
		myPixelShaders.Add(nullptr);
	}
	std::string shaderPaths[(unsigned short)EFullScreenShader::COUNT];
	shaderPaths[(unsigned short)EFullScreenShader::COPY] = "Content/Shaders/FullscreenPS_Copy.cso";
	shaderPaths[(unsigned short)EFullScreenShader::COPY_LETTERBOX] = "Content/Shaders/FullscreenPS_CopyLetterBoxed.cso";
	shaderPaths[(unsigned short)EFullScreenShader::TONEMAPPING] = "Content/Shaders/FullscreenPS_ToneMapping.cso";
	shaderPaths[(unsigned short)EFullScreenShader::GAUSSIANHORIZONTAL] = "Content/Shaders/FullscreenPS_GaussianHorizontal.cso";
	shaderPaths[(unsigned short)EFullScreenShader::GAUSSIANVERTICAL] = "Content/Shaders/FullscreenPS_GaussianVertical.cso";
	shaderPaths[(unsigned short)EFullScreenShader::GAUSSIAN] = "Content/Shaders/FullscreenPS_GaussianBlur.cso";
	shaderPaths[(unsigned short)EFullScreenShader::BLOOM] = "Content/Shaders/FullscreenPS_Bloom.cso";
	shaderPaths[(unsigned short)EFullScreenShader::VIGNETTE] = "Content/Shaders/FullscreenPS_Vignette.cso";
	shaderPaths[(unsigned short)EFullScreenShader::ATMOSPHERE] = "Content/Shaders/FullscreenPS_AtmosphericFog.cso";
	shaderPaths[(unsigned short)EFullScreenShader::SSAO] = "Content/Shaders/FullscreenPS_SSAO.cso";
	shaderPaths[(unsigned short)EFullScreenShader::SSAO_BLUR] = "Content/Shaders/FullscreenPS_SSAO_Blur.cso";
	shaderPaths[(unsigned short)EFullScreenShader::CHROMATICABBERATION] = "Content/Shaders/FullscreenPS_ChromaticAberration.cso";
	shaderPaths[(unsigned short)EFullScreenShader::DoF_COLORSPLIT] = "Content/Shaders/FullscreenPS_DoF_ColorSeperation.cso";
	shaderPaths[(unsigned short)EFullScreenShader::DoF_DEPTHSPLIT] = "Content/Shaders/FullscreenPS_DoF_DepthSeperation.cso";
	shaderPaths[(unsigned short)EFullScreenShader::DoF_COMBINE] = "Content/Shaders/FullscreenPS_DoF_Composite.cso";
	shaderPaths[(unsigned short)EFullScreenShader::DoF_BLUR] = "Content/Shaders/FullscreenPS_DoF_CircularBlur.cso";
	shaderPaths[(unsigned short)EFullScreenShader::GBUFFER_DEBUG] = "Content/Shaders/PS_Debug_GBufferPasses.cso";
	shaderPaths[(unsigned short)EFullScreenShader::CLEAR_BLACKPIXELS] = "Content/Shaders/FullscreenPS_ClearBlackPixels.cso";

	for (unsigned short i = 0; i < (unsigned short)EFullScreenShader::COUNT; i++)
	{
		std::ifstream psFile;
		psFile.open(shaderPaths[i], std::ios::binary);
		std::string psData = { std::istreambuf_iterator<char>(psFile), std::istreambuf_iterator<char>() };
		result = myDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &myPixelShaders[i]);
		psFile.close();
		if (FAILED(result))
			return false;
	}
	return true;
}

void Engine::FullScreenRenderer::Render(EFullScreenShader aMode, std::atomic<unsigned int>& aDrawcallCounter)
{	
	myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myContext->IASetInputLayout(nullptr);
	myContext->IASetVertexBuffers(0,0,nullptr, nullptr, nullptr);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	myContext->VSSetShader(myVertexShader, nullptr, 0);
	myContext->PSSetShader(myPixelShaders[(unsigned short)aMode], nullptr, 0);
	myContext->Draw(3, 0);
	aDrawcallCounter++;
}

void Engine::FullScreenRenderer::RenderToCustomPS()
{
	myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myContext->IASetInputLayout(nullptr);
	myContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	myContext->VSSetShader(myVertexShader, nullptr, 0);
	myContext->Draw(3, 0);
}
