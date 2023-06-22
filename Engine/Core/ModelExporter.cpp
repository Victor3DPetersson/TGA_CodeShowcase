#include "stdafx.h"
#include "ModelExporter.h"
#include "..\Engine\GameObjects\Material.h"
#include "..\Engine\GameObjects\ModelInstance.h"
#include "..\Engine\GameObjects\ModelData.h"
#include "..\Engine\GameObjects\Model.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <filesystem>
#include <stdio.h>
#include <iostream>
#include <cstdio>

static unsigned int versionNumb = 3;
void Engine::ExportModelToEngine(Model* aModelInstance, ShortString aModelName, CU::GrowingArray<Material*>& someMaterials, bool aGenerateGUID)
{
	const unsigned short numbModels = aModelInstance->GetAmountOfSubModels();
	std::string name = aModelName.GetString();
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::string outpathAbs = currentPath.u8string();
	outpathAbs.append("\\Content\\Models\\Static\\" + name + ".grat");
	if (std::filesystem::exists(outpathAbs))
	{
		std::filesystem::permissions(outpathAbs.c_str(), std::filesystem::perms::all);
		std::filesystem::remove(outpathAbs.c_str());
	}
	std::string outpath = "Content/Models/Static/";
	outpath.append(name);
	std::filesystem::create_directories(outpath);
	std::string modelPath = "Content\\Models\\Static\\" + name + ".grat";
	std::ofstream outModel;
	outModel.open(modelPath, std::ios::trunc | std::ios::out | std::ios::binary );
	outModel.write((char*)&versionNumb, sizeof(unsigned int));
	GUID id;
	if (aGenerateGUID)
	{
		UuidCreate(&id);
	}
	else
	{
		id = aModelInstance->GetModelData(0).myGUID;
	}
	outModel.write((char*)&id, sizeof(GUID));
	outModel.write((char*)&numbModels, sizeof(numbModels));

	for (unsigned short i = 0; i < numbModels; i++)
	{
		ModelExportData modelExportData;

		std::string subModelName = aModelInstance->GetModelData(i).mySubModelName.GetString();
		for (unsigned char c = 0;  c < 127; c++)
		{
			if (subModelName.size() > c)
			{
				modelExportData.mySubModelName[c] = subModelName[c];
			}
			else
			{
				modelExportData.mySubModelName[c] = 0;
			}
		}
		std::string materialName = someMaterials[i]->myMaterialName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (materialName.size() > c)
			{
				modelExportData.myMaterialName[c] = materialName[c];
			}
			else
			{
				modelExportData.myMaterialName[c] = 0;
			}
		}
		modelExportData.myMaterialName[127] = 0;
		modelExportData.mySubModelName[127] = 0;

		ModelData* modelData = &aModelInstance->GetModelData(i);
		modelExportData.myNumberOfIndices = (unsigned int)modelData->myNumberOfIndices;
		modelExportData.myNumberOfVertices = (unsigned int)modelData->myNumberOfVertices;

		switch (someMaterials[i]->myMaterialType)
		{
		case MaterialTypes::EPBR:
			modelExportData.myMaterialType = 0;
			break;
		case MaterialTypes::EPBR_Transparent:
			modelExportData.myMaterialType = 1;
			break;
		case MaterialTypes::EPBR_Anim:
			modelExportData.myMaterialType = 2;
			break;
		case MaterialTypes::EPBRTransparent_Anim:
			modelExportData.myMaterialType = 3;
			break;
		case MaterialTypes::EParticle_Default:
			modelExportData.myMaterialType = 4;
			break;
		case MaterialTypes::EParticle_Glow:
			modelExportData.myMaterialType = 5;
			break;
		case MaterialTypes::EDecal:
			modelExportData.myMaterialType = 6;
			break;
		default:
			modelExportData.myMaterialType = 0;
			break;
		}
		modelExportData.myOffset = modelData->myOffset;
		modelExportData.myStride = modelData->myStride;
		modelExportData.myNumberOfSubmeshes = modelData->myNumberOfSubmeshes;
		outModel.write((char*)&modelExportData, sizeof(ModelExportData));

		const unsigned int numberOfVerts = modelExportData.myNumberOfVertices;
		for (unsigned int v = 0; v < numberOfVerts; v++)
		{
			outModel.write((char*)&modelData->myVertices[v], sizeof(Vertex_PBR));
		}
		const unsigned int numberOfIndices = modelExportData.myNumberOfIndices;
		for (unsigned int index = 0; index < numberOfIndices; index++)
		{
			outModel.write((char*)&modelData->myIndices[index], sizeof(unsigned int));
		}
	}

	outModel.close();

}
void Engine::ExportModelToEngineWithPath(Model* aModelInstance, ShortString aModelPath, bool aGenerateGUID)
{
	const unsigned short numbModels = aModelInstance->GetAmountOfSubModels();
	if (std::filesystem::exists(aModelPath.GetString()))
	{
		std::filesystem::permissions(aModelPath.GetString(), std::filesystem::perms::all);
		std::filesystem::remove(aModelPath.GetString());
	}
	std::ofstream outModel;
	outModel.open(aModelPath.GetString(), std::ios::trunc | std::ios::out | std::ios::binary);
	outModel.write((char*)&versionNumb, sizeof(unsigned int));
	GUID id;
	if (aGenerateGUID)
	{
		UuidCreate(&id);
	}
	else
	{
		id = aModelInstance->GetModelData(0).myGUID;
	}
	outModel.write((char*)&id, sizeof(GUID));
	outModel.write((char*)&numbModels, sizeof(numbModels));

	for (unsigned short i = 0; i < numbModels; i++)
	{
		ModelExportData modelExportData;

		std::string subModelName = aModelInstance->GetModelData(i).mySubModelName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (subModelName.size() > c)
			{
				modelExportData.mySubModelName[c] = subModelName[c];
			}
			else
			{
				modelExportData.mySubModelName[c] = 0;
			}
		}
		std::string materialName = aModelInstance->GetModelData(i).myMaterial->myMaterialName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (materialName.size() > c)
			{
				modelExportData.myMaterialName[c] = materialName[c];
			}
			else
			{
				modelExportData.myMaterialName[c] = 0;
			}
		}
		modelExportData.myMaterialName[127] = 0;
		modelExportData.mySubModelName[127] = 0;

		ModelData* modelData = &aModelInstance->GetModelData(i);
		modelExportData.myNumberOfIndices = (unsigned int)modelData->myNumberOfIndices;
		modelExportData.myNumberOfVertices = (unsigned int)modelData->myNumberOfVertices;

		switch (aModelInstance->GetModelData(i).myMaterial->myMaterialType)
		{
		case MaterialTypes::EPBR:
			modelExportData.myMaterialType = 0;
			break;
		case MaterialTypes::EPBR_Transparent:
			modelExportData.myMaterialType = 1;
			break;
		case MaterialTypes::EPBR_Anim:
			modelExportData.myMaterialType = 2;
			break;
		case MaterialTypes::EPBRTransparent_Anim:
			modelExportData.myMaterialType = 3;
			break;
		case MaterialTypes::EParticle_Default:
			modelExportData.myMaterialType = 4;
			break;
		case MaterialTypes::EParticle_Glow:
			modelExportData.myMaterialType = 5;
			break;
		case MaterialTypes::EDecal:
			modelExportData.myMaterialType = 6;
			break;
		case MaterialTypes::ERenderTarget:
			modelExportData.myMaterialType = 7;
			break;
		default:
			modelExportData.myMaterialType = 0;
			break;
		}
		modelExportData.myOffset = modelData->myOffset;
		modelExportData.myStride = modelData->myStride;
		modelExportData.myNumberOfSubmeshes = modelData->myNumberOfSubmeshes;
		outModel.write((char*)&modelExportData, sizeof(ModelExportData));

		const unsigned int numberOfVerts = modelExportData.myNumberOfVertices;
		for (unsigned int v = 0; v < numberOfVerts; v++)
		{
			outModel.write((char*)&modelData->myVertices[v], sizeof(Vertex_PBR));
		}
		const unsigned int numberOfIndices = modelExportData.myNumberOfIndices;
		for (unsigned int index = 0; index < numberOfIndices; index++)
		{
			outModel.write((char*)&modelData->myIndices[index], sizeof(unsigned int));
		}
	}

	outModel.close();
}
void Engine::ExportModelToEngineNoMaterials(Model* aModelInstance, ShortString aModelName, bool aGenerateGUID)
{
	const unsigned short numbModels = aModelInstance->GetAmountOfSubModels();
	std::string name = aModelName.GetString();
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::string outpathAbs = currentPath.u8string();
	outpathAbs.append("\\Content\\Models\\Static\\" + name + ".grat");
	if (std::filesystem::exists(outpathAbs))
	{
		std::filesystem::permissions(outpathAbs.c_str(), std::filesystem::perms::all);
		std::filesystem::remove(outpathAbs.c_str());
	}
	std::string outpath = "Content/Models/Static/";
	outpath.append(name);
	//std::filesystem::create_directories(outpath);
	std::string modelPath = "Content\\Models\\Static\\" + name + ".grat";
	std::ofstream outModel;
	outModel.open(modelPath, std::ios::trunc | std::ios::out | std::ios::binary);
	outModel.write((char*)&versionNumb, sizeof(unsigned int));
	GUID id;
	if (aGenerateGUID)
	{
		UuidCreate(&id);
	}
	else
	{
		id = aModelInstance->GetModelData(0).myGUID;
	}
	outModel.write((char*)&id, sizeof(GUID));
	outModel.write((char*)&numbModels, sizeof(numbModels));

	for (unsigned short i = 0; i < numbModels; i++)
	{
		ModelExportData modelExportData;

		std::string subModelName = aModelInstance->GetModelData(i).mySubModelName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (subModelName.size() > c)
			{
				modelExportData.mySubModelName[c] = subModelName[c];
			}
			else
			{
				modelExportData.mySubModelName[c] = 0;
			}
		}
		for (unsigned char c = 0; c < 127; c++)
		{
			modelExportData.myMaterialName[c] = 0;
		}
		modelExportData.myMaterialName[127] = 0;
		modelExportData.mySubModelName[127] = 0;

		ModelData* modelData = &aModelInstance->GetModelData(i);
		modelExportData.myNumberOfIndices = (unsigned int)modelData->myNumberOfIndices;
		modelExportData.myNumberOfVertices = (unsigned int)modelData->myNumberOfVertices;
		modelExportData.myMaterialType = 0;
		modelExportData.myOffset = modelData->myOffset;
		modelExportData.myStride = modelData->myStride;
		modelExportData.myNumberOfSubmeshes = modelData->myNumberOfSubmeshes;
		outModel.write((char*)&modelExportData, sizeof(ModelExportData));

		const unsigned int numberOfVerts = modelExportData.myNumberOfVertices;
		for (unsigned int v = 0; v < numberOfVerts; v++)
		{
			outModel.write((char*)&modelData->myVertices[v], sizeof(Vertex_PBR));
		}
		const unsigned int numberOfIndices = modelExportData.myNumberOfIndices;
		for (unsigned int index = 0; index < numberOfIndices; index++)
		{
			outModel.write((char*)&modelData->myIndices[index], sizeof(unsigned int));
		}
	}

	outModel.close();
}
static unsigned int versionAnimNumb = 1;
void Engine::ExportAnimatedModelToEngine(ModelAnimated* aModelInstance, ShortString aModelName, CU::GrowingArray<Material*>& someMaterials, bool aGenerateGUID)
{
	const unsigned short numbModels = aModelInstance->GetAmountOfSubModels();

	std::filesystem::path currentPath = std::filesystem::current_path();
	std::string name = aModelName.GetString();
	std::string outpathAbs = currentPath.u8string();
	outpathAbs.append("\\Content\\Models\\Animated\\" + name +".gratmotorik");
	if (std::filesystem::exists(outpathAbs.c_str()))
	{
		std::filesystem::permissions(outpathAbs.c_str(), std::filesystem::perms::all);
		std::filesystem::remove(outpathAbs.c_str());
	}

	std::string outpath = "Content/Models/Animated/";
	outpath.append(name);
	//std::filesystem::create_directories(outpath);
	std::string modelPath = "Content/Models/Animated/" + name + ".gratmotorik";
	std::ofstream outModel;
	outModel.open(modelPath, std::ios::out | std::ios::binary);
	outModel.write((char*)&versionNumb, sizeof(unsigned int));
	GUID id;
	if (aGenerateGUID)
	{
		UuidCreate(&id);
	}
	else
	{
		id = aModelInstance->GetModelData(0).myGUID;
	}
	outModel.write((char*)&id, sizeof(GUID));

	outModel.write((char*)&numbModels, sizeof(numbModels));

	for (unsigned short i = 0; i < numbModels; i++)
	{
		ModelExportData modelExportData;

		std::string subModelName = aModelInstance->GetModelData(i).mySubModelName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (subModelName.size() > c)
			{
				modelExportData.mySubModelName[c] = subModelName[c];
			}
			else
			{
				modelExportData.mySubModelName[c] = 0;
			}
		}
		std::string materialName = someMaterials[i]->myMaterialName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (materialName.size() > c)
			{
				modelExportData.myMaterialName[c] = materialName[c];
			}
			else
			{
				modelExportData.myMaterialName[c] = 0;
			}
		}
		modelExportData.myMaterialName[127] = 0;
		modelExportData.mySubModelName[127] = 0;

		AnimatedModelData* modelData = &aModelInstance->GetModelData(i);
		modelExportData.myNumberOfIndices = (unsigned int)modelData->myNumberOfIndices;
		modelExportData.myNumberOfVertices = (unsigned int)modelData->myNumberOfVertices;

		switch (someMaterials[i]->myMaterialType)
		{
		case MaterialTypes::EPBR:
			modelExportData.myMaterialType = 0;
			break;
		case MaterialTypes::EPBR_Transparent:
			modelExportData.myMaterialType = 1;
			break;
		case MaterialTypes::EPBR_Anim:
			modelExportData.myMaterialType = 2;
			break;
		case MaterialTypes::EPBRTransparent_Anim:
			modelExportData.myMaterialType = 3;
			break;
		case MaterialTypes::EParticle_Default:
			modelExportData.myMaterialType = 4;
			break;
		case MaterialTypes::EParticle_Glow:
			modelExportData.myMaterialType = 5;
			break;
		case MaterialTypes::EDecal:
			modelExportData.myMaterialType = 6;
			break;
		default:
			modelExportData.myMaterialType = 2;
			break;
		}
		

		modelExportData.myOffset = modelData->myOffset;
		modelExportData.myStride = modelData->myStride;
		modelExportData.myNumberOfSubmeshes = modelData->myNumberOfSubmeshes;
		outModel.write((char*)&modelExportData, sizeof(ModelExportData));

		const unsigned int numberOfVerts = modelExportData.myNumberOfVertices;
		for (unsigned int v = 0; v < numberOfVerts; v++)
		{
			outModel.write((char*)&modelData->myVertices[v], sizeof(Vertex_PBR_Animated));
		}
		const unsigned int numberOfIndices = modelExportData.myNumberOfIndices;
		for (unsigned int index = 0; index < numberOfIndices; index++)
		{
			outModel.write((char*)&modelData->myIndices[index], sizeof(unsigned int));
		}
	}

	//GRÅT MOTORIK HÄR NEDANFÖR
	char exportString[128];
	outModel.write((char*)&versionAnimNumb, sizeof(unsigned int));
	AnimationData& anim = aModelInstance->GetAnimationData();

	unsigned int size;

	size = (unsigned int)anim.myBoneNameToIndex.size();
	outModel.write((char*)&size, sizeof(unsigned int));
	for (auto& boneToName : anim.myBoneNameToIndex)
	{
		std::string key = boneToName.first.GetString();
		size = (unsigned int)key.size();

		strcpy_s(exportString, key.c_str());
		outModel.write((char*)&size, sizeof(size));
		outModel.write((char*)&exportString[0], sizeof(char) * size);
		outModel.write((char*)&boneToName.second, sizeof(unsigned int));
	}

	size = anim.myIndexedSkeleton.Size();
	outModel.write((char*)&size, sizeof(unsigned int));
	for (unsigned short i = 0; i < anim.myIndexedSkeleton.Size(); i++)
	{
		Bone& bone = anim.myIndexedSkeleton[i];
		outModel.write((char*)&bone, sizeof(m4f));
	}

	unsigned int numberOfAnimations = anim.myAnimations.Size();
	outModel.write((char*)&numberOfAnimations, sizeof(unsigned int));
	for (unsigned int i = 0; i < numberOfAnimations; i++)
	{
		Animation& Animation = anim.myAnimations[i];
		std::string nameLength = aModelInstance->GetAnimationData().myAnimations[i].animationName.GetString();
		size = (unsigned int)nameLength.size();
		strcpy_s(exportString, nameLength.c_str());
		outModel.write((char*)&size, sizeof(unsigned int));
		outModel.write((char*)&exportString[0], sizeof(char) * size);
		outModel.write((char*)&Animation.duration, sizeof(float));
		outModel.write((char*)&Animation.ticksPerSec, sizeof(unsigned int));

		unsigned int frameCount = Animation.frames.Size();
		outModel.write((char*)&frameCount, sizeof(unsigned int));
		
		for (auto& frame : Animation.frames)
		{
			outModel.write((char*)&frame.myBonedata[0], anim.myIndexedSkeleton.Size() * sizeof(m4f));
		}
	}

	outModel.close();
}

