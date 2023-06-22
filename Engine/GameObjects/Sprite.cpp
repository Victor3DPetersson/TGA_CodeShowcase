#include "stdafx.h"
#include "Sprite.h"
#include <d3d11.h>
#include <fstream>

#include "../Engine/Managers/TextureManager.h"
#include "../Engine/EngineInterface.h"

//SpriteCommand& SpriteCommand::Get(SpriteCommandID id)
//{
//	static Engine::TextureManager* tm = EngineInterface::GetTextureManager();
//	return *tm->Get(id);
//}
//
//SpriteCommand::SpriteCommand()
//{
//	myTexture = nullptr;
//	myVertexBuffer = nullptr;
//	myVertexShader = nullptr;
//	myPixelShader = nullptr;
//	myGeometryShader = nullptr;
//	myInputLayout = nullptr;
//	myContext = nullptr;
//	//myIndex = USHRT_MAX;
//	myID = size_t(-1);
//}
//
//SpriteCommand::SpriteCommand(ID3D11Device* aDevice, ID3D11DeviceContext* aContext)
//{
//	myTexture = nullptr;
//	myVertexBuffer = nullptr;
//	myVertexShader = nullptr;
//	myPixelShader = nullptr;
//	myGeometryShader = nullptr;
//	myInputLayout = nullptr;
//	myContext = nullptr;
//
//	HRESULT result;
//	myContext = aContext;
//	myData.myColor = { 255, 255, 255, 255 };
//	myData.myUVOffsetBotR = { 1, 1 };
//	myData.myPivotOffset = { 0.5f, 0.5f };
//
//	D3D11_BUFFER_DESC bufferDescription = { 0 };
//	bufferDescription.ByteWidth = sizeof(myData);
//	bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
//	bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//	D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
//	subresourceData.pSysMem = &myData;
//
//	ID3D11Buffer* vertexBuffer;
//	result = aDevice->CreateBuffer(&bufferDescription, &subresourceData, &vertexBuffer);
//	if (FAILED(result))
//	{
//		assert("Failed to create Vertex Buffer");
//	}
//
//	//Load Data on sprite
//	myVertexBuffer = vertexBuffer;
//}
//
//SpriteCommand::~SpriteCommand()
//{
//	myTexture = nullptr;
//	myVertexBuffer = nullptr;
//	myVertexShader = nullptr;
//	myPixelShader = nullptr;
//	myGeometryShader = nullptr;
//	myInputLayout = nullptr;
//	myContext = nullptr;
//}
//
//SpriteCommand::SpriteCommand(SpriteCommand&& aSprCmd) noexcept
//{
//	myTexture = aSprCmd.myTexture;
//	myData = aSprCmd.myData;
//
//	myContext = aSprCmd.myContext;
//	myVertexBuffer = aSprCmd.myVertexBuffer;
//	myVertexShader = aSprCmd.myVertexShader;
//	myPixelShader = aSprCmd.myPixelShader;
//	myGeometryShader = aSprCmd.myGeometryShader;
//	myInputLayout = aSprCmd.myInputLayout;
//	//myIndex = aSprCmd.myIndex;
//	myID = aSprCmd.myID;
//
//	//aSprCmd.myIndex = USHRT_MAX;
//	aSprCmd.myID = size_t(-1);
//	aSprCmd.myTexture = nullptr;
//	aSprCmd.myContext = nullptr;
//	aSprCmd.myVertexBuffer = nullptr;
//	aSprCmd.myVertexShader = nullptr;
//	aSprCmd.myPixelShader = nullptr;
//	aSprCmd.myGeometryShader = nullptr;
//	aSprCmd.myInputLayout = nullptr;
//}
//
//SpriteCommand& SpriteCommand::operator=(SpriteCommand&& aSprCmd) noexcept
//{
//	myTexture = aSprCmd.myTexture;
//	myData = aSprCmd.myData;
//
//	myContext = aSprCmd.myContext;
//	myVertexBuffer = aSprCmd.myVertexBuffer;
//	myVertexShader = aSprCmd.myVertexShader;
//	myPixelShader = aSprCmd.myPixelShader;
//	myGeometryShader = aSprCmd.myGeometryShader;
//	myInputLayout = aSprCmd.myInputLayout;
//	//myIndex = aSprCmd.myIndex;
//	myID = aSprCmd.myID;
//
//	//aSprCmd.myIndex = USHRT_MAX;
//	aSprCmd.myID = size_t(-1);
//	aSprCmd.myTexture = nullptr;
//	aSprCmd.myContext = nullptr;
//	aSprCmd.myVertexBuffer = nullptr;
//	aSprCmd.myVertexShader = nullptr;
//	aSprCmd.myPixelShader = nullptr;
//	aSprCmd.myGeometryShader = nullptr;
//	aSprCmd.myInputLayout = nullptr;
//
//	return *this;
//}
//
//void SpriteCommand::UpdateBuffer()
//{
//	D3D11_MAPPED_SUBRESOURCE res;
//	myContext->Map(myVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
//	memcpy(res.pData, &myData, sizeof(Vertex_Sprite));
//	myContext->Unmap(myVertexBuffer, 0);
//}
//
//void SpriteCommand::UpdateBuffer(Vertex_Sprite aVertex)
//{
//	D3D11_MAPPED_SUBRESOURCE res;
//	myContext->Map(myVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
//	memcpy(res.pData, &aVertex, sizeof(Vertex_Sprite));
//	myContext->Unmap(myVertexBuffer, 0);
//}
//
//void SpriteCommand::ReleaseResources()
//{
//	SAFE_RELEASE(myVertexBuffer);
//}

