#include "stdafx.h"
#include "ModelManager.h"
#include "Core/DirectXFramework.h"
#include <d3d11.h>
#include <fstream>
#include <assert.h>
#include "TextureManager.h"
#include "MaterialManager.h"
#include "Core/ModelFromFile_Importer.h"
#include "../CommonUtilities/CU/Containers/MinHeap.hpp"
#include "EngineInterface.h"
#include "Managers\Managers.h"
#include <RenderData.h>
#include "../Core/ModelExporter.h"

const float PI = 3.1415926f;
Engine::ModelManager::ModelManager()
{
	myAmountOfSubModels = 0;
	myAmountOfAnimatedSubModels = 0;
	myTextureManager = nullptr;
	myDevice = nullptr;
	myMaterialManager = nullptr;
	//myLockModelLoading = false;
	//myLockModelLoadingAnimated = false;
	//myFetchingLockModelLoading = false;
	//myFetchingModelLoadingAnimated = false;
}

void Engine::ModelManager::Init(ID3D11Device* aDeviceContext, TextureManager* aTextureManagerPntr, MaterialManager* aMaterialManager)
{
	myDevice = aDeviceContext;
	myTextureManager = aTextureManagerPntr;
	myMaterialManager = aMaterialManager;
	CreateUnitCube();
	CreateUnitCylinder();
	CreateUnitSphere();
	CreateGrid({1000, 1000}, { 10, 10 }, EGridOrientation::ePlane);
	ModelData& planeModelData = myLoadedModels.Get(LoadPrimitive(PrimitiveType::PrimitiveType_Plane))->GetModelData(0);
	ModelData rtModelData = ModelData(planeModelData.myNumberOfVertices, planeModelData.myNumberOfIndices, 1);
	rtModelData.myBoundingVolume = planeModelData.myBoundingVolume;
	rtModelData.myIndexBuffer = planeModelData.myIndexBuffer;
	memcpy(rtModelData.myIndices, planeModelData.myIndices, planeModelData.myNumberOfIndices);
	rtModelData.myNumberOfIndices = planeModelData.myNumberOfIndices;
	rtModelData.myNumberOfSubmeshes = planeModelData.myNumberOfSubmeshes;
	rtModelData.myNumberOfVertices = planeModelData.myNumberOfVertices;
	rtModelData.myOffset = planeModelData.myOffset;
	rtModelData.myStride = planeModelData.myStride;
	rtModelData.mySubModelName = "RT_Plane";
	rtModelData.myVertexBuffer = planeModelData.myVertexBuffer;
	memcpy(rtModelData.myVertices, planeModelData.myVertices, planeModelData.myNumberOfVertices);

	rtModelData.myMaterial = myMaterialManager->GetGratPlat("", MaterialTypes::ERenderTarget);
	Model renderTarget;  
	renderTarget.Init(rtModelData);
	renderTarget.AddModelData(rtModelData);

	GUID key = NIL_UUID;
	key.Data4[0] = (unsigned char)18 + 1;
	*myLoadedModels[key] = std::move(renderTarget);
	myModelsToDelete.Init(10);
	myDeleteLoopStruct.modelsToDelete = &myModelsToDelete;
	myDeleteLoopStruct.loadedModelsRefCounter = &myLoadedModelsRefCounter;
	myDeleteLoopStruct.loadedAnimatedModelsRefCounter = &myLoadedAnimatedModelsRefCounter;
	myDeleteLoopStruct.loadedStringToModels = &myLoadedModelsStringToModel;

	SortMeshBuffer();
}

GUID Engine::ModelManager::GetModel(const ShortString& aString, bool aForceLoad)
{
	assert(myDevice != nullptr && "Init Factory with DX device");
	if (myLoadedModelsStringToModel.Contains(aString.GetString()))
	{
		return *myLoadedModelsStringToModel[aString.GetString()];
	}
	GUID guid = NIL_UUID;
	if (aForceLoad)
	{
		if (LoadGratModelInternal(aString))
		{
			myModelListsHaveChanged.store(true);
			return *myLoadedModelsStringToModel.Get(aString.GetString());
		}
	}
	else
	{
		LoadGratModel(aString, false);
		return LoadModelGUIDFromFile(aString);
	}
	return guid;
}

GUID Engine::ModelManager::GetAnimatedModel(const ShortString& aModelKey)
{
	assert(myDevice != nullptr && "Init Factory with DX device");
	if (myLoadedAnimatedModelsStringToModel.find(aModelKey.GetString()) == myLoadedAnimatedModelsStringToModel.end())
	{
		LoadGratMotorikModel(aModelKey, false);
		return LoadModelAnimatedGUIDFromFile(aModelKey);
	}
	return myLoadedAnimatedModelsStringToModel[aModelKey.GetString()];
}

Model* Engine::ModelManager::GetModel(const GUID aGUID)
{
	assert(myDevice != nullptr && "Init Factory with DX device");

	Model* model = myLoadedModels.Get(aGUID);
	if (model)
	{
		return model;
	}
	return nullptr;
}
ModelAnimated* Engine::ModelManager::GetAnimatedModel(const GUID aGUID)
{
	assert(myDevice != nullptr && "Init Factory with DX device");
	for (auto object : myLoadedAnimatedModelsStringToModel)
	{
		if (object.second == aGUID)
		{
			return &myLoadedAnimatedModels[object.first];
		}
	}
	return nullptr;
}

void Engine::ModelManager::IncrementModelCounter(const GUID aGUID)
{
	//Model* model = myLoadedModels.Get(aGUID);
	//if (model){	
	//	if (model->myRefCounter < 0)
	//	{
	//		model->myRefCounter = 0;
	//	}
	//	model->myRefCounter++;	}
}

void Engine::ModelManager::DecrementModelCounter(const GUID aGUID)
{
	//Model* model = myLoadedModels.Get(aGUID);
	//if (model) { model->myRefCounter--; 
	//assert(model->myRefCounter >= 0 && "Reference counter should never be able to get negative");
	//}
}


void Engine::ModelManager::RemoveModel(const ShortString& aModelToRemove)
{
	//if (myLoadedModelsRefCounter.Contains(aModelToRemove.GetString()))
	//{
	//	int* test = myLoadedModelsRefCounter.Get(aModelToRemove.GetString());
	//	if (test && *test != 1)
	//	{
	//		*test = 1;
	//	}
	//}
}
void Engine::ModelManager::RemoveAnimatedModel(const ShortString& aModelToRemove)
{
	/*int* test = myLoadedAnimatedModelsRefCounter.Get(aModelToRemove.GetString());
	if (test && *test != 1)
	{
		*test = 1;
	}*/
}

