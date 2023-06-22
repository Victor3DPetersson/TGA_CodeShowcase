#pragma once
#include "Includes.h"
#include "RenderData.h"

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11SamplerState;
namespace Engine
{
	class FullScreenRenderer
	{
	public:
		enum class EFullScreenShader
		{
			COPY,
			COPY_LETTERBOX,
			TONEMAPPING,
			GAUSSIANHORIZONTAL,
			GAUSSIANVERTICAL,
			GAUSSIAN,
			BLOOM,
			VIGNETTE,
			ATMOSPHERE,
			CHROMATICABBERATION,
			SSAO,
			SSAO_BLUR,
			DoF_COLORSPLIT,
			DoF_DEPTHSPLIT,
			DoF_COMBINE,
			DoF_BLUR,
			CLEAR_BLACKPIXELS,
			GBUFFER_DEBUG,
			COUNT
		};
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
		void Render(EFullScreenShader mode, std::atomic<unsigned int>& aDrawcallCounter);
		void RenderToCustomPS();
	private:
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		ID3D11VertexShader* myVertexShader;
		CU::VectorOnStack<ID3D11PixelShader*, (unsigned int)EFullScreenShader::COUNT> myPixelShaders;
	};
}