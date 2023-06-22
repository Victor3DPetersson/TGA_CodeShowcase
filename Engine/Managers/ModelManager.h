#pragma once
#include <unordered_map>
#include <string>
#include "../../CommonUtilities/CU/Utility/ShortString.h"
#include "../../CommonUtilities/CU/Containers/Dictionary.h"
#include "../../CommonUtilities/CU/Containers/Queue.hpp"
#include "../GameObjects/Model.h"
#include "../GameObjects/ModelInstance.h"
#include "../Engine/Includes.h"
#include <rpc.h>
#include "../ECS\SerializedEnums.hpp"

enum class EGridOrientation
{
	eXY,
	eXZ,
	eYZ,
	ePlane
};
enum class ERenderTypes
{
	eSolid,
	eWireframe,
	eLine
};

struct ID3D11Device;
namespace Engine
{
	class Engine;
	class TextureManager;
	class MaterialManager;
	struct DeletionStruct
	{
		CU::GrowingArray<GUID>* modelsToDelete = nullptr;
		CU::Dictionary<FixedString256, int>* loadedModelsRefCounter = nullptr;
		CU::Dictionary<FixedString256, int>* loadedAnimatedModelsRefCounter = nullptr;
		CU::Dictionary<FixedString256, GUID>* loadedStringToModels = nullptr;
	};

	class ModelManager
	{
	public:
		ModelManager();
		//This is for updating all the data inside the ModelManager
		void Update();
		//void LoadUpdate();
		void Init(ID3D11Device* aDeviceContext, TextureManager* aTextureManagerPntr, MaterialManager* aMaterialManager);
		//Also loads the model
		GUID GetModel(const ShortString& aModelKey, bool aForceLoad);
		//Also loads the model
		GUID GetAnimatedModel(const ShortString& aModelKey);

		Model* GetModel(const GUID aGUID);
		ModelAnimated* GetAnimatedModel(const GUID aGUID);
		void IncrementModelCounter(const GUID aGUID);
		void DecrementModelCounter(const GUID aGUID);

		//CREATION
		/// Create the model and load it in to the memory
		bool LoadGratModel(const ShortString& aModelKey, bool aForceLoad);
		/// Create the Animated model and load it in to the memory
		bool LoadGratMotorikModel(const ShortString& aModelKey, bool aForceLoad);
		/// Create either an animated or a static model.
		bool LoadGrat(const ShortString& aModelKeyWithExtension, bool aForceLoad = true);
		//REMOVAL
		void RemoveModel(const ShortString& aModelToRemove);
		void RemoveAnimatedModel(const ShortString& aModelToRemove);
	
		GUID LoadPrimitive(const PrimitiveType aType);
		//Model* GetRenderTarget() { return &myRenderTarget; }
		MaterialManager* GetMaterialManager() { return myMaterialManager; }
		void SortAllMeshes();
		const size_t GetAmountOfModels() const { return myAmountOfSubModels; }
		const size_t GetAmountOfAnimModels() const { return myAmountOfAnimatedSubModels; }

		void ReleaseAllResources();

		FixedString256 GetFileName(UUID aGrat);
		FixedString256 GetFileNameAnim(UUID aGrat);

		void ReloadGratModel(UUID aGrat);
		void ReloadGratMotorikModel(UUID aGrat);

	private:
		bool UnloadModelFromEngine(const ShortString& aModelKey);
		bool LoadGratModelInternal(const ShortString& aModelKey);
		bool LoadGratMotorikModelInternal(const ShortString& aModelKey);

		bool CreateUnitCube();
		bool CreateUnitSphere();
		bool CreateUnitCylinder();
		bool AssemblePrimitveModelObject(const PrimitiveType aType, ModelData& someModelData, const CU::AABB3Df aBoundingVolume);
		void BuildCylinderTopCap(float topRadius, float height, UINT sliceCount, CU::GrowingArray<Vertex_PBR>& meshVertices, CU::GrowingArray<int>& meshIndeces);
		void BuildCylinderBottomCap(float bottomRadius, float height, UINT sliceCount, CU::GrowingArray<Vertex_PBR>& meshVertices, CU::GrowingArray<int>& meshIndeces);
		bool CreateGrid(const CU::Vector2f& aDimensions, const CU::Vector2ui& aDensity, const EGridOrientation aOrientation);
		GUID LoadModelGUIDFromFile(const ShortString& aModelKey);
		GUID LoadModelAnimatedGUIDFromFile(const ShortString& aModelKey);

		TextureManager* myTextureManager;
		MaterialManager* myMaterialManager;
		ID3D11Device* myDevice;

		CU::Dictionary<FixedString256, GUID> myLoadedModelsStringToModel;
		CU::Dictionary<FixedString256, int> myLoadedModelsRefCounter;
		CU::Dictionary<GUID, Model> myLoadedModels;
		//std::atomic<bool> myLockModelLoading;
		//std::atomic<bool> myFetchingLockModelLoading;

		std::unordered_map<std::string, GUID> myLoadedAnimatedModelsStringToModel;
		std::unordered_map<std::string, ModelAnimated> myLoadedAnimatedModels;
		CU::Dictionary<FixedString256, int> myLoadedAnimatedModelsRefCounter;
		//std::atomic<bool> myLockModelLoadingAnimated;
		//std::atomic<bool> myFetchingModelLoadingAnimated;


		CU::Queue<ShortString> myAddModelCommands;
		CU::Queue<ShortString> myAddAnimatedCommands;

		//CU::Queue<ShortString> myReloadCommands;
		CU::Queue<GUID> myReloadCommands;
		CU::Queue<ShortString> myReloadAnimatedCommands;
		CU::GrowingArray<GUID> myModelsToDelete;
		Model myBox;
		Model myCylinder;
		Model mySphere;
		Model myPlane;
		DeletionStruct myDeleteLoopStruct;

		/// ///// ---------------To Do Remove thisfilthy shit-........-
		Model myRenderTarget;

		unsigned short myAmountOfSubModels;
		unsigned short myAmountOfAnimatedSubModels;
		std::atomic<bool> myModelListsHaveChanged;
	};
}