WorldSpriteCommand& WorldSpriteCommand::Get(WorldSpriteCommandID id)
{
	static Engine::TextureManager* tm = EngineInterface::GetTextureManager();
	return *tm->GetW(id);
}

WorldSpriteCommand::WorldSpriteCommand()
{
	myTexture = nullptr;
	myVertexBuffer = nullptr;
	myVertexShader = nullptr;
	myPixelShader = nullptr;
	myGeometryShader = nullptr;
	myInputLayout = nullptr;
	myContext = nullptr;
	myIndex = USHRT_MAX;
}

WorldSpriteCommand::WorldSpriteCommand(ID3D11Device* aDevice, ID3D11DeviceContext* aContext)
{
	myTexture = nullptr;
	myVertexBuffer = nullptr;
	myVertexShader = nullptr;
	myPixelShader = nullptr;
	myGeometryShader = nullptr;
	myInputLayout = nullptr;
	myContext = nullptr;
	HRESULT result;
	myContext = aContext;
	myData.myColor = { 255, 255, 255, 255 };
	myData.myUVOffsetBotR = { 1, 1 };
	myData.myPivotOffset = { 0.5f, 0.5f };

	D3D11_BUFFER_DESC bufferDescription = { 0 };
	bufferDescription.ByteWidth = sizeof(myData);
	bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
	subresourceData.pSysMem = &myData;

	ID3D11Buffer* vertexBuffer;
	result = aDevice->CreateBuffer(&bufferDescription, &subresourceData, &vertexBuffer);
	if (FAILED(result))
	{
		assert("Failed to create Vertex Buffer");
	}
	//Load Data on sprite
	myVertexBuffer = vertexBuffer;
}

WorldSpriteCommand::~WorldSpriteCommand()
{
	myTexture = nullptr;
	myVertexBuffer = nullptr;
	myVertexShader = nullptr;
	myPixelShader = nullptr;
	myGeometryShader = nullptr;
	myInputLayout = nullptr;
	myContext = nullptr;
}

WorldSpriteCommand::WorldSpriteCommand(WorldSpriteCommand&& aSprCmd) noexcept
{
	myTexture = aSprCmd.myTexture;
	myData = aSprCmd.myData;

	myContext = aSprCmd.myContext;
	myVertexBuffer = aSprCmd.myVertexBuffer;
	myVertexShader = aSprCmd.myVertexShader;
	myPixelShader = aSprCmd.myPixelShader;
	myGeometryShader = aSprCmd.myGeometryShader;
	myInputLayout = aSprCmd.myInputLayout;
	myIndex = aSprCmd.myIndex;

	aSprCmd.myIndex = USHRT_MAX;
	aSprCmd.myTexture = nullptr;
	aSprCmd.myContext = nullptr;
	aSprCmd.myVertexBuffer = nullptr;
	aSprCmd.myVertexShader = nullptr;
	aSprCmd.myPixelShader = nullptr;
	aSprCmd.myGeometryShader = nullptr;
	aSprCmd.myInputLayout = nullptr;
}

WorldSpriteCommand& WorldSpriteCommand::operator=(WorldSpriteCommand&& aSprCmd) noexcept
{
	myTexture = aSprCmd.myTexture;
	myData = aSprCmd.myData;

	myContext = aSprCmd.myContext;
	myVertexBuffer = aSprCmd.myVertexBuffer;
	myVertexShader = aSprCmd.myVertexShader;
	myPixelShader = aSprCmd.myPixelShader;
	myGeometryShader = aSprCmd.myGeometryShader;
	myInputLayout = aSprCmd.myInputLayout;
	myIndex = aSprCmd.myIndex;

	aSprCmd.myIndex = USHRT_MAX;
	aSprCmd.myTexture = nullptr;
	aSprCmd.myContext = nullptr;
	aSprCmd.myVertexBuffer = nullptr;
	aSprCmd.myVertexShader = nullptr;
	aSprCmd.myPixelShader = nullptr;
	aSprCmd.myGeometryShader = nullptr;
	aSprCmd.myInputLayout = nullptr;

	return *this;
}

bool WorldSpriteCommand::operator<(const WorldSpriteCommand& aCommand)
{
	if (distanceToCamera < aCommand.distanceToCamera)
	{
		return true;
	}
	return false;
}

bool WorldSpriteCommand::operator>(const WorldSpriteCommand& aCommand)
{
	if (distanceToCamera > aCommand.distanceToCamera)
	{
		return true;
	}
	return false;
}

void WorldSpriteCommand::UpdateBuffer()
{
	D3D11_MAPPED_SUBRESOURCE res;
	myContext->Map(myVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	memcpy(res.pData, &myData, sizeof(Vertex_Sprite));
	myContext->Unmap(myVertexBuffer, 0);
}

void WorldSpriteCommand::ReleaseResources()
{
	SAFE_RELEASE(myVertexBuffer);
}