void Engine::ModelManager::Update()
{
	bool ifHasChangedMeshes = false;
	
	if (myAddModelCommands.Size() > 0)
	{
		//while (myFetchingLockModelLoading) {}
		//myLockModelLoading = true;
		unsigned int size = myAddModelCommands.Size();
		for (size_t i = 0; i < size; i++)
		{
			ShortString model = myAddModelCommands.Dequeue();
			if (LoadGratModelInternal(model))
			{
				//Model& test = *myLoadedModels.Get(*myLoadedModelsStringToModel.Get(model.GetString()));
				myModelListsHaveChanged.store(true);
			}
			else
			{
				//assert(false && "Damn fuck");
			}
		}
		myAddModelCommands.ClearQueue();
		//myLockModelLoading = false;
	}
	if (myAddAnimatedCommands.Size() > 0)
	{
		//while(myFetchingModelLoadingAnimated) {}
		//myLockModelLoadingAnimated = true;
		unsigned int size = myAddAnimatedCommands.Size();
		for (size_t i = 0; i < size; i++)
		{
			ShortString model = myAddAnimatedCommands.Dequeue();
			if (LoadGratMotorikModelInternal(model))
			{
				myModelListsHaveChanged.store(true);
			}
			else
			{
				//assert(false && "Damn fuck");
			}
		}
		myAddAnimatedCommands.ClearQueue();
		//myLockModelLoadingAnimated = false;
	}

	myLoadedModels.ForEach([](GUID& key, Model& val, CU::Dictionary<GUID, Model>&, void* loopDeletionData)
		{
			DeletionStruct& data = *(DeletionStruct*)loopDeletionData;
			if (strlen(val.myName) > 0)
			{
				if (*data.loadedModelsRefCounter->Get(val.myName) == 1 && val.GetRefCounter() == 0)
				{
					data.modelsToDelete->Add(key);
				}
			}
		}, &myDeleteLoopStruct);
	
	if (myModelsToDelete.Size() > 0)
	{
	/*	unsigned short size = myModelsToDelete.Size();
		for (unsigned short i = 0; i < size; i++)
		{
			GUID ID = myModelsToDelete[i];
			FixedString256 name = myLoadedModels.Get(ID)->myName;
			myLoadedModelsStringToModel.Remove(name);
			myLoadedModelsRefCounter.Remove(name);
			Model* model = myLoadedModels[ID];
			model->ReleaseResources();
			myLoadedModels.Remove(ID);
		}
		myModelsToDelete.RemoveAll();
		ifHasChangedMeshes = true;*/
	}
	//if (myAnimatedRemoveCommands.Size() > 0)
	//{
	//	unsigned int size = myAnimatedRemoveCommands.Size();
	//	for (size_t i = 0; i < size; i++)
	//	{
	//		ShortString model = myAnimatedRemoveCommands.Dequeue();
	//		if (myLoadedAnimatedModelsStringToModel.find(model.GetString()) != myLoadedAnimatedModelsStringToModel.end())
	//		{
	//			GUID id = myLoadedAnimatedModelsStringToModel[model.GetString()];
	//			myLoadedAnimatedModelsStringToModel.erase(model.GetString());
	//			myLoadedAnimatedModels[model.GetString()].ReleaseResources();
	//			myLoadedAnimatedModels.erase(model.GetString());
	//		}
	//	}
	//	ifHasChangedMeshes = true;
	//}

	if (myReloadCommands.Size() > 0)
	{
		unsigned int size = myReloadCommands.Size();
		for (size_t i = 0; i < size; i++)
		{
			GUID model = myReloadCommands.Dequeue();
			LoadGratModelInternal(GetFileName(model).Data());
		}
		ifHasChangedMeshes = true;
	}

	if (myReloadAnimatedCommands.Size() > 0)
	{
		unsigned int size = myReloadAnimatedCommands.Size();
		for (size_t i = 0; i < size; i++)
		{
			ShortString model = myReloadAnimatedCommands.Dequeue();
			LoadGratMotorikModelInternal(model);
		}
		ifHasChangedMeshes = true;
	}

	if (ifHasChangedMeshes || myModelListsHaveChanged)
	{
		SortMeshBuffer();
		myModelListsHaveChanged.store(false);
	}
}

//void Engine::ModelManager::LoadUpdate()
//{
//	
//}

void Engine::ModelManager::ReleaseAllResources()
{
	myLoadedModels.ForEach([](GUID&, Model& cmd, CU::Dictionary<GUID, Model>&)
		{
			cmd.ReleaseResources();
		});

	for (auto& model : myLoadedAnimatedModelsStringToModel)
	{
		myLoadedAnimatedModels[model.first].ReleaseResources();
	}
}

FixedString256 Engine::ModelManager::GetFileName(UUID aGrat)
{
	struct UUIDToFind
	{
		FixedString256 returnName = "";
		GUID valToFind;
	} findStruct;
	findStruct.valToFind = aGrat;
	myLoadedModelsStringToModel.ForEach([](FixedString256&key, GUID& val, CU::Dictionary<FixedString256, GUID>&, void* nameToFind)
	{
		UUIDToFind& data = *(UUIDToFind*)nameToFind;
		if (val == data.valToFind)
		{
			data.returnName = key;
		}
	}, &findStruct);


	return findStruct.returnName;
}

FixedString256 Engine::ModelManager::GetFileNameAnim(UUID aGrat)
{
	for (auto& prutt : myLoadedAnimatedModelsStringToModel)
	{
		if (prutt.second == aGrat)
		{
			return prutt.first.c_str();
		}
	}

	return "Could not find file.";
}

void Engine::ModelManager::ReloadGratModel(UUID aGrat)
{
	myReloadCommands.Enqueue(aGrat);
}

void Engine::ModelManager::ReloadGratMotorikModel(UUID aGrat)
{
	FixedString256 name;
	for (auto& prutt : myLoadedAnimatedModelsStringToModel)
	{
		if (prutt.second == aGrat)
		{
			name = prutt.first.c_str();
			break;
		}
	}
	myReloadAnimatedCommands.Enqueue(name.Data());
}

