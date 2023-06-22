#pragma once
#include "CU\Containers\GrowingArray.hpp"
#include "CU\Math\Matrix4x4.hpp"
#include "../Engine/ECS/Systems/RenderCommands.h"
#include "RenderData.h"


struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;

class ParticleEmitter_Instance;
struct ParticleRenderCommand;
class Camera;
namespace Engine
{
	class ConstantBufferManager;
	class ParticleRenderer
	{
	public:
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
		void RenderParticles(ParticleBuffer& aParticleEmitterList, ConstantBufferManager& aCbufferManager, Camera& aRenderCamera);
	private:
		ID3D11DeviceContext* myContext;
		ID3D11Device* myDevice;
	};
}


