#pragma once
#include "../GameObjects\Material.h"
#include <map>
#include "..\CommonUtilities\CU\Containers\Queue.hpp"
#include "..\CommonUtilities\CU\Containers\Dictionary.h"
#include "../Ecs/SerializedEnums.hpp"
#include "../RenderConstants.hpp"
struct ID3D11Device;
struct ID3D11DeviceContext;
namespace Engine
{
	class FullScreenTexture;
	class GBuffer;

	class Engine;
	class TextureManager;
	class MaterialManager
	{
	public:
		void Update();
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, TextureManager* aTextureManager);
		Material* GetGratPlat(const ShortString& aGratKey, MaterialTypes aMaterialType);
		Material* GetGratPlat(const GUID& aGratKey, MaterialTypes aMaterialType);
		GUID GetGUID(const FixedString256& aGratKey, MaterialTypes aMaterialType);
		void ReloadMaterials();
		void ReloadMaterial(const ShortString& aGratKey, MaterialTypes aMaterialType);
		void ReleaseMaterials();
		void OnResize();
		FullScreenTexture** GetRenderTarget(int aIndex, RenderRes aTargetToGet) { return &myRenderTargets[aIndex][(int)aTargetToGet]; }
		FullScreenTexture** GetRenderTargetDepth(RenderRes aTargetToGet) { return &myRenderTargetDepthTextures[(int)aTargetToGet]; }
		FullScreenTexture** GetRenderTargetIntermediate(RenderRes aTargetToGet) { return &myRenderTargetIntermediateTextures[(int)aTargetToGet]; }
		GBuffer** GetRenderTargetGBuffer(RenderRes aTargetToGet) { return &myRenderTargetsGBuffer[(int)aTargetToGet]; }
	private:
		void ReloadAllMaterials();
		void ReloadMaterialsOfType(MaterialTypes aMaterialType);
		bool LoadMaterialFromMaterialName(const ShortString& aGratKey, MaterialTypes aMaterialType, Material& aMaterialToFill);
		bool LoadMaterial(Material& aMaterialToFill, std::wstring aMaterialPath);
		void CreateDefaultMaterials();
		ID3D11VertexShader* LoadVS(ShortString aPath, std::string& someVSData);
		ID3D11GeometryShader* LoadGS(ShortString aPath);
		ID3D11PixelShader* LoadPS(ShortString aPath);
		ID3D11InputLayout* GetInputLayout(MaterialTypes aMaterialType, std::string aVSData);
	private:
		//std::map<std::string, Material> myLoadedMaterials[(unsigned int)MaterialTypes::ECount];
		CU::Dictionary<GUID, Material> myLoadedMaterials[(unsigned int)MaterialTypes::ECount];
		CU::Dictionary<FixedString256, GUID> myLoadedStringsToGUID[(unsigned int)MaterialTypes::ECount];
		Material myDefaultMaterials[(unsigned int)MaterialTypes::ECount];
		TextureManager* myTextureManager;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		friend class Engine;
		bool myShouldReloadAllMaterials;
		bool myShouldReloadMaterial;
		FullScreenTexture* myRenderTargets[NUMBOF_RENDERTARGETS][(int)RenderRes::RenderRes_Count];
		FullScreenTexture* myRenderTargetDepthTextures[(int)RenderRes::RenderRes_Count];
		FullScreenTexture* myRenderTargetIntermediateTextures[(int)RenderRes::RenderRes_Count]; //this is for PostProcessing
		GBuffer* myRenderTargetsGBuffer[(int)RenderRes::RenderRes_Count];
		CU::Queue<std::pair<ShortString, MaterialTypes>> myMaterialsToReload;


		struct MaterialExportStruct
		{
			char materialName[128];
			char texturePaths[MAX_TEXTURE_AMOUNT][128];
			char myShaders[3][128];
		};
		void ExportMaterial(Material& aMatToExport, MaterialExportStruct aExportStruct);

	};
}