bool Engine::ModelManager::LoadGratModelInternal(const ShortString& aModelKey)
{
	HRESULT result;
	CU::Matrix4x4f modelTransform;
	ModelFromFile_Importer modelImporter;
	ModelData* modelDataToFill = nullptr;
	unsigned short amountOfSubModels = 0;
	bool hasGUID;
	if (modelImporter.ImportGratFile(aModelKey, &modelDataToFill, amountOfSubModels, myMaterialManager, hasGUID))
	{
		Model model;
		CU::GrowingArray<Material*> materials;
		v3f min, max;
		model.Init(modelDataToFill[0]);
		unsigned short submodelCount = amountOfSubModels;
		materials.Init(submodelCount);
		for (unsigned short subModel = 0; subModel < submodelCount; subModel++)
		{
			v3f subMeshMin, subMeshMax;
			for (unsigned short i = 0; i < modelDataToFill[subModel].myNumberOfVertices; i++)
			{
				v3f vPos = modelDataToFill[subModel].myVertices[i].myPosition;
				if (vPos.x < subMeshMin.x) { subMeshMin.x = vPos.x; }
				if (vPos.y < subMeshMin.y) { subMeshMin.y = vPos.y; }
				if (vPos.z < subMeshMin.z) { subMeshMin.z = vPos.z; }
				if (vPos.x > subMeshMax.x) { subMeshMax.x = vPos.x; }
				if (vPos.y > subMeshMax.y) { subMeshMax.y = vPos.y; }
				if (vPos.z > subMeshMax.z) { subMeshMax.z = vPos.z; }
			}
			if (subMeshMin.x < min.x) { min.x = subMeshMin.x; }
			if (subMeshMin.y < min.y) { min.y = subMeshMin.y; }
			if (subMeshMin.z < min.z) { min.z = subMeshMin.z; }
			if (subMeshMax.x > max.x) { max.x = subMeshMax.x; }
			if (subMeshMax.y > max.y) { max.y = subMeshMax.y; }
			if (subMeshMax.z > max.z) { max.z = subMeshMax.z; }

			D3D11_BUFFER_DESC bufferDescription = { 0 };
			bufferDescription.ByteWidth = sizeof(modelDataToFill[subModel].myVertices[0]) * modelDataToFill[subModel].myNumberOfVertices;
			bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
			subresourceData.pSysMem = modelDataToFill[subModel].myVertices;

			ID3D11Buffer* vertexBuffer;
			result = myDevice->CreateBuffer(&bufferDescription, &subresourceData, &vertexBuffer);
			if (FAILED(result))
			{
				return false;
			}

			D3D11_BUFFER_DESC indexBufferDescription = { 0 };
			indexBufferDescription.ByteWidth = sizeof(modelDataToFill[subModel].myIndices[0]) * modelDataToFill[subModel].myNumberOfIndices;
			indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
			indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
			D3D11_SUBRESOURCE_DATA indexSubresourceData = { 0 };
			indexSubresourceData.pSysMem = modelDataToFill[subModel].myIndices;

			ID3D11Buffer* indexBuffer;
			result = myDevice->CreateBuffer(&indexBufferDescription, &indexSubresourceData, &indexBuffer);
			if (FAILED(result))
			{
				return false;
			}
			//Done Vertex
			//Load Data on model
			modelDataToFill[subModel].myBoundingVolume.InitWithMinAndMax(subMeshMin, subMeshMax);
			modelDataToFill[subModel].myVertexBuffer = vertexBuffer;
			modelDataToFill[subModel].myIndexBuffer = indexBuffer;
			modelDataToFill[subModel].myID = modelDataToFill[subModel].myModelIDCounter + (unsigned int)subMeshMax.x + (unsigned int)subMeshMax.y + (unsigned int)subMeshMax.z + modelDataToFill[subModel].myMaterial->myID;
			materials.Add(modelDataToFill[subModel].myMaterial);
			model.AddModelData(modelDataToFill[subModel]);
		}
		if (hasGUID)
		{
			myLoadedModelsStringToModel.Insert(aModelKey.GetString(), modelDataToFill[0].myGUID);//[aModelKey.GetString()] = modelDataToFill[0].myGUID;
		}
		else
		{
			ExportModelToEngine(&model, aModelKey, materials, true);
			modelImporter.ImportGratFile(aModelKey, &modelDataToFill, myAmountOfSubModels, myMaterialManager, hasGUID);
			myLoadedModelsStringToModel.Insert(aModelKey.GetString(), modelDataToFill[0].myGUID);//[aModelKey.GetString()] = modelDataToFill[0].myGUID;
		}
		CU::AABB3Df collider;
		collider.InitWithMinAndMax(min, max);
		model.SetCollider(collider);
		model.myName = aModelKey.GetString();
		*myLoadedModels[modelDataToFill[0].myGUID] = std::move(model);

		if (myLoadedModelsRefCounter.Contains(aModelKey.GetString()) == false)
		{
			myLoadedModelsRefCounter.Insert(aModelKey.GetString(), -1);
		}
		*myLoadedModelsRefCounter.Get(aModelKey.GetString()) = 0;
		return true;
	}
	return false;
}

bool Engine::ModelManager::LoadGratMotorikModelInternal(const ShortString& aModelKey)
{
	HRESULT result;
	CU::Matrix4x4f modelTransform;
	ModelFromFile_Importer modelImporter;
	ModelAnimated model;
	bool hasGUID = false;
	if (modelImporter.ImportGratMotorikFile(aModelKey, &model, myMaterialManager, hasGUID))
	{
		unsigned short submodelCount = model.myModelData.Size();
		CU::GrowingArray<Material*> materials;
		materials.Init(submodelCount);
		for (unsigned short subModel = 0; subModel < submodelCount; subModel++)
		{
			D3D11_BUFFER_DESC bufferDescription = { 0 };
			bufferDescription.ByteWidth = sizeof(model.myModelData[subModel].myVertices[0]) * model.myModelData[subModel].myNumberOfVertices;
			bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
			subresourceData.pSysMem = model.myModelData[subModel].myVertices;

			ID3D11Buffer* vertexBuffer;
			result = myDevice->CreateBuffer(&bufferDescription, &subresourceData, &vertexBuffer);
			if (FAILED(result))
			{
				return false;
			}

			D3D11_BUFFER_DESC indexBufferDescription = { 0 };
			indexBufferDescription.ByteWidth = sizeof(model.myModelData[subModel].myIndices[0]) * model.myModelData[subModel].myNumberOfIndices;
			indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
			indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
			D3D11_SUBRESOURCE_DATA indexSubresourceData = { 0 };
			indexSubresourceData.pSysMem = model.myModelData[subModel].myIndices;

			ID3D11Buffer* indexBuffer;
			result = myDevice->CreateBuffer(&indexBufferDescription, &indexSubresourceData, &indexBuffer);
			if (FAILED(result))
			{
				return false;
			}

			model.myModelData[subModel].myVertexBuffer = vertexBuffer;
			model.myModelData[subModel].myIndexBuffer = indexBuffer;
			materials.Add(model.myModelData[subModel].myMaterial);
		}
		if (!hasGUID)
		{
			ExportAnimatedModelToEngine(&model, aModelKey, materials, true);
			modelImporter.ImportGratMotorikFile(aModelKey, &model, myMaterialManager, hasGUID);
		}
		myLoadedAnimatedModelsStringToModel[aModelKey.GetString()] = model.myModelData[0].myGUID;
		myLoadedAnimatedModels[aModelKey.GetString()] = std::move(model);
		if (myLoadedAnimatedModelsRefCounter.Contains(aModelKey.GetString()) == false)
		{
			myLoadedAnimatedModelsRefCounter.Insert(aModelKey.GetString(), -1);
		}
		*myLoadedAnimatedModelsRefCounter.Get(aModelKey.GetString()) = 0;
		return true;
	}
	return false;
}

void Engine::ModelManager::SortAllMeshes()
{
	CU::MinHeap<ModelData*> sortedModels;
	myLoadedModels.ForEach([](GUID&, Model& cmd, CU::Dictionary<GUID, Model>&, void* listToFill)
		{
			CU::MinHeap<ModelData*>* list = (CU::MinHeap<ModelData*>*)listToFill;
			for (unsigned short subModel = 0; subModel < cmd.GetAmountOfSubModels(); subModel++)
			{
				ModelData* modelToAdd;
				modelToAdd = &cmd.myModelData[subModel];
				list->Enqueue(modelToAdd);
			}
		}, &sortedModels);


	myAmountOfSubModels = sortedModels.Size();
	assert("Not enough mesh slots, Increase MAX_SORTED_MESHES"  && myAmountOfSubModels < MAX_SORTED_MESHES);
	for (unsigned short i = 0; i < myAmountOfSubModels; i++)
	{
		ModelData* modelToIndex = sortedModels.Dequeue();
		modelToIndex->myRenderIndex = i;
	}
;

	CU::MinHeap<AnimatedModelData*> sortedAnimatedModels;
	for (auto& GuidData : myLoadedAnimatedModelsStringToModel)
	{
		ModelAnimated* model = &myLoadedAnimatedModels[GuidData.first];
		for (unsigned short subModel = 0; subModel < model->myModelData.Size(); subModel++)
		{
			AnimatedModelData* modelToAdd;

			modelToAdd = &model->myModelData[subModel];
			sortedAnimatedModels.Enqueue(modelToAdd);
		}
	}
	myAmountOfAnimatedSubModels = sortedAnimatedModels.Size();
	assert("Not enough animated mesh slots, Increase MAX_ANIM_SORTED_MESHES" && myAmountOfAnimatedSubModels < MAX_ANIM_SORTED_MESHES);
	for (unsigned short i = 0; i < myAmountOfAnimatedSubModels; i++)
	{
		AnimatedModelData* modelToIndex = sortedAnimatedModels.Dequeue();
		modelToIndex->myRenderIndex = i;
	}
}

