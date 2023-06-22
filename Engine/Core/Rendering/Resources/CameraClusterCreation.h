#pragma once
struct ID3D11Device;
class Camera;
namespace Engine
{
	class ConstantBufferManager;
	struct RenderData;
	namespace DX
	{
		//void FillCluster(RenderData& aBuffer, Camera& aCamera, ConstantBufferManager& aCBufferManager);
		void FillClusterCS(ConstantBufferManager& aCBufferManager, bool aFillClusterOnlyLights);
	}
}
