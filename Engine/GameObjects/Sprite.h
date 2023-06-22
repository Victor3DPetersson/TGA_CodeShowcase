#pragma once
#include "Vertex.h"
#include "..\Core\Rendering\Resources\DX_Includes.h"
#include "..\CommonUtilities\CU\Utility\FixedString.hpp"

namespace Engine
{
	class SpriteRenderer;
	class TextureManager;
}

#include "../Engine/ECS/Level/StructExporter.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

struct SpriteCommand
{
	SpriteID texture;
	Vertex_Sprite vtx;
	Entity id;
};

class WorldSpriteCommand
{
public:
	static WorldSpriteCommand& Get(WorldSpriteCommandID id);

	WorldSpriteCommand();
	WorldSpriteCommand(ID3D11Device* aDevice, ID3D11DeviceContext* aContext);
	~WorldSpriteCommand();

	WorldSpriteCommand(const WorldSpriteCommand&) = default;
	WorldSpriteCommand& operator=(const WorldSpriteCommand&) = default;
	WorldSpriteCommand(WorldSpriteCommand&& aSprCmd) noexcept;
	WorldSpriteCommand& operator=(WorldSpriteCommand&& aSprCmd) noexcept;

	ID3D11ShaderResourceView* myTexture;
	WorldSpriteCommandID mySecondTexture = 0;
	Vertex_World_Sprite myData;
	FixedString256 myTexturePath;

	bool operator < (const WorldSpriteCommand& aCommand);
	bool operator > (const WorldSpriteCommand& aCommand);

	inline bool operator == (const WorldSpriteCommand& aComp)
	{
		return myEntityIndex == aComp.myEntityIndex;
	}

	void UpdateBuffer();
	void ReleaseResources();
	friend class Engine::SpriteRenderer;
	friend class Engine::TextureManager;
	float distanceToCamera = FLT_MAX;
	ID3D11DeviceContext* myContext;
	ID3D11Buffer* myVertexBuffer;
	ID3D11VertexShader* myVertexShader;
	ID3D11PixelShader* myPixelShader;
	ID3D11GeometryShader* myGeometryShader;
	ID3D11InputLayout* myInputLayout;
	unsigned short myIndex;
	unsigned short myEntityIndex;
};