void Engine::ExportAnimatedModelToEngineWithPath(ModelAnimated* aModelInstance, ShortString aModelPath, bool aGenerateGUID)
{
	const unsigned short numbModels = aModelInstance->GetAmountOfSubModels();
	if (std::filesystem::exists(aModelPath.GetString()))
	{
		std::filesystem::permissions(aModelPath.GetString(), std::filesystem::perms::all);
		std::filesystem::remove(aModelPath.GetString());
	}
	std::ofstream outModel;
	outModel.open(aModelPath.GetString(), std::ios::out | std::ios::binary);
	outModel.write((char*)&versionNumb, sizeof(unsigned int));
	GUID id;
	if (aGenerateGUID)
	{
		UuidCreate(&id);
	}
	else
	{
		id = aModelInstance->GetModelData(0).myGUID;
	}
	outModel.write((char*)&id, sizeof(GUID));

	outModel.write((char*)&numbModels, sizeof(numbModels));

	for (unsigned short i = 0; i < numbModels; i++)
	{
		ModelExportData modelExportData;

		std::string subModelName = aModelInstance->GetModelData(i).mySubModelName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (subModelName.size() > c)
			{
				modelExportData.mySubModelName[c] = subModelName[c];
			}
			else
			{
				modelExportData.mySubModelName[c] = 0;
			}
		}
		std::string materialName = aModelInstance->GetModelData(i).myMaterial->myMaterialName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (materialName.size() > c)
			{
				modelExportData.myMaterialName[c] = materialName[c];
			}
			else
			{
				modelExportData.myMaterialName[c] = 0;
			}
		}
		modelExportData.myMaterialName[127] = 0;
		modelExportData.mySubModelName[127] = 0;

		AnimatedModelData* modelData = &aModelInstance->GetModelData(i);
		modelExportData.myNumberOfIndices = (unsigned int)modelData->myNumberOfIndices;
		modelExportData.myNumberOfVertices = (unsigned int)modelData->myNumberOfVertices;

		switch (aModelInstance->GetModelData(i).myMaterial->myMaterialType)
		{
		case MaterialTypes::EPBR:
			modelExportData.myMaterialType = 0;
			break;
		case MaterialTypes::EPBR_Transparent:
			modelExportData.myMaterialType = 1;
			break;
		case MaterialTypes::EPBR_Anim:
			modelExportData.myMaterialType = 2;
			break;
		case MaterialTypes::EPBRTransparent_Anim:
			modelExportData.myMaterialType = 3;
			break;
		case MaterialTypes::EParticle_Default:
			modelExportData.myMaterialType = 4;
			break;
		case MaterialTypes::EParticle_Glow:
			modelExportData.myMaterialType = 5;
			break;
		case MaterialTypes::EDecal:
			modelExportData.myMaterialType = 6;
			break;
		default:
			modelExportData.myMaterialType = 2;
			break;
		}
		modelExportData.myOffset = modelData->myOffset;
		modelExportData.myStride = modelData->myStride;
		modelExportData.myNumberOfSubmeshes = modelData->myNumberOfSubmeshes;
		outModel.write((char*)&modelExportData, sizeof(ModelExportData));

		const unsigned int numberOfVerts = modelExportData.myNumberOfVertices;
		for (unsigned int v = 0; v < numberOfVerts; v++)
		{
			outModel.write((char*)&modelData->myVertices[v], sizeof(Vertex_PBR_Animated));
		}
		const unsigned int numberOfIndices = modelExportData.myNumberOfIndices;
		for (unsigned int index = 0; index < numberOfIndices; index++)
		{
			outModel.write((char*)&modelData->myIndices[index], sizeof(unsigned int));
		}
	}

	//GRÅT MOTORIK HÄR NEDANFÖR
	char exportString[128];
	outModel.write((char*)&versionAnimNumb, sizeof(unsigned int));
	AnimationData& anim = aModelInstance->GetAnimationData();

	unsigned int size;

	size = (unsigned int)anim.myBoneNameToIndex.size();
	outModel.write((char*)&size, sizeof(unsigned int));
	for (auto& boneToName : anim.myBoneNameToIndex)
	{
		std::string key = boneToName.first.GetString();
		size = (unsigned int)key.size();

		strcpy_s(exportString, key.c_str());
		outModel.write((char*)&size, sizeof(size));
		outModel.write((char*)&exportString[0], sizeof(char) * size);
		outModel.write((char*)&boneToName.second, sizeof(unsigned int));
	}

	size = anim.myIndexedSkeleton.Size();
	outModel.write((char*)&size, sizeof(unsigned int));
	for (unsigned short i = 0; i < anim.myIndexedSkeleton.Size(); i++)
	{
		Bone& bone = anim.myIndexedSkeleton[i];
		outModel.write((char*)&bone, sizeof(m4f));
	}

	unsigned int numberOfAnimations = anim.myAnimations.Size();
	outModel.write((char*)&numberOfAnimations, sizeof(unsigned int));
	for (unsigned int i = 0; i < numberOfAnimations; i++)
	{
		Animation& Animation = anim.myAnimations[i];
		std::string nameLength = aModelInstance->GetAnimationData().myAnimations[i].animationName.GetString();
		size = (unsigned int)nameLength.size();
		strcpy_s(exportString, nameLength.c_str());
		outModel.write((char*)&size, sizeof(unsigned int));
		outModel.write((char*)&exportString[0], sizeof(char) * size);
		outModel.write((char*)&Animation.duration, sizeof(float));
		outModel.write((char*)&Animation.ticksPerSec, sizeof(unsigned int));

		unsigned int frameCount = Animation.frames.Size();
		outModel.write((char*)&frameCount, sizeof(unsigned int));

		for (auto& frame : Animation.frames)
		{
			outModel.write((char*)&frame.myBonedata[0], anim.myIndexedSkeleton.Size() * sizeof(m4f));
		}
	}

	outModel.close();
}

