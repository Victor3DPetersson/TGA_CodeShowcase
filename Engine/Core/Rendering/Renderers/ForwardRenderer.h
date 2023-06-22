#pragma once
#include "CU/Containers/GrowingArray.hpp"
#include "CU/Math\Vector2.hpp"
#include "CU/Math/Matrix4x4.hpp"
#include "CU/Math/Color.hpp"
#include "GameObjects/Camera.h"
#include <atomic>
#include "..\Resources\RenderFunctions.h"
#include "RenderData.h"

struct ID3D11SamplerState;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct MeshesToRender;

namespace Engine
{
	class ConstantBufferManager;
	class DirectXFramework;
	class Renderer;
	class FullScreenTexture;
	class ModelManager;
	class ForwardRenderer
	{
	public:
		ForwardRenderer();
		~ForwardRenderer();

		bool Init(DirectXFramework* aFramework, ModelManager* aModelManager);

		void RenderTranslucent(MeshesToRender& meshes,	std::atomic<unsigned int>& aDrawcallCounter, ConstantBufferManager& aCBufferManager, bool isMainRender);

		void RenderTransparentCutoutMeshes(MeshesToRender& meshes, std::atomic<unsigned int>& aDrawcallCounter, ConstantBufferManager& aCBufferManager, bool aRenderOnlyLights, bool aRenderLights);
		
		void RenderOutlines(MeshesToRender& meshes,
			Renderer* aRenderer, FullScreenTexture* aStencilTexture, FullScreenTexture* aDeferredTarget,
			std::atomic<unsigned int>& aDrawcallCounter, ConstantBufferManager& aCBufferManager);
	private:
		ID3D11DeviceContext* myDeviceContext;
		ID3D11VertexShader* myOutline_VS;
		ID3D11VertexShader* myOutlineAnim_VS;
		ID3D11PixelShader* myOutline_PS;
		ID3D11PixelShader* myOutline_PS_Hovered;
		ModelManager* myModelmanager;
	};

}


