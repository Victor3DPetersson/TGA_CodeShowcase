#include "stdafx.h"
#include "TextureManager.h"
#include "DDS_TextureLoading/DDSTextureLoader11.h"

#include "..\ModelViewer\ModelStuff\TextureImportData.h"

#include <direct.h>
#include <assert.h>

#include "GameObjects\Sprite.h"
#include "Core\WindowHandler.h"

#include "DDS_TextureLoading\ScreenGrab11.h"
#include <wincodec.h>
#include "Core\Rendering/DX_Functions\DX_RenderFunctions.h"
#include "Core\Rendering/Resources\FullScreenTexture.h"
#include "Core\Rendering/Resources\FullScreenTexture_Factory.h"

#include <filesystem>
#include "../RenderData.h"

Engine::TextureManager::~TextureManager()
{
	myDevice = nullptr;
}

size_t Engine::TextureManager::mySpriteCommandIDCounter = size_t(0);
size_t Engine::TextureManager::myWSpriteCommandIDCounter = size_t(0);

bool Engine::TextureManager::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext)
{
	myDevice = aDevice;
	myContext = aContext;
	//std::filesystem::path currentWD = std::filesystem::current_path();
	if (!CreateInternalTextureObject("Content/Textures/default_texture.dds")) {
		assert(false && "Failed to create default_texture, check if texture is deleted");
		return false;
	}
	if (!CreateInternalTextureObject("Content/Textures/default_texture_Normal.dds")) {
		assert(false && "Failed to create default_normal_texture, check if texture is deleted");
		return false;
	}
	if (!CreateInternalTextureObject("Content/Textures/default_texture_Material.dds")) {
		assert(false && "Failed to create default_material_texture, check if texture is deleted");
		return false;
	}
	if (!CreateInternalTextureObject("Content/Textures/CubeMaps/ClearSky_cube_radiance.dds")) {
		assert(false && "Failed to create default_CubeTexture, check if texture is deleted");
		return false;
	}
	if (!CreateInternalTextureObject("Content/Textures/default_emissive.dds")) {
		assert(false && "Failed to create default_CubeTexture, check if texture is deleted");
		return false;
	}

	ID3D11GeometryShader* geoShader = DX::LoadGS(myDevice, "Content/Shaders/UI_GS.cso");
	if (!geoShader) {
		assert(false && "FUCK YOU Geo shader failed to create");
		return false;
	}
	myPooledGeometeryShaders.Insert("Content/Shaders/UI_GS.cso", geoShader);

	ID3D11PixelShader* pixelShader = DX::LoadPS(myDevice, "Content/Shaders/UI_PS.cso");
	if (!pixelShader) {
		assert(false && "FUCK YOU Pixel shader failed to create");
		return false;
	}
	myPooledPixelShaders.Insert("Content/Shaders/UI_PS.cso", pixelShader);

	std::string vsData;
	ID3D11VertexShader* vertexShader = DX::LoadVS(myDevice, "Content/Shaders/UI_VS.cso", vsData);
	if (!vertexShader) {
		assert(false && "FUCK YOU Vertex shader failed to create");
		return false; 
	}
	myPooledVertexShaders.Insert("Content/Shaders/UI_VS.cso", vertexShader);


	HRESULT result;
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION",	0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",		0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//SIZE
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//UVOffset Top L
		{ "TEXCOORD",	1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//UVOffset Bot R
		{ "TEXCOORD",	2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//Pivot Offset
		{ "ROTATION",	0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//rotation
		{ "DATA",		0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "ZINDEX",		0, DXGI_FORMAT_R32_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "PADDING",	0, DXGI_FORMAT_R32G32_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	ID3D11InputLayout* inputLayout;
	result = myDevice->CreateInputLayout(layout, sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC), vsData.data(), vsData.size(), &inputLayout);
	if (FAILED(result))
	{
		assert(false && "Failed to assemble input layout");
		return false;
	}
	myPooledInputLayouts.Insert("ScreenSpace", inputLayout);

	{
		D3D11_BUFFER_DESC bufferDescription = { 0 };
		bufferDescription.ByteWidth = sizeof(Vertex_Sprite);
		bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
		bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
		subresourceData.pSysMem = &myVtxSpr;

		ID3D11Buffer* vertexBuffer;
		result = aDevice->CreateBuffer(&bufferDescription, &subresourceData, &vertexBuffer);
		if (FAILED(result))
		{
			assert(!"Failed to create Vertex Buffer");
		}
		myVtxBuf = vertexBuffer;
	}



	/////////////////WORLD SPRITES////////////////////////

	ID3D11GeometryShader* worldGeoShader = DX::LoadGS(myDevice, "Content/Shaders/World_UI_GS.cso");
	if (!worldGeoShader)
	{
		assert(false && "Failed to create world geo shader");
		return false;
	}
	myPooledGeometeryShaders.Insert("Content/Shaders/World_UI_GS.cso", worldGeoShader);

	ID3D11VertexShader* worldVertexShader = DX::LoadVS(myDevice, "Content/Shaders/World_UI_VS.cso", vsData);
	if (!worldVertexShader)
	{
		assert(false && "Failed to create world vertex shader");
		return false;
	}
	myPooledVertexShaders.Insert("Content/Shaders/World_UI_VS.cso", worldVertexShader);

	D3D11_INPUT_ELEMENT_DESC worldLayout[] =
	{
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",		0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//SIZE
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//UVOffset Top L
		{ "TEXCOORD",	1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//UVOffset Bot R
		{ "TEXCOORD",	2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//Pivot Offset
		{ "ROTATION",	0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//rotation
	};
	ID3D11InputLayout* worldInputLayout;
	result = aDevice->CreateInputLayout(worldLayout, sizeof(worldLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), vsData.data(), vsData.size(), &worldInputLayout);
	if (FAILED(result))
	{
		assert("Failed to assemble input layout");
		return false;
	}
	myPooledInputLayouts.Insert("WorldSpace", worldInputLayout);

	return true;
}

SpriteID Engine::TextureManager::GetSpriteID(const char* const aPath)
{
	SpriteID id = NIL_UUID;

	SpriteID* idptr = myPathSpriteIDDict.Get(aPath);
	if (idptr)
	{
		id = *idptr;
	}
	else
	{
		if (UuidCreate(&id) != RPC_S_OK)
		{
			return NIL_UUID;
		}
		*mySpriteIDPathDict[id] = aPath;
		*myPathSpriteIDDict[aPath] = id;
	}

	return id;
}

ID3D11ShaderResourceView* Engine::TextureManager::GetTextureObject(
	const ShortString& aTexturePath, 
	ETextureTypes aTextureType)
{
	std::string string = aTexturePath.GetString();

	if (string.empty())
	{
		switch (aTextureType)
		{
		case ETextureTypes::eColor:
			myTextureCounter["Content/Textures/default_texture.dds"]++;
			return  myTextures["Content/Textures/default_texture.dds"];
			break;
		case ETextureTypes::eNormal:
			myTextureCounter["Content/Textures/default_texture_Normal.dds"]++;
			return  myTextures["Content/Textures/default_texture_Normal.dds"];
			break;
		case ETextureTypes::eMaterial:
			myTextureCounter["Content/Textures/default_texture_Material.dds"]++;
			return  myTextures["Content/Textures/default_texture_Material.dds"];
			break;
		case ETextureTypes::eEmissive:
			myTextureCounter["Content/Textures/default_emissive.dds"]++;
			return myTextures["Content/Textures/default_emissive.dds"];
			break;
		case ETextureTypes::eEnvironment:
			myTextureCounter["Content/Textures/CubeMaps/ClearSky_cube_radiance.dds"]++;
			return myTextures["Content/Textures/CubeMaps/ClearSky_cube_radiance.dds"];
			break;
		default:
			break;
		}
	}
	auto t = myTextures.find(string);
	if (t == myTextures.end())
	{
		ID3D11ShaderResourceView* textureObject = CreateTextureObject(string);
		if (textureObject != nullptr)
		{
			myTextureCounter[string]++;
			return textureObject;
		}
		else
		{
			// if texture can not be created the path was incorrect
			switch (aTextureType)
			{
			case ETextureTypes::eColor:
				myTextureCounter["Content/Textures/default_texture.dds"]++;
				textureObject = myTextures["Content/Textures/default_texture.dds"];
				break;
			case ETextureTypes::eNormal:
				myTextureCounter["Content/Textures/default_texture_Normal.dds"]++;
				textureObject = myTextures["Content/Textures/default_texture_Normal.dds"];
				break;
			case ETextureTypes::eMaterial:
				myTextureCounter["Content/Textures/default_texture_Material.dds"]++;
				textureObject = myTextures["Content/Textures/default_texture_Material.dds"];
				break;
			case ETextureTypes::eEmissive:
				return nullptr;
				break;
			case ETextureTypes::eEnvironment:
				myTextureCounter["Content/Textures/Square_night_cube_specular.dds"]++;
				textureObject = myTextures["Content/Textures/CubeMaps/Square_night_cube_specular.dds"];
				break;
			default:
				break;
			}
			return textureObject;
		}
	}
	//If texture exists return it
	myTextureCounter[string]++;
	return  myTextures[string];
}

//SpriteCommandID Engine::TextureManager::CreateSprite(
//	const ShortString& aTexturePath,
//	const char* const aCustomPS,
//	const char* const aCustomVS,
//	const char* const aCustomGS
//)
//{
//	SpriteCommand* sprite = nullptr;
//	size_t id = ++mySpriteCommandIDCounter;
//	mySpriteCommandDict.Insert(id, SpriteCommand(myDevice, myContext));
//	sprite = mySpriteCommandDict.Get(id);
//	sprite->myContext = myContext;
//	sprite->myID = id;
//	if (aCustomPS)
//	{
//		if (myPooledPixelShaders.Contains(aCustomPS))
//		{
//			sprite->myPixelShader = *myPooledPixelShaders[aCustomPS];
//		}
//		else
//		{
//			ID3D11PixelShader* pixelShader = DX::LoadPS(myDevice, aCustomPS);
//			if (pixelShader)
//			{
//				myPooledPixelShaders.Insert(aCustomPS, pixelShader);
//				sprite->myPixelShader = *myPooledPixelShaders[aCustomPS];
//			}
//			else
//			{
//				return 0;
//			}
//		}
//	}
//	else
//	{
//		sprite->myPixelShader = *myPooledPixelShaders["Content/Shaders/UI_PS.cso"];
//	}
//
//	if (aCustomGS)
//	{
//		if (myPooledGeometeryShaders.Contains(aCustomGS))
//		{
//			sprite->myGeometryShader = *myPooledGeometeryShaders[aCustomGS];
//		}
//		else
//		{
//			ID3D11GeometryShader* geoShader = DX::LoadGS(myDevice, aCustomGS);
//			if (geoShader)
//			{
//				myPooledGeometeryShaders.Insert(aCustomGS, geoShader);
//				sprite->myGeometryShader = *myPooledGeometeryShaders[aCustomGS];
//			}
//			else
//			{
//				return size_t(-1);
//			}
//		}
//	}
//	else
//	{
//		sprite->myGeometryShader = *myPooledGeometeryShaders["Content/Shaders/UI_GS.cso"];
//	}
//	
//	if (aCustomVS)
//	{
//		if (myPooledVertexShaders.Contains(aCustomVS))
//		{
//			sprite->myVertexShader = *myPooledVertexShaders[aCustomVS];
//		}
//		else
//		{
//			ID3D11VertexShader* vertexShader = DX::LoadVS(myDevice, aCustomVS);
//			if (vertexShader)
//			{
//				myPooledVertexShaders.Insert(aCustomVS, vertexShader);
//				sprite->myVertexShader = *myPooledVertexShaders[aCustomVS];
//			}
//			else
//			{
//				return size_t(-1);
//			}
//
//		}
//	}
//	else
//	{
//		sprite->myVertexShader = *myPooledVertexShaders["Content/Shaders/UI_VS.cso"];
//	}
//	sprite->myInputLayout = *myPooledInputLayouts["ScreenSpace"];
//
//	if (GetTextureObject(aTexturePath.GetString()))
//	{
//		sprite->myTexture = GetTextureObject(aTexturePath);
//		sprite->myData.mySize = GetImageDimensions(sprite->myTexture);
//		sprite->UpdateBuffer();
//		sprite->myTexturePath = aTexturePath.GetString();
//		return id;
//	}
//	else
//	{
//		mySpriteCommandDict.Remove(id);
//		return 0;
//	}
//}

WorldSpriteCommandID Engine::TextureManager::CreateWorldSprite(
	const ShortString& aTexturePath, 
	const char* const aCustomPS, 
	const char* const aCustomVS, 
	const char* const aCustomGS)
{
	size_t id = ++myWSpriteCommandIDCounter;
	WorldSpriteCommand* sprite = myWorldSpriteCmdDict.Insert(id, WorldSpriteCommand(myDevice, myContext));
	sprite->myContext = myContext;

	if (aCustomPS)
	{
		if (myPooledPixelShaders.Contains(aCustomPS))
		{
			sprite->myPixelShader = *myPooledPixelShaders[aCustomPS];
		}
		else
		{
			ID3D11PixelShader* pixelShader = DX::LoadPS(myDevice, aCustomPS);
			if (pixelShader)
			{
				myPooledPixelShaders.Insert(aCustomPS, pixelShader);
				sprite->myPixelShader = *myPooledPixelShaders[aCustomPS];
			}
			else
			{
				return 0;
			}
		}
	}
	else
	{
		sprite->myPixelShader = *myPooledPixelShaders["Content/Shaders/UI_PS.cso"];
	}

	if (aCustomGS)
	{
		if (myPooledGeometeryShaders.Contains(aCustomGS))
		{
			sprite->myGeometryShader = *myPooledGeometeryShaders[aCustomGS];
		}
		else
		{
			ID3D11GeometryShader* geoShader = DX::LoadGS(myDevice, aCustomGS);
			if (geoShader)
			{
				myPooledGeometeryShaders.Insert(aCustomGS, geoShader);
				sprite->myGeometryShader = *myPooledGeometeryShaders[aCustomGS];
			}
			else
			{
				return 0;
			}
		}
	}
	else
	{
		sprite->myGeometryShader = *myPooledGeometeryShaders["Content/Shaders/World_UI_GS.cso"];
	}

	if (aCustomVS)
	{
		if (myPooledVertexShaders.Contains(aCustomVS))
		{
			sprite->myVertexShader = *myPooledVertexShaders[aCustomVS];
		}
		else
		{
			ID3D11VertexShader* vertexShader = DX::LoadVS(myDevice, aCustomVS);
			if (vertexShader)
			{
				myPooledVertexShaders.Insert(aCustomVS, vertexShader);
				sprite->myVertexShader = *myPooledVertexShaders[aCustomVS];
			}
			else
			{
				return 0;
			}

		}
	}
	else
	{
		sprite->myVertexShader = *myPooledVertexShaders["Content/Shaders/World_UI_VS.cso"];
	}
	sprite->myInputLayout = *myPooledInputLayouts["WorldSpace"];


	if (GetTextureObject(aTexturePath.GetString()))
	{
		sprite->myTexture = GetTextureObject(aTexturePath);
		sprite->myData.mySize = GetImageDimensions(sprite->myTexture);;
		sprite->UpdateBuffer();
		sprite->myTexturePath = aTexturePath.GetString();
		return id;
	}
	else
	{
		return 0;
	}
}



bool Engine::TextureManager::ImportExternalTexture(const std::string& aTexturePath)
{
	std::string filepath;
	filepath.append(aTexturePath);
	std::wstring filename(filepath.begin(), filepath.end());

	HRESULT result;
	ID3D11ShaderResourceView* shaderResourceView;
	result = DirectX::CreateDDSTextureFromFile(myDevice, filename.c_str(), nullptr, &shaderResourceView);
	if (FAILED(result))
	{
		return false;
	}
	CU::Vector2ui resourceRes = GetImageDimensions(shaderResourceView);
	CU::Vector2f floatRes = { (float)resourceRes.x, (float)resourceRes.y };
	floatRes.x = log2(floatRes.x);
	floatRes.y = log2(floatRes.y);
	if ((float)(int)floatRes.x == floatRes.x && (float)(int)floatRes.y == floatRes.y)
	{
		myTextures[aTexturePath] = shaderResourceView;
		return true;
	}
	return false;
}

ID3D11ShaderResourceView* Engine::TextureManager::CreateTextureObject(const ShortString& aTexturePath)
{
	if (CreateInternalTextureObject(aTexturePath.GetString()))
	{
		auto textureObject = myTextures[aTexturePath.GetString()];

		{
			mySizeCacheLock.test_and_set();

			SpriteID* id = myPathSpriteIDDict.Get(aTexturePath.GetString());
			if (id && !mySizeCache.Contains(*id))
			{
				mySizeCache.Insert(*id, GetImageDimensions(textureObject));
			}

			mySizeCacheLock.clear();
		}
		return textureObject;
	}
    return nullptr;
}

bool Engine::TextureManager::CreateInternalTextureObject(const std::string& aTexturePath)
{
	std::wstring filename (aTexturePath.begin(), aTexturePath.end());
	HRESULT result;
	ID3D11ShaderResourceView* shaderResourceView;
	result = DirectX::CreateDDSTextureFromFile(myDevice, filename.c_str(), nullptr, &shaderResourceView);
	if (FAILED(result))
	{
		return false;
	}
	myTextures[aTexturePath] = shaderResourceView;
	myTextureCounter[aTexturePath] = 1;

	return true;
}

CU::Vector2ui Engine::TextureManager::GetImageDimensions(ID3D11ShaderResourceView* aSRV)
{
	ID3D11Resource* res = nullptr;
	aSRV->GetResource(&res);

	ID3D11Texture2D* texture2d = nullptr;
	HRESULT hr = res->QueryInterface(&texture2d);

	CU::Vector2ui dim(0, 0);
	if (SUCCEEDED(hr))
	{
		D3D11_TEXTURE2D_DESC desc;
		texture2d->GetDesc(&desc);
		dim.x = (unsigned int)(desc.Width);
		dim.y = (unsigned int)(desc.Height);
	}
	SAFE_RELEASE(texture2d);
	SAFE_RELEASE(res);
	return dim;
}

void Engine::TextureManager::ReturnTexture(ID3D11ShaderResourceView* aTexturePointer, bool aDeleteTextureResource)
{
	if (!aTexturePointer) { return; }
	std::string texturePath;
	for (auto texture : myTextures)
	{
		if (texture.second == aTexturePointer)
		{
			texturePath = texture.first;
			break;
		}
	}
	myTextureCounter[texturePath]--;
	aTexturePointer = nullptr;
	if (aDeleteTextureResource && myTextureCounter[texturePath] == 0)
	{
		SAFE_RELEASE(myTextures[texturePath]);
		myTextures.erase(texturePath);
		myTextureCounter.erase(texturePath);
	}
}

//void Engine::TextureManager::ReturnSprite(SpriteCommandID const aSprCmd)
//{
//	myReturnedSpriteCommands.Enqueue(aSprCmd);
//	SpriteCommand* spr = Get(aSprCmd);
//	if (spr->mySecondTexture)
//	{
//		ReturnSprite(spr->mySecondTexture);
//	}
//}

void Engine::TextureManager::ReturnWorldSprite(WorldSpriteCommandID const aSprCmd)
{
	myReturnedWorldSpriteCommands.Enqueue(aSprCmd);
	WorldSpriteCommand* ptr = GetW(aSprCmd);
	if (ptr && ptr->mySecondTexture)
	{
		ReturnWorldSprite(ptr->mySecondTexture);
	} 
}

void Engine::TextureManager::Flush()
{
	/*for (unsigned short i = 0; i < myReturnedSpriteCommands.Size(); i++)
	{
		SpriteCommand* sprite = Get(myReturnedSpriteCommands.Dequeue());
		if (!sprite) continue;
		ReturnTexture(sprite->myTexture);
		mySpriteCommandDict.Remove(sprite->myID);
	}*/
	for (unsigned short i = 0; i < myReturnedWorldSpriteCommands.Size(); i++)
	{
		WorldSpriteCommandID id = myReturnedWorldSpriteCommands.Dequeue();
		WorldSpriteCommand* worldSprite = GetW(id);
		if (!worldSprite) continue;
		ReturnTexture(worldSprite->myTexture);
		myWorldSpriteCmdDict.Remove(id);
	}
}

void Engine::TextureManager::ReleaseAllResources()
{
	for (std::pair<std::string, ID3D11ShaderResourceView*>textureRS : myTextures)
	{
		textureRS.second->Release();
		textureRS.second = nullptr;
	}

	myPooledPixelShaders.ForEach([](FixedString256&, ID3D11PixelShader*& v, CU::Dictionary<FixedString256, ID3D11PixelShader*>&)
	{
			v->Release();
	});
	myPooledPixelShaders.Clear();

	myPooledGeometeryShaders.ForEach([](FixedString256&, ID3D11GeometryShader*& v, CU::Dictionary<FixedString256, ID3D11GeometryShader*>&)
		{
			v->Release();
		});
	myPooledGeometeryShaders.Clear();

	myPooledVertexShaders.ForEach([](FixedString256&, ID3D11VertexShader*& v, CU::Dictionary<FixedString256, ID3D11VertexShader*>&)
		{
			v->Release();
		});
	myPooledVertexShaders.Clear();

	myPooledInputLayouts.ForEach([](FixedString256&, ID3D11InputLayout*& v, CU::Dictionary<FixedString256, ID3D11InputLayout*>&)
		{
			v->Release();
		});
	myPooledInputLayouts.Clear();

	myWorldSpriteCmdDict.ForEach([](auto, WorldSpriteCommand& cmd, auto)
	{
		cmd.ReleaseResources();
	});
	myWorldSpriteCmdDict.Clear();
}

void Engine::TextureManager::ExportTextureToFile(ID3D11Resource* aTexturePointer, ShortString aFileName)
{
	std::string sKey = aFileName.GetString();
	std::wstring name(sKey.begin(), sKey.end());
	const WCHAR* pwcsName = name.c_str();
	DirectX::SaveWICTextureToFile(myContext, aTexturePointer, GUID_ContainerFormatTiff, pwcsName);
}