bool Engine::ModelManager::LoadGratModel(const ShortString& aModelKey, bool aForceLoad)
{
	//if (myLockModelLoading) return false;
	bool contains = myLoadedModelsStringToModel.Contains(aModelKey.GetString());
	if (contains)
	{
		return true;
	}
	if (aForceLoad)
	{
		if (LoadGratModelInternal(aModelKey))
		{
			myModelListsHaveChanged.store(true);
			return true;
		}
		return false;
	}
	//if (myLockModelLoading) return false;
	//myFetchingLockModelLoading = true;
	int* test = myLoadedModelsRefCounter.Get(aModelKey.GetString());
	if (test)
	{
		if (*test != -1)
		{
			//myFetchingLockModelLoading = false;
			return true;
		}
		else
		{
			//myFetchingLockModelLoading = false;
			return false;
		}
	}
	myLoadedModelsRefCounter.Insert(aModelKey.GetString(), -1);
	myAddModelCommands.Enqueue(aModelKey);
	//myFetchingLockModelLoading = false;

	return false;
}

bool Engine::ModelManager::LoadGratMotorikModel(const ShortString& aModelKey, bool aForceLoad)
{
	//if (myLockModelLoadingAnimated) return false;
	auto t = myLoadedAnimatedModelsStringToModel.find(aModelKey.GetString());
	if (t != myLoadedAnimatedModelsStringToModel.end())
	{
		return true;
	}
	if (aForceLoad)
	{
		if (LoadGratMotorikModelInternal(aModelKey))
		{
			myModelListsHaveChanged.store(true);
			return true;
		}
	}
	int* test = nullptr;
	//if (myLockModelLoadingAnimated) return false;
	//myFetchingModelLoadingAnimated = true;
	if (myLoadedAnimatedModelsRefCounter.Contains(aModelKey.GetString()))
	{
		test = myLoadedAnimatedModelsRefCounter.Get(aModelKey.GetString());
	}
	if (test)
	{
		if (*test != -1)
		{
			//myFetchingModelLoadingAnimated = false;
			return true;
		}
		else
		{
			//myFetchingModelLoadingAnimated = false;
			return false;
		}
	}

	myAddAnimatedCommands.Enqueue(aModelKey);
	myLoadedAnimatedModelsRefCounter.Insert(aModelKey.GetString(), -1);
	//myFetchingModelLoadingAnimated = false;
	return false;
}

bool Engine::ModelManager::LoadGrat(const ShortString& aModelKeyWithExtension, bool aForceLoad)
{
	const char* prutt = strchr(aModelKeyWithExtension.GetString(), '.');
	if (!prutt) return false;

	ShortString key = aModelKeyWithExtension;

	if (strcmp(prutt, ".gratmotorik") == 0)
	{
		key.Data()[strlen(key.Data()) - 12] = '\0';
		if (LoadGratMotorikModel(key, aForceLoad))
		{
			myModelListsHaveChanged.store(true);
			return true;
		}
		return false;
	}

	key.Data()[strlen(key.Data()) - 5] = '\0';
	if(LoadGratModel(key, aForceLoad))
	{
		return true;
	}
	return false;
}

bool Engine::ModelManager::UnloadModelFromEngine(const ShortString& aModelKey)
{
	//GUID key = *myLoadedModelsStringToModel.Get(aModelKey.GetString());
	//myLoadedModelsStringToModel.Remove(aModelKey.GetString());
	//myLoadedModels.Remove(key);
	return true;
}

GUID Engine::ModelManager::LoadPrimitive(const PrimitiveType aType)
{
	GUID key = NIL_UUID;
	key.Data4[0] = (unsigned char)aType + 1;
	return key;
}

