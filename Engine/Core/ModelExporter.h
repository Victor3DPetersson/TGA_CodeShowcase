#pragma once
class Model;
class ModelAnimated;
struct Material;

namespace Engine
{
	struct ModelExportData
	{
		unsigned char mySubModelName[128];
		unsigned char myMaterialName[128];
		unsigned int myNumberOfVertices;
		unsigned int myNumberOfIndices;
		unsigned int myOffset;
		unsigned int myStride;
		unsigned short myNumberOfSubmeshes;
		unsigned int myMaterialType;
	};
	struct ModelImportData;
	struct AnimationModelImportData;
	//Legacy function, only here for the internal Model Manager if artist create model through old viewer
	void ExportModelToEngine(Model* aModelInstance, ShortString aModelName, CU::GrowingArray<Material*>& someMaterials, bool aGenerateGUID = false);
	//Fucntion for the editor modelviewer to save to specified spot
	void ExportModelToEngineWithPath(Model* aModelInstance, ShortString aModelPath, bool aGenerateGUID);
	//When importing FBX to convert to own format without any materials
	void ExportModelToEngineNoMaterials(Model* aModelInstance, ShortString aModelName, bool aGenerateGUID = false);
	
	//Legacy function, only here for the internal Model Manager if artist create model through old viewer
	void ExportAnimatedModelToEngine(ModelAnimated* aModelInstance, ShortString aModelName, CU::GrowingArray<Material*>& someMaterials, bool aGenerateGUID = false);
	//Fucntion for the editor modelviewer to save to specified spot
	void ExportAnimatedModelToEngineWithPath(ModelAnimated* aModelInstance, ShortString aModelPath, bool aGenerateGUID);
	//When importing FBX to convert to own format without any materials
	void ExportAnimatedModelToEngineNoMaterials(ModelAnimated* aModelInstance, ShortString aModelName, bool aGenerateGUID = false);
}