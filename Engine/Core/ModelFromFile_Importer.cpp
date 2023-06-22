#include "stdafx.h"
#include "Core/ModelFromFile_Importer.h"
#include <assert.h>

#include "assimp\Importer.hpp"
//#include "assimp\BaseImporter.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>    
#include "Managers/TextureManager.h"
#include "Managers/MaterialManager.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <filesystem>
#include <stdio.h>

#include "CU\Containers\Queue.hpp"

bool Engine::ModelFromFile_Importer::ImportGratFile(const ShortString& aModelName, ModelData** someModelDataToFill, unsigned short& aAmountOfSubModels, MaterialManager* aMaterialManager, bool& aHasGUID)
{
	std::string modelPath = "Content/Models/Static/";
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
	{
		std::string filename = dirEntry.path().filename().replace_extension().string();
		if (dirEntry.is_regular_file() && strcmp(filename.c_str(), aModelName.GetString()) == 0)
		{
			modelPath = dirEntry.path().relative_path().string().c_str();
			break;
		}
	}
	std::ifstream iMD;
	iMD.open(modelPath, std::ios::in | std::ios::binary);
	
	if (iMD)
	{
		unsigned int versionNumber = 0;
		iMD.read((char*)&versionNumber, sizeof(versionNumber));
		GUID GUID{};
		if (versionNumber < 3)
		{
			aHasGUID = false;
		}
		else
		{
			aHasGUID = true;
			iMD.read((char*)&GUID, sizeof(GUID));
		}
		unsigned short subModelCount = 0;

		iMD.read((char*)&subModelCount, sizeof(subModelCount));
		(*someModelDataToFill) = new ModelData[subModelCount];
		
		for (unsigned int subModel = 0; subModel < subModelCount; subModel++)
		{
			ModelImporttData importData;
			iMD.read((char*)&importData, sizeof(ModelImporttData));
			
			ModelData modelData = ModelData(importData.myNumberOfVertices, importData.myNumberOfIndices, subModelCount);
			modelData.myGUID = GUID;
			Vertex_PBR* vertices = (Vertex_PBR*)malloc(sizeof Vertex_PBR * importData.myNumberOfVertices);
			modelData.myVertices = new Vertex_PBR[importData.myNumberOfVertices];
			iMD.read((char*)&vertices[0], sizeof Vertex_PBR * importData.myNumberOfVertices);
			memcpy(&modelData.myVertices[0], &vertices[0], sizeof Vertex_PBR * importData.myNumberOfVertices);
			free(vertices);

			unsigned int* indices = (unsigned int*)malloc(sizeof(unsigned int) * importData.myNumberOfIndices);
			iMD.read((char*)&indices[0], sizeof(unsigned int) * importData.myNumberOfIndices);
			modelData.myIndices = new unsigned int[importData.myNumberOfIndices];
			memcpy(&modelData.myIndices[0], &indices[0], sizeof(unsigned int) * importData.myNumberOfIndices);
			free(indices);

			modelData.myOffset = importData.myOffset;
			modelData.myStride = importData.myStride;
			modelData.myMaterial = aMaterialManager->GetGratPlat(importData.myMaterialName, (MaterialTypes)importData.myMaterialType);
			modelData.mySubModelName = importData.mySubModelName;
			//someModelDataToFill.Add(modelData); 
			(*someModelDataToFill)[aAmountOfSubModels++] = std::move(modelData);
		}
		if (aHasGUID)
		{
			for (unsigned short subModel = 0; subModel < subModelCount; subModel++)
			{
				(*someModelDataToFill)[subModel].myGUID = GUID;
			}
		}
		return true;
	}
	
	return false;
}