void Engine::ExportAnimatedModelToEngineNoMaterials(ModelAnimated* aModelInstance, ShortString aModelName, bool aGenerateGUID)
{
	const unsigned short numbModels = aModelInstance->GetAmountOfSubModels();

	std::filesystem::path currentPath = std::filesystem::current_path();
	std::string name = aModelName.GetString();
	std::string outpathAbs = currentPath.u8string();
	outpathAbs.append("\\Content\\Models\\Animated\\" + name + ".gratmotorik");
	if (std::filesystem::exists(outpathAbs.c_str()))
	{
		std::filesystem::permissions(outpathAbs.c_str(), std::filesystem::perms::all);
		std::filesystem::remove(outpathAbs.c_str());
	}

	std::string outpath = "Content/Models/Animated/";
	outpath.append(name);
	//std::filesystem::create_directories(outpath);
	std::string modelPath = "Content/Models/Animated/" + name + ".gratmotorik";
	std::ofstream outModel;
	outModel.open(modelPath, std::ios::out | std::ios::binary);
	outModel.write((char*)&versionNumb, sizeof(unsigned int));
	GUID id;
	if (aGenerateGUID)
	{
		UuidCreate(&id);
	}
	else
	{
		id = aModelInstance->GetModelData(0).myGUID;
	}
	outModel.write((char*)&id, sizeof(GUID));

	outModel.write((char*)&numbModels, sizeof(numbModels));

	for (unsigned short i = 0; i < numbModels; i++)
	{
		ModelExportData modelExportData;

		std::string subModelName = aModelInstance->GetModelData(i).mySubModelName.GetString();
		for (unsigned char c = 0; c < 127; c++)
		{
			if (subModelName.size() > c)
			{
				modelExportData.mySubModelName[c] = subModelName[c];
			}
			else
			{
				modelExportData.mySubModelName[c] = 0;
			}
		}
		for (unsigned char c = 0; c < 127; c++)
		{
			modelExportData.myMaterialName[c] = 0;
		}
		modelExportData.myMaterialName[127] = 0;
		modelExportData.mySubModelName[127] = 0;

		AnimatedModelData* modelData = &aModelInstance->GetModelData(i);
		modelExportData.myNumberOfIndices = (unsigned int)modelData->myNumberOfIndices;
		modelExportData.myNumberOfVertices = (unsigned int)modelData->myNumberOfVertices;

		modelExportData.myMaterialType = 2;
		modelExportData.myOffset = modelData->myOffset;
		modelExportData.myStride = modelData->myStride;
		modelExportData.myNumberOfSubmeshes = modelData->myNumberOfSubmeshes;
		outModel.write((char*)&modelExportData, sizeof(ModelExportData));

		const unsigned int numberOfVerts = modelExportData.myNumberOfVertices;
		for (unsigned int v = 0; v < numberOfVerts; v++)
		{
			outModel.write((char*)&modelData->myVertices[v], sizeof(Vertex_PBR_Animated));
		}
		const unsigned int numberOfIndices = modelExportData.myNumberOfIndices;
		for (unsigned int index = 0; index < numberOfIndices; index++)
		{
			outModel.write((char*)&modelData->myIndices[index], sizeof(unsigned int));
		}
	}

	//GRÅT MOTORIK HÄR NEDANFÖR
	char exportString[128];
	outModel.write((char*)&versionAnimNumb, sizeof(unsigned int));
	AnimationData& anim = aModelInstance->GetAnimationData();

	unsigned int size;

	size = (unsigned int)anim.myBoneNameToIndex.size();
	outModel.write((char*)&size, sizeof(unsigned int));
	for (auto& boneToName : anim.myBoneNameToIndex)
	{
		std::string key = boneToName.first.GetString();
		size = (unsigned int)key.size();

		strcpy_s(exportString, key.c_str());
		outModel.write((char*)&size, sizeof(size));
		outModel.write((char*)&exportString[0], sizeof(char) * size);
		outModel.write((char*)&boneToName.second, sizeof(unsigned int));
	}

	size = anim.myIndexedSkeleton.Size();
	outModel.write((char*)&size, sizeof(unsigned int));
	for (unsigned short i = 0; i < anim.myIndexedSkeleton.Size(); i++)
	{
		Bone& bone = anim.myIndexedSkeleton[i];
		outModel.write((char*)&bone, sizeof(m4f));
	}

	unsigned int numberOfAnimations = anim.myAnimations.Size();
	outModel.write((char*)&numberOfAnimations, sizeof(unsigned int));
	for (unsigned int i = 0; i < numberOfAnimations; i++)
	{
		Animation& Animation = anim.myAnimations[i];
		std::string nameLength = aModelInstance->GetAnimationData().myAnimations[i].animationName.GetString();
		size = (unsigned int)nameLength.size();
		strcpy_s(exportString, nameLength.c_str());
		outModel.write((char*)&size, sizeof(unsigned int));
		outModel.write((char*)&exportString[0], sizeof(char) * size);
		outModel.write((char*)&Animation.duration, sizeof(float));
		outModel.write((char*)&Animation.ticksPerSec, sizeof(unsigned int));

		unsigned int frameCount = Animation.frames.Size();
		outModel.write((char*)&frameCount, sizeof(unsigned int));

		for (auto& frame : Animation.frames)
		{
			outModel.write((char*)&frame.myBonedata[0], anim.myIndexedSkeleton.Size() * sizeof(m4f));
		}
	}
	outModel.close();
}
