#include "stdafx.h"
#include "ParticleRenderer.h"
#include <d3d11.h>

//#include "GameObjects\ParticleEmitter_Instance.h"
//#include "GameObjects\ParticleEmitter.h"
#include "GameObjects\Camera.h"

#include "..\DX_Functions\DX_RenderFunctions.h"
#include "..\Resources\ConstantBufferManager.h"
bool Engine::ParticleRenderer::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	return true;
}

void Engine::ParticleRenderer::RenderParticles(ParticleBuffer& aParticleEmitterList, ConstantBufferManager& aCbufferManager, Camera& aRenderCamera)
{
	if (aParticleEmitterList.myNumberOfSystems > 0)
	{
		HRESULT result;
		D3D11_MAPPED_SUBRESOURCE bufferData;
		//D3D11_MAPPED_SUBRESOURCE bufferData = D3D11_MAPPED_SUBRESOURCE();
		//myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		for (unsigned short i = 0; i < aParticleEmitterList.myNumberOfSystems; i++)
		{
			const ParticleRenderCommand& command = aParticleEmitterList.buffer[i];
			if (command.myAmountOfActiveVertices > 0)
			{
				//CU::AABB3Df col = command.myParticlesBound;
				//col.myMax += command.myTransform.GetTranslationVector() + v3f(100, 100, 100);
				//col.myMin += command.myTransform.GetTranslationVector() - v3f(100, 100, 100);
				if (aRenderCamera.IsPointInsideFarRadius(command.myTransform.GetTranslationVector()))
				{
					aCbufferManager.myObjectBufferData.fromOB_toWorld = command.myTransform;
					aCbufferManager.MapUnMapObjectBuffer();

					ZeroMemory(&bufferData, sizeof(D3D11_MAPPED_SUBRESOURCE));
					result = myContext->Map(command.myParticleVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferData);
					if (FAILED(result))
					{
						return;
					}
					memcpy(bufferData.pData, &(command.myVertices[0]),
						sizeof(Vertex_Particle) * command.myAmountOfActiveVertices);
					myContext->Unmap(command.myParticleVertexBuffer, 0);

					myContext->IASetVertexBuffers(0, 1, &command.myParticleVertexBuffer,
						&command.myStride,
						&command.myOffset);
					DX::SetMaterialSettings(myContext, command.myMaterial, DX::EModelRenderMode::EWholeMaterial);
		
					myContext->Draw(command.myAmountOfActiveVertices, 0);
				}
			}
		}
		myContext->GSSetShader(nullptr, nullptr, 0);
	}
}
