#include "stdafx.h"
#include "SpriteRenderer.h"
#include "GameObjects\Camera.h"
#include "..\DX_Functions/DX_RenderFunctions.h"
#include "..\Resources\ConstantBufferManager.h"
#include <d3d11.h>

#include <iostream>
#include <algorithm>
#include "UI\UIContext.h"

void Engine::SpriteRenderer::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ConstantBufferManager& aConstantBufferManager)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myCBufferManager = &aConstantBufferManager;
}
void Engine::SpriteRenderer::RenderWorldSpaceUI(WorldSpriteCommand* someSpritesToRender, size_t aSpriteCount, Camera* aRenderCamera)
{
	if (aSpriteCount > 0)
	{
		myAmountOfWorldSprites = (unsigned short)aSpriteCount;
		const v3f cameraPosition = aRenderCamera->GetMatrix().GetTranslationVector();
		SpriteCommandIndex commandIndex;
		for (unsigned short i = 0; i < myAmountOfWorldSprites; i++)
		{
			someSpritesToRender[i].distanceToCamera = (someSpritesToRender[i].myData.myPosition - cameraPosition).LengthSqr();
			float distance = someSpritesToRender[i].distanceToCamera;
			someSpritesToRender[i].UpdateBuffer();
			commandIndex.cmdIndex = i;
			commandIndex.distToCam = distance;
			mySortedIndices.Enqueue(commandIndex);
		}

		//Render the sprites
		myContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
		myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		unsigned int stride = sizeof(Vertex_World_Sprite);
		unsigned int offset = 0;
		for (unsigned short i = 0; i < myAmountOfWorldSprites; i++)
		{
			unsigned short index = mySortedIndices.Dequeue().cmdIndex;
			myContext->IASetInputLayout(someSpritesToRender[index].myInputLayout);
			myContext->IASetVertexBuffers(0, 1, &someSpritesToRender[index].myVertexBuffer, &stride, &offset);
			myContext->VSSetShader(someSpritesToRender[index].myVertexShader, nullptr, 0);

			myContext->GSSetShader(someSpritesToRender[index].myGeometryShader, nullptr, 0);

			myContext->PSSetShader(someSpritesToRender[index].myPixelShader, nullptr, 0);
			myContext->PSSetShaderResources(8, 1, &someSpritesToRender[index].myTexture);

			if (someSpritesToRender[index].mySecondTexture)
			{
				myContext->PSSetShaderResources(9, 1, &WorldSpriteCommand::Get(someSpritesToRender[index].mySecondTexture).myTexture);
			}

			myContext->Draw(1, 0);
		}
		myContext->GSSetShader(nullptr, nullptr, 0);
	}
}

void Engine::SpriteRenderer::RenderWorldSpaceUINoSortForPicking(WorldSpriteCommand* someSpritesToRender, size_t aSpriteCount)
{
	if (aSpriteCount > 0)
	{
		myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		myCBufferManager->objectEffectBufferData.gBufferVSEffectIndex = 1;
		unsigned int stride = sizeof(Vertex_World_Sprite);
		unsigned int offset = 0;
		for (unsigned short i = 0; i < myAmountOfWorldSprites; i++)
		{
			myCBufferManager->objectEffectBufferData.modelIndex = someSpritesToRender[i].myEntityIndex;
			myCBufferManager->MapUnMapEffectBuffer();
			myContext->IASetInputLayout(someSpritesToRender[i].myInputLayout);
			myContext->IASetVertexBuffers(0, 1, &someSpritesToRender[i].myVertexBuffer, &stride, &offset);
			myContext->VSSetShader(someSpritesToRender[i].myVertexShader, nullptr, 0);
			myContext->GSSetShader(someSpritesToRender[i].myGeometryShader, nullptr, 0);

			myContext->Draw(1, 0);
		}
		myContext->GSSetShader(nullptr, nullptr, 0);
	}
}

void Engine::SpriteRenderer::RenderScreenSpaceUI(SpriteCommand* someSpritesToRender, size_t aSpriteCount, v2f aMainRenderRes)
{
	TextureManager* txmn = EngineInterface::GetTextureManager();
	ID3D11InputLayout* il = *txmn->myPooledInputLayouts.Get("ScreenSpace");
	ID3D11GeometryShader* gs = *txmn->myPooledGeometeryShaders.Get("Content/Shaders/UI_GS.cso");
	ID3D11VertexShader* vs = *txmn->myPooledVertexShaders.Get("Content/Shaders/UI_VS.cso");
	ID3D11PixelShader* ps = *txmn->myPooledPixelShaders.Get("Content/Shaders/UI_PS.cso");

	// SORT SPRITES HERE
	unsigned short size = (unsigned short)aSpriteCount;
	SpriteCommandIndex commandIndex;
	for (unsigned short i = 0; i < size; i++)
	{
		commandIndex.cmdIndex = i;
		commandIndex.distToCam = (float)someSpritesToRender[i].vtx.myZIndex;
		mySortedIndices.Enqueue(commandIndex);
	}

	myContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	unsigned int stride = sizeof(Vertex_Sprite);
	unsigned int offset = 0;
	Vertex_Sprite data;
	ID3D11ShaderResourceView* srv;
	for (unsigned short i = 0; i < size; i++)
	{
		unsigned short index = mySortedIndices.Dequeue().cmdIndex;
		data = someSpritesToRender[index].vtx;

		if (someSpritesToRender[index].texture == NIL_UUID)
			continue;

		D3D11_MAPPED_SUBRESOURCE res;
		myContext->Map(txmn->myVtxBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, &data, sizeof(Vertex_Sprite));
		myContext->Unmap(txmn->myVtxBuf, 0);

		myContext->IASetInputLayout(il);
		myContext->IASetVertexBuffers(0, 1, &txmn->myVtxBuf, &stride, &offset);
		myContext->VSSetShader(vs, nullptr, 0);
		
		myContext->GSSetShader(gs, nullptr, 0);

		myContext->PSSetShader(ps, nullptr, 0);

		const FixedString256& path = txmn->GetSpritePath(someSpritesToRender[index].texture);
		srv = txmn->GetTextureObject(path.Data());
		myContext->PSSetShaderResources(8, 1, &srv);

		/*if (someSpritesToRender[index].mySecondTexture)
		{
			myContext->PSSetShaderResources(9, 1, &SpriteCommand::Get(someSpritesToRender[index].mySecondTexture).myTexture);
		}*/

		myContext->Draw(1, 0);
	}
	myContext->PSSetShader(nullptr, nullptr, 0);
	myContext->GSSetShader(nullptr, nullptr, 0);
	myContext->VSSetShader(nullptr, nullptr, 0);
}

void Engine::SpriteRenderer::SetUiRenderResolution(v2f aResolution)
{
	myUIRenderRes = aResolution;
}