bool Engine::ModelManager::CreateUnitCube()
{
	float w = 50;
	float h = 50;
	float d = 50;

	//Create Vertices
	const int amountOfVertices = 24;
	ModelData modelData(amountOfVertices, 36, 1);
	Vertex_PBR v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24;

	CU::Vector3f toBinormal;
	//Back
	v1.myPosition = { -w, -h, -d };
	v1.myColor = { 128, 128, 0, 255 };
	v1.myUVCoords = { 0, 1 };
	v1.myNormal = { 0, 0, -1 };
	v1.myTangent = { 1, 0, 0 };
	toBinormal = { v1.myNormal.x, v1.myNormal.y, v1.myNormal.z };
	toBinormal = toBinormal.Cross({ v1.myTangent.x, v1.myTangent.y, v1.myTangent.z });
	v1.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v2.myPosition = { -w, h, -d };
	v2.myColor = { 128, 128, 0, 255 };
	v2.myUVCoords = { 0, 0 };
	v2.myNormal = { 0, 0, -1 };
	v2.myTangent = { 1, 0, 0 };
	toBinormal = { v2.myNormal.x, v2.myNormal.y, v2.myNormal.z };
	toBinormal = toBinormal.Cross({ v2.myTangent.x, v2.myTangent.y, v2.myTangent.z });
	v2.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v3.myPosition = { w, h, -d };
	v3.myColor = { 128, 128, 0, 255 };
	v3.myUVCoords = { 1, 0 };
	v3.myNormal = { 0, 0, -1 };
	v3.myTangent = { 1, 0, 0 };
	toBinormal = { v3.myNormal.x, v3.myNormal.y, v3.myNormal.z };
	toBinormal = toBinormal.Cross({ v3.myTangent.x, v3.myTangent.y, v3.myTangent.z });
	v3.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v4.myPosition = { w, -h, -d };
	v4.myColor = { 128, 128, 0, 255 };
	v4.myUVCoords = { 1, 1 };
	v4.myNormal = { 0, 0, -1 };
	v4.myTangent = { 1, 0, 0 };
	toBinormal = { v4.myNormal.x, v4.myNormal.y, v4.myNormal.z };
	toBinormal = toBinormal.Cross({ v4.myTangent.x, v4.myTangent.y, v4.myTangent.z });
	v4.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	//Front
	v5.myPosition = { -w, -h, d };
	v5.myColor = { 128, 128, 255, 255 };
	v5.myUVCoords = { 1, 1 };
	v5.myNormal = { 0, 0, 1 };
	v5.myTangent = { -1, 0, 0 };
	toBinormal = { v5.myNormal.x, v5.myNormal.y, v5.myNormal.z };
	toBinormal = toBinormal.Cross({ v5.myTangent.x, v5.myTangent.y, v5.myTangent.z });
	v5.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v6.myPosition = { w, -h, d };
	v6.myColor = { 128, 128, 255, 255 };
	v6.myUVCoords = { 0, 1 };
	v6.myNormal = { 0, 0, 1 };
	v6.myTangent = { -1, 0, 0 };
	toBinormal = { v6.myNormal.x, v6.myNormal.y, v6.myNormal.z };
	toBinormal = toBinormal.Cross({ v6.myTangent.x, v6.myTangent.y, v6.myTangent.z });
	v6.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v7.myPosition = { w, h, d };
	v7.myColor = { 128, 128, 255, 255 };
	v7.myUVCoords = { 0, 0 };
	v7.myNormal = { 0, 0, 1 };
	v7.myTangent = { -1, 0, 0 };
	toBinormal = { v7.myNormal.x, v7.myNormal.y, v7.myNormal.z };
	toBinormal = toBinormal.Cross({ v7.myTangent.x, v7.myTangent.y, v7.myTangent.z });
	v7.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v8.myPosition = { -w, h, d };
	v8.myColor = { 128, 128, 255, 255 };
	v8.myUVCoords = { 1, 0 };
	v8.myNormal = { 0, 0, 1 };
	v8.myTangent = { -1, 0, 0 };
	toBinormal = { v8.myNormal.x, v8.myNormal.y, v8.myNormal.z };
	toBinormal = toBinormal.Cross({ v8.myTangent.x, v8.myTangent.y, v8.myTangent.z });
	v8.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	//Top
	v9.myPosition = { -w, h, -d };
	v9.myColor = { 128, 255, 128, 255 };
	v9.myUVCoords = { 0, 1 };
	v9.myNormal = { 0, 1, 0 };
	v9.myTangent = { 1, 0, 0 };
	toBinormal = { v9.myNormal.x, v9.myNormal.y, v9.myNormal.z };
	toBinormal = toBinormal.Cross({ v9.myTangent.x, v9.myTangent.y, v9.myTangent.z });
	v9.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v10.myPosition = { -w, h, d };
	v10.myColor = { 128, 255, 128, 255 };
	v10.myUVCoords = { 0, 0 };
	v10.myNormal = { 0, 1, 0 };
	v10.myTangent = { 1, 0, 0 };
	toBinormal = { v10.myNormal.x, v10.myNormal.y, v10.myNormal.z };
	toBinormal = toBinormal.Cross({ v10.myTangent.x, v10.myTangent.y, v10.myTangent.z });
	v10.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v11.myPosition = { w, h, d };
	v11.myColor = { 128, 255, 128, 255 };
	v11.myUVCoords = { 1, 0 };
	v11.myNormal = { 0, 1, 0 };
	v11.myTangent = { 1, 0, 0 };
	toBinormal = { v11.myNormal.x, v11.myNormal.y, v11.myNormal.z };
	toBinormal = toBinormal.Cross({ v11.myTangent.x, v11.myTangent.y, v11.myTangent.z });
	v11.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v12.myPosition = { w, h, -d };
	v12.myColor = { 128, 255, 128, 255 };
	v12.myUVCoords = { 1, 1 };
	v12.myNormal = { 0, 1, 0 };
	v12.myTangent = { 1, 0, 0 };
	toBinormal = { v12.myNormal.x, v12.myNormal.y, v12.myNormal.z };
	toBinormal = toBinormal.Cross({ v12.myTangent.x, v12.myTangent.y, v12.myTangent.z });
	v12.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	//Bot
	v13.myPosition = { -w, -h, -d };
	v13.myColor = { 128, 0, 128, 255 };
	v13.myUVCoords = { 1, 1 };
	v13.myNormal = { 0, -1, 0 };
	v13.myTangent = { -1, 0, 0 };
	toBinormal = { v13.myNormal.x, v13.myNormal.y, v13.myNormal.z };
	toBinormal = toBinormal.Cross({ v13.myTangent.x, v13.myTangent.y, v13.myTangent.z });
	v13.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v14.myPosition = { w, -h, -d };
	v14.myColor = { 128, 0, 128, 255 };
	v14.myUVCoords = { 0, 1 };
	v14.myNormal = { 0, -1, 0 };
	v14.myTangent = { -1, 0, 0 };
	toBinormal = { v14.myNormal.x, v14.myNormal.y, v14.myNormal.z };
	toBinormal = toBinormal.Cross({ v14.myTangent.x, v14.myTangent.y, v14.myTangent.z });
	v14.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v15.myPosition = { w, -h, d };
	v15.myColor = { 128, 0, 128, 255 };
	v15.myUVCoords = { 0, 0 };
	v15.myNormal = { 0, -1, 0 };
	v15.myTangent = { -1, 0, 0 };
	toBinormal = { v15.myNormal.x, v15.myNormal.y, v15.myNormal.z };
	toBinormal = toBinormal.Cross({ v15.myTangent.x, v15.myTangent.y, v15.myTangent.z });
	v15.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v16.myPosition = { -w, -h, d };
	v16.myColor = { 128, 0, 128, 255 };
	v16.myUVCoords = { 1, 0 };
	v16.myNormal = { 0, -1, 0 };
	v16.myTangent = { -1, 0, 0 };
	toBinormal = { v16.myNormal.x, v16.myNormal.y, v16.myNormal.z };
	toBinormal = toBinormal.Cross({ v16.myTangent.x, v16.myTangent.y, v16.myTangent.z });
	v16.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	//Left
	v17.myPosition = { -w, -h, d };
	v17.myColor = { 0, 128, 128, 255 };
	v17.myUVCoords = { 0, 1 };
	v17.myNormal = { -1, 0, 0 };
	v17.myTangent = { 0, 0, -1 };
	toBinormal = { v17.myNormal.x, v17.myNormal.y, v17.myNormal.z };
	toBinormal = toBinormal.Cross({ v17.myTangent.x, v17.myTangent.y, v17.myTangent.z });
	v17.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v18.myPosition = { -w, h, d };
	v18.myColor = { 0, 128, 128, 255 };
	v18.myUVCoords = { 0, 0 };
	v18.myNormal = { -1, 0, 0 };
	v18.myTangent = { 0, 0, -1 };
	toBinormal = { v18.myNormal.x, v18.myNormal.y, v18.myNormal.z };
	toBinormal = toBinormal.Cross({ v18.myTangent.x, v18.myTangent.y, v18.myTangent.z });
	v18.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v19.myPosition = { -w, h, -d };
	v19.myColor = { 0, 128, 128, 255 };
	v19.myUVCoords = { 1, 0 };
	v19.myNormal = { -1, 0, 0 };
	v19.myTangent = { 0, 0, -1 };
	toBinormal = { v19.myNormal.x, v19.myNormal.y, v19.myNormal.z };
	toBinormal = toBinormal.Cross({ v19.myTangent.x, v19.myTangent.y, v19.myTangent.z });
	v19.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v20.myPosition = { -w, -h, -d };
	v20.myColor = { 0, 128, 128, 255 };
	v20.myUVCoords = { 1, 1 };
	v20.myNormal = { -1, 0, 0 };
	v20.myTangent = { 0, 0, -1 };
	toBinormal = { v20.myNormal.x, v20.myNormal.y, v20.myNormal.z };
	toBinormal = toBinormal.Cross({ v20.myTangent.x, v20.myTangent.y, v20.myTangent.z });
	v20.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	//Right
	v21.myPosition = { w, -h, -d };
	v21.myColor = { 255, 128, 128, 255 };
	v21.myUVCoords = { 0, 1 };
	v21.myNormal = { 1, 0, 0 };
	v21.myTangent = { 0, 0, 1 };
	toBinormal = { v21.myNormal.x, v21.myNormal.y, v21.myNormal.z };
	toBinormal = toBinormal.Cross({ v21.myTangent.x, v21.myTangent.y, v21.myTangent.z });
	v21.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v22.myPosition = { w, h, -d };
	v22.myColor = { 255, 128, 128, 255 };
	v22.myUVCoords = { 0, 0 };
	v22.myNormal = { 1, 0, 0 };
	v22.myTangent = { 0, 0, 1 };
	toBinormal = { v22.myNormal.x, v22.myNormal.y, v22.myNormal.z };
	toBinormal = toBinormal.Cross({ v22.myTangent.x, v22.myTangent.y, v22.myTangent.z });
	v22.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v23.myPosition = { w, h, d };
	v23.myColor = { 255, 128, 128, 255 };
	v23.myUVCoords = { 1, 0 };
	v23.myNormal = { 1, 0, 0 };
	v23.myTangent = { 0, 0, 1 };
	toBinormal = { v23.myNormal.x, v23.myNormal.y, v23.myNormal.z };
	toBinormal = toBinormal.Cross({ v23.myTangent.x, v23.myTangent.y, v23.myTangent.z });
	v23.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };

	v24.myPosition = { w, -h, d };
	v24.myColor = { 255, 128, 128, 255 };
	v24.myUVCoords = { 1, 1 };
	v24.myNormal = { 1, 0, 0 };
	v24.myTangent = { 0, 0, 1 };
	toBinormal = { v24.myNormal.x, v24.myNormal.y, v24.myNormal.z };
	toBinormal = toBinormal.Cross({ v24.myTangent.x, v24.myTangent.y, v24.myTangent.z });
	v24.myBinormal = { toBinormal.x, toBinormal.y, toBinormal.z };


	modelData.myVertices[0] = (v1);
	modelData.myVertices[1] = (v2);
	modelData.myVertices[2] = (v3);
	modelData.myVertices[3] = (v4);
	modelData.myVertices[4] = (v5);
	modelData.myVertices[5] = (v6);
	modelData.myVertices[6] = (v7);
	modelData.myVertices[7] = (v8);
	modelData.myVertices[8] = (v9);
	modelData.myVertices[9] = (v10);
	modelData.myVertices[10] = (v11);
	modelData.myVertices[11] = (v12);
	modelData.myVertices[12] = (v13);
	modelData.myVertices[13] = (v14);
	modelData.myVertices[14] = (v15);
	modelData.myVertices[15] = (v16);
	modelData.myVertices[16] = (v17);
	modelData.myVertices[17] = (v18);
	modelData.myVertices[18] = (v19);
	modelData.myVertices[19] = (v20);
	modelData.myVertices[20] = (v21);
	modelData.myVertices[21] = (v22);
	modelData.myVertices[22] = (v23);
	modelData.myVertices[23] = (v24);


	unsigned int indices[36] =
	{
		0,1,2,0,2,3,
		4,5,6,4,6,7,
		8,9,10,8,10,11,
		12,13,14,12,14,15,
		16,17,18,16,18,19,
		20,21,22,20,22,23
	};
	for (unsigned int i = 0; i < 36; i++)
	{
		modelData.myIndices[i] = indices[i];
	}
	CU::AABB3Df aBoundingVolume;
	aBoundingVolume.InitWithMinAndMax({ -50, -50, -50 }, { 50, 50, 50 });
	modelData.mySubModelName = "Unit Box";
	if (AssemblePrimitveModelObject(PrimitiveType::PrimitiveType_Cube, modelData, aBoundingVolume))
	{
		return true;
	}
	return false;
}

