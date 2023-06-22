#pragma once
#include "CU\Containers\GrowingArray.hpp"
#include "GameObjects\Sprite.h"
#include "CU\Containers\MinHeap.hpp"
struct ID3D11Device;
struct ID3D11DeviceContext;
class Camera;
namespace Engine
{
	class ConstantBufferManager;
	//myPrimitiveTopology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
	class SpriteRenderer
	{
	public:
		void Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ConstantBufferManager& aConstantBufferManager);
		void RenderWorldSpaceUI(WorldSpriteCommand* someSpritesToRender, size_t aSpriteCount, Camera* aRenderCamera);
		void RenderWorldSpaceUINoSortForPicking(WorldSpriteCommand* someSpritesToRender, size_t aSpriteCount);
		void RenderScreenSpaceUI(SpriteCommand* someSpritesToRender, size_t aSpriteCount, v2f aMainRenderRes);
		void SetUiRenderResolution(v2f aResolution);
	private:

		struct SpriteCommandIndex
		{
			unsigned short cmdIndex;
			float distToCam;
			bool operator > (const SpriteCommandIndex& aCommand)
			{
				if (aCommand.distToCam < distToCam)
				{
					return true;
				}
				return false;
			}
		};
		//SORT SPRITES HERE
		CU::MinHeap<SpriteCommandIndex> mySortedIndices;
		unsigned short myAmountOfWorldSprites;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		ConstantBufferManager* myCBufferManager;
		const float myStandardAspectRatio = 1920.0f / 1080.0f;
		v2f myUIRenderRes; 
	};
}