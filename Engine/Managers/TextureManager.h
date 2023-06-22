#pragma once
#include <string>
#include <unordered_map>
#include "../../CommonUtilities/CU\Utility\ShortString.h"
#include "../../CommonUtilities/CU\Math/Vector2.hpp"
#include "../CommonUtilities/CU/Utility/FixedString.hpp"
#include <queue>
#include "../CommonUtilities/CU\Containers\Dictionary.h"
#include "../CommonUtilities/CU\Containers\VectorOnStack.h"
#include "../CommonUtilities/CU\Containers\Queue.hpp"
#include "../Engine/GameObjects/Sprite.h"
#include "../RenderConstants.hpp"

using SpriteCommandID = size_t;
using WorldSpriteCommandID = size_t;

class WorldSpriteCommand;
struct ID3D11ShaderResourceView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Resource;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11VertexShader;
struct ID3D11InputLayout;
namespace MV
{
	class Core;
	class ModelFromFile_Importer;
}

enum class ETextureTypes
{
	eColor,
	eNormal,
	eMaterial,
	eEmissive,
	eEnvironment
};

namespace Engine
{
	class FullScreenTexture;
	constexpr unsigned int NUMB_SCREENSPRITES = 4096;
	constexpr unsigned int NUMB_WORLDSPRITES = 2048;
	class TextureManager
	{
	public:
		TextureManager() {
			myDevice = nullptr; myContext = nullptr;
		}
		~TextureManager();
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext);

		ID3D11ShaderResourceView* GetTextureObject(const ShortString& aTexturePath = "", ETextureTypes aTextureType = ETextureTypes::eColor);
		SpriteID GetSpriteID(const char* const aPath);
		WorldSpriteCommandID CreateWorldSprite(const ShortString& aTexturePath,
			const char* const aCustomPS = nullptr,
			const char* const aCustomVS = nullptr,
			const char* const aCustomGS = nullptr
		);
		CU::Vector2ui GetImageDimensions(ID3D11ShaderResourceView* aSRV);
		inline v2ui GetSpriteDimensions(SpriteID spr)
		{
			return GetImageDimensions(GetTextureObject((const char*) * mySpriteIDPathDict.Get(spr)));
		}

		inline v2ui* GetSpriteDimensionsSafe(SpriteID spr)
		{
			if (mySizeCacheLock.test_and_set())
			{
				return 0;
			}
			mySizeCacheLock.clear();
			return mySizeCache.Get(spr);
		}
	
		void ReturnTexture(ID3D11ShaderResourceView* aTexturePointer, bool aDeleteTextureResource = false);
		//void ReturnSprite(SpriteCommandID const aSprCmd);
		void ReturnWorldSprite(WorldSpriteCommandID const aSprCmd);

		void Flush();
		void ReleaseAllResources();
		void ExportTextureToFile(ID3D11Resource* aTexturePointer, ShortString aFileName);

		__forceinline WorldSpriteCommand* GetW(WorldSpriteCommandID id)
		{
			return myWorldSpriteCmdDict.Get(id);
		}

		CU::Dictionary<FixedString256, ID3D11PixelShader*> myPooledPixelShaders;
		CU::Dictionary<FixedString256, ID3D11GeometryShader*> myPooledGeometeryShaders;
		CU::Dictionary<FixedString256, ID3D11VertexShader*> myPooledVertexShaders;
		CU::Dictionary<FixedString256, ID3D11InputLayout*> myPooledInputLayouts;

		inline const FixedString256& GetSpritePath(SpriteID id)
		{
			FixedString256* ptr = mySpriteIDPathDict.Get(id);
			return *ptr;
		}

		Vertex_Sprite myVtxSpr;
		ID3D11Buffer* myVtxBuf;

		CU::Dictionary<SpriteID, FixedString256> mySpriteIDPathDict;
		CU::Dictionary<FixedString256, SpriteID> myPathSpriteIDDict;
		CU::Dictionary<SpriteID, v2ui> mySizeCache;
		std::atomic_flag mySizeCacheLock = ATOMIC_FLAG_INIT;

	//private:
		friend class MV::Core;
		friend class MV::ModelFromFile_Importer;
		bool ImportExternalTexture(const std::string& aTexturePath);
		ID3D11ShaderResourceView* CreateTextureObject(const ShortString& aTexturePath);
		bool CreateInternalTextureObject(const std::string& aTexturePath);
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		std::unordered_map<std::string, ID3D11ShaderResourceView*> myTextures;
		std::unordered_map<std::string, unsigned int> myTextureCounter;

		static size_t mySpriteCommandIDCounter;
		static size_t myWSpriteCommandIDCounter;
		/*CU::Dictionary<SpriteCommandID, SpriteCommand> mySpriteCommandDict;
		CU::Queue<SpriteCommandID, NUMB_SCREENSPRITES> myReturnedSpriteCommands;*/
		CU::Dictionary<WorldSpriteCommandID, WorldSpriteCommand> myWorldSpriteCmdDict;
		CU::Queue<WorldSpriteCommandID, NUMB_WORLDSPRITES> myReturnedWorldSpriteCommands;
	};
}