bool Engine::ModelManager::CreateUnitSphere()
{
	CU::AABB3Df aBoundingVolume;
	aBoundingVolume.InitWithMinAndMax({ -50, -50, -50 }, { 50, 50, 50 });
	float radius = 50.0f;
	UINT sliceCount = 12;
	UINT stackCount = 12;

	float phiStep = PI / stackCount;
	float thetaStep = 2.0f * PI / sliceCount;

	CU::GrowingArray<Vertex_PBR> vertices;
	vertices.Init(40);
	Vertex_PBR topVertex;
	topVertex.myPosition = { 0, radius, 0 };
	topVertex.myColor = { 255, 255,255,255 };
	topVertex.myUVCoords = { 0, 1 };
	vertices.Add(topVertex);

	for (UINT i = 1; i <= stackCount - 1; i++)
	{
		float phi = i * phiStep;
		for (UINT j = 0; j <= sliceCount; j++) {
			float theta = j * thetaStep;
			CU::Vector3f p =
			{
				(radius * sinf(phi) * cosf(theta)),
				(radius * cosf(phi)),
				(radius * sinf(phi) * sinf(theta))
			};
			CU::Vector3f tangent =
			{
				-radius * sinf(phi) * sinf(theta),
				0,
				radius * sinf(phi) * cosf(theta)
			};
			tangent.Normalize();
			CU::Vector3f n = { p.x, p.y, p.z };
			n.Normalize();
			CU::Vector3f binormal;
			binormal = tangent.Cross(n);
			binormal.Normalize();
			CU::Vector2f uv = { theta / (PI * 2), phi / PI };

			Vertex_PBR vertex;
			vertex.myPosition = p;
			vertex.myColor = { p.x * 255, p.y * 255 , p.z * 255 , 255 };
			vertex.myUVCoords = uv;
			vertex.myNormal = { n.x, n.y, n.z };
			vertex.myBinormal = { binormal.x, binormal.y, binormal.z };;
			vertex.myTangent = { tangent.x, tangent.y, tangent.z };;
			vertices.Add(vertex);
		}
	}
	Vertex_PBR botVertex;
	botVertex.myPosition = { 0, -radius, 0 };
	botVertex.myColor = { 0, 0,0,255 };
	botVertex.myUVCoords = { 0, 1 };
	botVertex.myNormal = { 0, -1, 0 };
	botVertex.myTangent = { 1, 0, 0 };
	CU::Vector3f tangent = { 1, 0, 0 };
	CU::Vector3f binormal = tangent.Cross({ 0, -1, 0 });
	botVertex.myBinormal = { binormal.x, binormal.y, binormal.z };

	vertices.Add(botVertex);

	CU::GrowingArray<UINT> vertexIndices;
	vertexIndices.Init(vertices.Size() * 3);

	for (UINT i = 1; i <= sliceCount; i++) {
		vertexIndices.Add(0);
		vertexIndices.Add(i + 1);
		vertexIndices.Add(i);
	}
	UINT baseIndex = 1;
	UINT ringVertexCount = sliceCount + 1;
	for (UINT i = 0; i < stackCount - 2; i++) {
		for (UINT j = 0; j < sliceCount; j++) {
			vertexIndices.Add(baseIndex + i * ringVertexCount + j);
			vertexIndices.Add(baseIndex + i * ringVertexCount + j + 1);
			vertexIndices.Add(baseIndex + (i + 1) * ringVertexCount + j);

			vertexIndices.Add(baseIndex + (i + 1) * ringVertexCount + j);
			vertexIndices.Add(baseIndex + i * ringVertexCount + j + 1);
			vertexIndices.Add(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}
	UINT southPoleIndex = vertices.Size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for (UINT i = 0; i < sliceCount; i++) {
		vertexIndices.Add(southPoleIndex);
		vertexIndices.Add(baseIndex + i);
		vertexIndices.Add(baseIndex + i + 1);
	}

	ModelData modelData(vertices.Size(), vertexIndices.Size(), 1);
	for (unsigned short i = 0; i < vertices.Size(); i++)
	{
		modelData.myVertices[i] = vertices[i];
	}
	for (unsigned short i = 0; i < vertexIndices.Size(); i++)
	{
		modelData.myIndices[i] = vertexIndices[i];
	}
	modelData.mySubModelName = "Unit Sphere";
	if (AssemblePrimitveModelObject(PrimitiveType::PrimitiveType_Sphere, modelData, aBoundingVolume))
	{
		return true;
	}
	return false;
}

bool Engine::ModelManager::CreateUnitCylinder()
{
	float bottomRadius, topRadius, height;
	UINT sliceCount, stackCount;
	bottomRadius = 50.f;
	topRadius = bottomRadius;
	height = 100;
	sliceCount = 16;
	stackCount = 8;
	CU::AABB3Df aBoundingVolume;
	aBoundingVolume.InitWithMinAndMax({ -50, -50, -50 }, { 50, 50, 50 });
	float stackHeight = height / (float)stackCount;
	float radiusStep = (topRadius - bottomRadius) / stackCount;
	UINT ringCount = stackCount + 1;

	//Vertices
	CU::GrowingArray<Vertex_PBR> vertices;
	vertices.Init(40);
	for (UINT i = 0; i < ringCount + 1; i++)
	{
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		float dTheta = 2.0f * PI / sliceCount;
		for (UINT j = 0; j <= sliceCount; j++)
		{
			Vertex_PBR vertex;

			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex.myPosition = { r * c, y, r * s };
			vertex.myUVCoords.x = (float)j / sliceCount;
			vertex.myUVCoords.y = 1.0f - (float)i / stackCount;
			vertex.myColor = { r * c * 255, y * 255, r * s * 255, 255 };
			vertex.myTangent = { -s, 0.0f, c };
			float dr = bottomRadius - topRadius;
			vertex.myBinormal = { dr * c, -height, dr * s };

			CU::Vector3f normal = { vertex.myTangent.x,vertex.myTangent.y,vertex.myTangent.z };

			normal = normal.Cross({ vertex.myBinormal.x,vertex.myBinormal.y,vertex.myBinormal.z });
			vertex.myNormal = { normal.x, normal.y, normal.z };

			vertices.Add(vertex);
		}
	}

	//Indices
	CU::GrowingArray<int> vertexIndices;
	vertexIndices.Init(vertices.Size() * 3);
	UINT ringVertexCount = sliceCount + 1;
	for (UINT i = 0; i < stackCount; i++)
	{
		for (UINT j = 0; j < sliceCount; j++)
		{
			vertexIndices.Add(i * ringVertexCount + j);
			vertexIndices.Add((i + 1) * ringVertexCount + j);
			vertexIndices.Add((i + 1) * ringVertexCount + j + 1);

			vertexIndices.Add(i * ringVertexCount + j);
			vertexIndices.Add((i + 1) * ringVertexCount + j + 1);
			vertexIndices.Add(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(topRadius, height, sliceCount, vertices, vertexIndices);
	BuildCylinderBottomCap(bottomRadius, height, sliceCount, vertices, vertexIndices);

	ModelData modelData(vertices.Size(), vertexIndices.Size(), 1);
	for (unsigned short i = 0; i < vertices.Size(); i++)
	{
		modelData.myVertices[i] = vertices[i];
	}
	for (unsigned short i = 0; i < vertexIndices.Size(); i++)
	{
		modelData.myIndices[i] = vertexIndices[i];
	}
	modelData.mySubModelName = "Unit Cylinder";
	if (AssemblePrimitveModelObject(PrimitiveType::PrimitiveType_Cylinder, modelData, aBoundingVolume))
	{
		return true;
	}
	return false;
}

bool Engine::ModelManager::AssemblePrimitveModelObject(const PrimitiveType aType, ModelData& someModelData, const CU::AABB3Df aBoundingVolume)
{
	HRESULT result;

	D3D11_BUFFER_DESC vertexBufferDescription = { 0 };
	vertexBufferDescription.ByteWidth = sizeof(someModelData.myVertices[0]) * someModelData.myNumberOfVertices;
	vertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
	subresourceData.pSysMem = someModelData.myVertices;

	ID3D11Buffer* vertexBuffer;
	result = myDevice->CreateBuffer(&vertexBufferDescription, &subresourceData, &vertexBuffer);

	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDescription = { 0 };
	indexBufferDescription.ByteWidth = sizeof(someModelData.myIndices[0]) * someModelData.myNumberOfIndices;
	indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA indexSubresourceData = { 0 };
	indexSubresourceData.pSysMem = someModelData.myIndices;

	ID3D11Buffer* indexBuffer;
	result = myDevice->CreateBuffer(&indexBufferDescription, &indexSubresourceData, &indexBuffer);
	if (FAILED(result))
	{
		return false;
	}
	//Done Vertex
	someModelData.myStride = sizeof(Vertex_PBR);
	someModelData.myOffset = 0;
	someModelData.myVertexBuffer = vertexBuffer;
	someModelData.myIndexBuffer = indexBuffer;
	someModelData.myMaterial = myMaterialManager->GetGratPlat("", MaterialTypes::EPBR);

	Model model;
	model.Init(someModelData);
	model.AddModelData(someModelData);
	model.myModelData[0].myBoundingVolume = aBoundingVolume;
	model.myAABB = aBoundingVolume;
	GUID key = NIL_UUID;
	key.Data4[0] = (unsigned char)aType + 1;
	*myLoadedModels[key] = std::move(model);
	someModelData.myVertexBuffer = nullptr;
	someModelData.myIndexBuffer = nullptr;
	return true;
}

void Engine::ModelManager::BuildCylinderTopCap(float topRadius, float height, UINT sliceCount, CU::GrowingArray<Vertex_PBR>& meshVertices, CU::GrowingArray<int>& meshIndeces)
{
	UINT baseIndex = (UINT)meshVertices.Size();

	float y = 0.5f * height;
	float dTheta = 2.0f * PI / sliceCount;
	for (UINT i = 0; i <= sliceCount; i++)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		Vertex_PBR vertex;
		vertex.myPosition = { x, y, z };
		vertex.myUVCoords = { u,v };
		vertex.myColor = { x * 255 * y, 255 * y, z * 255 * y, 255 };
		vertex.myTangent = { 0,0,1 };
		vertex.myBinormal = { 1,0,0 };
		vertex.myNormal = { 0,1,0 };

		meshVertices.Add(vertex);
	}
	//Cap Center Vertex
	Vertex_PBR topMidVertex;
	topMidVertex.myPosition = { 0.0f, y, 0.0f };
	topMidVertex.myUVCoords = { 0.5f, 0.5f };
	topMidVertex.myColor = { 255 * y, 255 * y, 255 * y,255 };
	topMidVertex.myTangent = { 0,0,1 };
	topMidVertex.myBinormal = { 1,0,0 };
	topMidVertex.myNormal = { 0,1,0 };
	meshVertices.Add(topMidVertex);
	UINT centerIndex = (UINT)meshVertices.Size() - 1;

	for (UINT i = 0; i < sliceCount; i++)
	{
		meshIndeces.Add(centerIndex);
		meshIndeces.Add(baseIndex + i + 1);
		meshIndeces.Add(baseIndex + i);
	}

}

void Engine::ModelManager::BuildCylinderBottomCap(float bottomRadius, float height, UINT sliceCount, CU::GrowingArray<Vertex_PBR>& meshVertices, CU::GrowingArray<int>& meshIndeces)
{
	UINT baseIndex = (UINT)meshVertices.Size();

	float y = -0.5f * height;
	float dTheta = 2.0f * PI / sliceCount;
	for (UINT i = 0; i <= sliceCount; i++)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		Vertex_PBR vertex;
		vertex.myPosition = { x, y, z };
		vertex.myUVCoords = { u,v };
		vertex.myColor = { x * 255 * y, 0 , z * 255 * y, 255 };
		vertex.myTangent = { 0,0,1 };
		vertex.myBinormal = { -1,0,0 };
		vertex.myNormal = { 0,-1,0 };
		meshVertices.Add(vertex);
	}
	//Cap Center Vertex
	Vertex_PBR botMiddleVertex;
	botMiddleVertex.myPosition = { 0.0f, y, 0.0f };
	botMiddleVertex.myUVCoords = { 0.5f, 0.5f };
	botMiddleVertex.myColor = { 255 * y, 0, 255 * y ,255 };
	botMiddleVertex.myTangent = { 0,0,1 };
	botMiddleVertex.myBinormal = { -1,0,0 };
	botMiddleVertex.myNormal = { 0,-1,0 };
	meshVertices.Add(botMiddleVertex);
	UINT centerIndex = (UINT)meshVertices.Size() - 1;

	for (UINT i = 0; i < sliceCount; i++)
	{
		meshIndeces.Add(centerIndex);
		meshIndeces.Add(baseIndex + i);
		meshIndeces.Add(baseIndex + i + 1);
	}
}

bool Engine::ModelManager::CreateGrid(const CU::Vector2f& aDimensions, const CU::Vector2ui& aDensity, const EGridOrientation aOrientation)
{
	ShortString name;
	CU::AABB3Df aBoundingVolume;
	float halfWidth = aDimensions.x * 0.5f;
	float halfDepth = aDimensions.y * 0.5f;
	switch (aOrientation)
	{
	case EGridOrientation::eXY:
		name = "GridXY";
		aBoundingVolume.InitWithMinAndMax({ -halfWidth, -1, -halfDepth }, { halfWidth, 1, halfDepth });
		break;
	case EGridOrientation::eXZ:
		aBoundingVolume.InitWithMinAndMax({ -halfWidth, -halfDepth, -1 }, { halfWidth, halfDepth, 1 });
		name = "GridXZ";
		break;
	case EGridOrientation::eYZ:
		aBoundingVolume.InitWithMinAndMax({ -1, -halfDepth, -halfWidth }, { 1, halfDepth, halfWidth });
		name = "GridYZ";
		break;
	case EGridOrientation::ePlane:
		name = "Plane";
		aBoundingVolume.InitWithMinAndMax({ -halfWidth, -1, -halfDepth }, { halfWidth, 1, halfDepth });
		break;
	default:
		break;
	}
	unsigned int vertexCount = aDensity.x * aDensity.y;
	unsigned int faceCount = (aDensity.x - 1) * (aDensity.y - 1) * 2;


	float dx = aDimensions.x / (aDensity.y - 1);
	float dz = aDimensions.y / (aDensity.x - 1);
	float du = 1.0f / (aDensity.x - 1);
	float dv = 1.0f / (aDensity.y - 1);
	CU::GrowingArray<Vertex_PBR, UINT> vertices;
	vertices.Init(vertexCount);
	for (UINT i = 0; i < vertexCount; i++)
	{
		vertices.Add(Vertex_PBR());
	}
	for (UINT i = 0; i < aDensity.x; i++)
	{
		float z = halfDepth - i * dz;
		for (UINT j = 0; j < aDensity.y; j++)
		{
			float x = -halfWidth + j * dx;
			Vertex_PBR vertex;
			switch (aOrientation)
			{
			case EGridOrientation::eXY:
				vertex.myPosition = { x, 0.0f, z };
				break;
			case EGridOrientation::eXZ:
				vertex.myPosition = { x, z,  0.0f };
				break;
			case EGridOrientation::eYZ:
				vertex.myPosition = { 0.0f, x, z };
				break;

			case EGridOrientation::ePlane:
				vertex.myPosition = { x, 0.0f, z };
				break;
			default:
				break;
			}
			vertex.myNormal = { 0.0, 1.0f, 0.0f };
			vertex.myTangent = { 1.0, 0.0f, 0.0f };
			vertex.myBinormal = vertex.myNormal.Cross(vertex.myTangent);
			vertex.myUVCoords = { j * du, i * dv };

			vertices[i * aDensity.x + j] = vertex;
		}
	}

	unsigned int numberOfIndices = faceCount * 3;
	unsigned int quadIterator = 0;
	CU::GrowingArray<unsigned int, UINT> indices;
	indices.Init(numberOfIndices);
	for (UINT i = 0; i < numberOfIndices; i++)
	{
		indices.Add(0);
	}
	for (UINT i = 0; i < aDensity.x - 1; i++)
	{
		for (UINT j = 0; j < aDensity.y - 1; j++)
		{
			indices[quadIterator] = i * aDensity.y + j;
			indices[quadIterator + 1] = i * aDensity.y + j + 1;
			indices[quadIterator + 2] = (i + 1) * aDensity.y + j;
			indices[quadIterator + 3] = (i + 1) * aDensity.y + j;
			indices[quadIterator + 4] = i * aDensity.y + j + 1;
			indices[quadIterator + 5] = (i + 1) * aDensity.y + j + 1;

			quadIterator += 6;
		}
	}
	ModelData modelData(vertexCount, numberOfIndices, 1);
	for (UINT i = 0; i < vertexCount; i++)
	{
		modelData.myVertices[i] = vertices[i];
	}
	for (UINT i = 0; i < numberOfIndices; i++)
	{
		modelData.myIndices[i] = indices[i];
	}
	modelData.mySubModelName = "Plane";
	if (AssemblePrimitveModelObject(PrimitiveType::PrimitiveType_Plane, modelData, aBoundingVolume))
	{
		return true;
	}
	return false;
}

GUID Engine::ModelManager::LoadModelGUIDFromFile(const ShortString& aModelKey)
{
	ModelFromFile_Importer modelImporter;
	return modelImporter.GetGUIDFromGratFile(aModelKey);
}

GUID Engine::ModelManager::LoadModelAnimatedGUIDFromFile(const ShortString& aModelKey)
{
	ModelFromFile_Importer modelImporter;
	return modelImporter.GetGUIDFromGratMotorikFile(aModelKey);
}