bool Engine::ModelFromFile_Importer::ImportGratMotorikFile(const ShortString& aModelName, ModelAnimated* aModelToFill, MaterialManager* aMaterialManager, bool& aHasGUID)
{
	std::string modelPath = "Content/Models/Animated/";

	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
	{
		std::string filename = dirEntry.path().filename().replace_extension().string();
		if (dirEntry.is_regular_file() && strcmp(filename.c_str(), aModelName.GetString()) == 0)
		{
			modelPath = dirEntry.path().relative_path().string().c_str();
			break;
		}
	}
	std::ifstream iMD;
	iMD.open(modelPath, std::ios::in | std::ios::binary);
	if (iMD)
	{
		CU::GrowingArray<AnimatedModelData>& modelData = aModelToFill->myModelData;
		unsigned int versionNumber = 0;
		unsigned short subModelCount = 0;

		iMD.read((char*)&versionNumber, sizeof(versionNumber));
		GUID GUID{};
		if (versionNumber < 3)
		{
			aHasGUID = false;
		}
		else
		{
			aHasGUID = true;
			iMD.read((char*)&GUID, sizeof(GUID));
		}
		iMD.read((char*)&subModelCount, sizeof(subModelCount));
		if (versionNumber >= 2)
		{
			v3f min, max;
			modelData.ReInit(subModelCount);
			for (unsigned int subModel = 0; subModel < subModelCount; subModel++)
			{
				ModelImporttData importData;
				iMD.read((char*)&importData, sizeof(ModelImporttData));

				AnimatedModelData loadedModelData = AnimatedModelData(importData.myNumberOfVertices, importData.myNumberOfIndices, subModelCount);
				loadedModelData.myGUID = GUID;
				Vertex_PBR_Animated* vertices = (Vertex_PBR_Animated*)malloc(sizeof Vertex_PBR_Animated * importData.myNumberOfVertices);
				loadedModelData.myVertices = new Vertex_PBR_Animated[importData.myNumberOfVertices];
				iMD.read((char*)&vertices[0], sizeof Vertex_PBR_Animated * importData.myNumberOfVertices);
				memcpy(&loadedModelData.myVertices[0], &vertices[0], sizeof Vertex_PBR_Animated * importData.myNumberOfVertices);
				free(vertices);

				v3f subMeshMin, subMeshMax;
				for (unsigned short i = 0; i < importData.myNumberOfVertices; i++)
				{
					v3f vPos = loadedModelData.myVertices[i].myPosition;
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
				unsigned int* indices = (unsigned int*)malloc(sizeof(unsigned int) * importData.myNumberOfIndices);
				iMD.read((char*)&indices[0], sizeof(unsigned int) * importData.myNumberOfIndices);
				loadedModelData.myIndices = new unsigned int[importData.myNumberOfIndices];
				memcpy(&loadedModelData.myIndices[0], &indices[0], sizeof(unsigned int) * importData.myNumberOfIndices);
				free(indices);
				
				loadedModelData.myOffset = importData.myOffset;
				loadedModelData.myStride = importData.myStride;
				loadedModelData.myMaterial = aMaterialManager->GetGratPlat(importData.myMaterialName, (MaterialTypes)importData.myMaterialType);
				loadedModelData.mySubModelName = importData.mySubModelName;
				loadedModelData.myBoundingVolume.InitWithMinAndMax(subMeshMin, subMeshMax);
				loadedModelData.myID = loadedModelData.myAnimModelIDCounter + (unsigned int)subMeshMax.x + (unsigned int)subMeshMax.y + (unsigned int)subMeshMax.z + loadedModelData.myMaterial->myID;
				modelData.Add(loadedModelData);
				loadedModelData.myIndices = nullptr;
				loadedModelData.myVertices = nullptr;
			}
			CU::AABB3Df collider;
			collider.InitWithMinAndMax(min, max);
			aModelToFill->SetCollider(collider);
		}
		else
		{
			assert(false && "Wrong Model Version of file");
		}
		if (aHasGUID)
		{
			for (unsigned short subModel = 0; subModel < subModelCount; subModel++)
			{
				modelData[subModel].myGUID = GUID;
			}
		}
		//Läsa animations data här under
		AnimationData* animDataToFill = &aModelToFill->myAnimationData;

		unsigned int size = 0;
		unsigned int animVersionNumber = 0;
		iMD.read((char*)&animVersionNumber, sizeof(animVersionNumber));
		//Version number 1 is the first one

		unsigned int numberOfBoneToName = 0;
		iMD.read((char*)&numberOfBoneToName, sizeof(numberOfBoneToName));
		for (unsigned int i = 0; i < numberOfBoneToName; i++)
		{
			unsigned int numberOfChars = 0;
			iMD.read((char*)&numberOfChars, sizeof(numberOfChars));
			char name[128];
			iMD.read((char*)&name, sizeof(char) * numberOfChars);
			name[numberOfChars] = 0;
			unsigned int val = 0;
			iMD.read((char*)&val, sizeof(val));
			animDataToFill->myBoneNameToIndex[name] = val;
		}

		unsigned int numberOfBones = 0;
		iMD.read((char*)&numberOfBones, sizeof(numberOfBones));
		if (numberOfBones > 0)
		{
			if (animDataToFill->myIndexedSkeleton.IsInitialized())
			{
				animDataToFill->myIndexedSkeleton.ReInit((unsigned short)numberOfBones);
			}
			else
			{
				animDataToFill->myIndexedSkeleton.Init((unsigned short)numberOfBones);
			}
		}
		for (unsigned int b = 0; b < numberOfBones; b++)
		{
			Bone bone;
			iMD.read((char*)&bone, sizeof(m4f));
			animDataToFill->myIndexedSkeleton.Add(bone);
		}

		unsigned int numberOfAnimations = 0;
		iMD.read((char*)&numberOfAnimations, sizeof(numberOfAnimations));
		if (animDataToFill->myAnimations.IsInitialized())
		{
			animDataToFill->myAnimations.ReInit((unsigned short)numberOfAnimations);
		}
		else
		{
			animDataToFill->myAnimations.Init((unsigned short)numberOfAnimations);
		}
		for (unsigned int i = 0; i < numberOfAnimations; i++)
		{
			char name[128] = "";

			Animation animation;
			unsigned int numberOfChars = 0;
			iMD.read((char*)&numberOfChars, sizeof(numberOfChars));
			iMD.read((char*)&name[0], sizeof(char) * numberOfChars);
			iMD.read((char*)&animation.duration, sizeof(float));
			iMD.read((char*)&animation.ticksPerSec, sizeof(unsigned int));

			animation.animationName = ShortString(name);

			unsigned int numberOfFrames = 0;
			iMD.read((char*)&numberOfFrames, sizeof(numberOfFrames));

			animation.frames.Init(numberOfFrames);
			animation.frames.Fill();

			for (unsigned int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++)
			{
				animation.frames[frameIndex].myBonedata.Init((unsigned short)numberOfBones);
				animation.frames[frameIndex].myBonedata.Fill();
				iMD.read((char*)&animation.frames[frameIndex].myBonedata[0], numberOfBones * sizeof(m4f));
				
			}
			animDataToFill->myAnimations.Add(animation);
		}

		for (unsigned short i = 0; i < animDataToFill->myAnimations.Size(); i++)
		{
			animDataToFill->myAnimationNameToIndex[animDataToFill->myAnimations[i].animationName.GetString()] = i;
		}
		return true;
	}
	return false;
}

GUID Engine::ModelFromFile_Importer::GetGUIDFromGratFile(const ShortString& aModelName)
{
	std::string modelPath = "Content/Models/Static/";
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
	{
		std::string filename = dirEntry.path().filename().replace_extension().string();
		if (dirEntry.is_regular_file() && strcmp(filename.c_str(), aModelName.GetString()) == 0)
		{
			modelPath = dirEntry.path().relative_path().string().c_str();
			break;
		}
	}
	std::ifstream iMD;
	iMD.open(modelPath, std::ios::in | std::ios::binary);
	if (iMD)
	{
		unsigned int versionNumber = 0;
		iMD.read((char*)&versionNumber, sizeof(versionNumber));
		GUID GUID = NIL_UUID;
		if (versionNumber < 3)
		{
		}
		else
		{
			iMD.read((char*)&GUID, sizeof(GUID));
		}
		iMD.close();
		return GUID;
	}
	return NIL_UUID;
}

GUID Engine::ModelFromFile_Importer::GetGUIDFromGratMotorikFile(const ShortString& aModelName)
{
	std::string modelPath = "Content/Models/Animated/";

	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
	{
		std::string filename = dirEntry.path().filename().replace_extension().string();
		if (dirEntry.is_regular_file() && strcmp(filename.c_str(), aModelName.GetString()) == 0)
		{
			modelPath = dirEntry.path().relative_path().string().c_str();
			break;
		}
	}
	std::ifstream iMD;
	iMD.open(modelPath, std::ios::in | std::ios::binary);
	if (iMD)
	{
		unsigned int versionNumber = 0;
		iMD.read((char*)&versionNumber, sizeof(versionNumber));
		GUID GUID = NIL_UUID;
		if (versionNumber < 3)
		{
		}
		else
		{
			iMD.read((char*)&GUID, sizeof(GUID));
		}
		iMD.close();
		return GUID;
	}
	return NIL_UUID;
}

