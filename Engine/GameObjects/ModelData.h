#pragma once
#include "Vertex.h"
#include "../Includes.h"
#include "../CommonUtilities/CU/Collision/AABB3D.hpp"
#include "../CommonUtilities/CU/Math/Matrix.hpp"
#include <d3d11.h>
#include "Material.h"
#include <map>
namespace Engine
{
	class ModelManager;
	class ModelFromFile_Importer;
}
enum EModelType_
{
	EModelType_STATIC = 1 << 0,
	EModelType_NORMAL = 1 << 1,
	EModelType_OUTLINE = 1 << 2,
	EModelType_HOVERED = 1 << 3,
	EModelType_COUNT = 1 << 4
};
enum EAnimatedModelType_
{
	EAnimatedModelType_NORMAL,
	EAnimatedModelType_OUTLINE_ENEMY,
	EAnimatedModelType_PLAYER,
	EAnimatedModelType_HOVERED,
	EAnimatedModelType_COUNT
};
struct ModelImporttData
{
	//V2//////////
	char mySubModelName[128];
	char myMaterialName[128];
	unsigned int myNumberOfVertices;
	unsigned int myNumberOfIndices;
	unsigned int myOffset;
	unsigned int myStride;
	unsigned short myNumberOfSubmeshes;
	unsigned int myMaterialType;
};

struct ModelData
{
	ModelData()
	{
		myModelIDCounter++;
		myNumberOfSubmeshes = 0;
		myNumberOfVertices = 0;
		myNumberOfIndices = 0;
		myVertices = nullptr;
		myIndices = nullptr;
		myOffset = 0;
		myStride = sizeof(Vertex_PBR);
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
		myMaterial = nullptr;
		myID = 0;
		myRenderIndex = 0;
		myGUID = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	}
	ModelData(unsigned short aNumbOfSubmeshes) : myNumberOfSubmeshes(aNumbOfSubmeshes)
	{
		myModelIDCounter++;
		myNumberOfVertices = 0;
		myNumberOfIndices = 0;
		myVertices = nullptr;
		myIndices = nullptr;
		myOffset = 0;
		myStride = sizeof(Vertex_PBR);
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
		myMaterial = nullptr;
		myID = 0;
		myRenderIndex = 0;
		myGUID = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	}
	ModelData(unsigned int aNumbVertices, unsigned int aNumberOfIndices, unsigned short aAmountOfSubMeshes) : 
		myNumberOfVertices(aNumbVertices), 
		myNumberOfIndices(aNumberOfIndices), 
		myNumberOfSubmeshes(aAmountOfSubMeshes)
	{
		myModelIDCounter++;
		myVertices = new Vertex_PBR[aNumbVertices];
		myIndices = new unsigned int[aNumberOfIndices];
		myOffset = 0;
		myStride = sizeof(Vertex_PBR);
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
		myMaterial = nullptr;
		myID = 0;
		myRenderIndex = 0;
		myGUID = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	}
	ModelData(const ModelData& someData) {
		mySubModelName		= someData.mySubModelName;
		myVertices			= someData.myVertices;
		myIndices			= someData.myIndices;
		myOffset			= someData.myOffset;
		myNumberOfVertices	= someData.myNumberOfVertices;
		myNumberOfIndices	= someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride			= someData.myStride;
		myVertexBuffer		= someData.myVertexBuffer;
		myIndexBuffer		= someData.myIndexBuffer;
		myMaterial			= someData.myMaterial;
		myID				= someData.myID;
		myBoundingVolume	= someData.myBoundingVolume;
		myRenderIndex		= someData.myRenderIndex;
		myGUID				= someData.myGUID;
	}
	ModelData(ModelData&& someData) {
		mySubModelName = someData.mySubModelName;
		myVertices = std::move(someData.myVertices);
		myIndices = std::move(someData.myIndices);
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = std::move(someData.myVertexBuffer);
		myIndexBuffer = std::move(someData.myIndexBuffer);
		myMaterial = someData.myMaterial;
		myID = someData.myID;
		myBoundingVolume = someData.myBoundingVolume;
		myRenderIndex = someData.myRenderIndex;
		myGUID = someData.myGUID;
		someData.myVertices = nullptr;
		someData.myIndices = nullptr;
		someData.myVertexBuffer = nullptr;
		someData.myIndexBuffer = nullptr;
	}
	ModelData& operator=(const ModelData& someData) {
		mySubModelName = someData.mySubModelName;
		myVertices = (someData.myVertices);
		myIndices = (someData.myIndices);
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = someData.myVertexBuffer;
		myIndexBuffer = someData.myIndexBuffer;
		myMaterial = someData.myMaterial;
		myID = someData.myID;
		myBoundingVolume = someData.myBoundingVolume;
		myRenderIndex = someData.myRenderIndex;
		myGUID = someData.myGUID;
		return (*this);
	}
	ModelData& operator=(ModelData&& someData){
		mySubModelName = someData.mySubModelName;
		myVertices = std::move(someData.myVertices);
		myIndices = std::move(someData.myIndices);
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = std::move(someData.myVertexBuffer);
		myIndexBuffer = std::move(someData.myIndexBuffer);
		myMaterial = std::move(someData.myMaterial);
		myID = someData.myID;
		myBoundingVolume = someData.myBoundingVolume;
		myRenderIndex = someData.myRenderIndex;
		myGUID = someData.myGUID;

		someData.myVertices = nullptr;
		someData.myIndices = nullptr;
		someData.myVertexBuffer = nullptr;
		someData.myIndexBuffer = nullptr;
		someData.myMaterial = nullptr;
		return (*this);
	}
	~ModelData()
	{
		assert(myVertexBuffer == nullptr && "Must release resources before deleting object");
		assert(myIndexBuffer == nullptr && "Must release resources before deleting object");
		myMaterial = nullptr;
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
		if (myVertices)
		{
			delete[] myVertices;
			myVertices = nullptr;
		}
		if (myIndices)
		{
			delete[] myIndices;
			myIndices = nullptr;
		}
	}
	bool operator> (const ModelData& someData)
	{
		if (myID < someData.myID)
		{
			return true;
		}
		return false;
	}
	static unsigned int myModelIDCounter;
	inline const unsigned int GetID() const { return myID; }
	inline const unsigned short GetRenderIndex() const { return myRenderIndex; }

	GUID myGUID;
	CU::AABB3Df myBoundingVolume;
	ShortString mySubModelName;
	Vertex_PBR* myVertices;
	unsigned int* myIndices;
	unsigned int myOffset;
	unsigned int myNumberOfVertices;
	unsigned int myNumberOfIndices;
	unsigned int myStride;
	unsigned short myNumberOfSubmeshes;
	ID3D11Buffer* myVertexBuffer;
	ID3D11Buffer* myIndexBuffer;
	Material* myMaterial;
private:
	friend class Engine::ModelManager;
	unsigned int myID;
	unsigned short myRenderIndex;
};
struct AnimationFrame
{
	CU::GrowingArray<Bone> myBonedata;
};

struct Animation
{
	ShortString animationName;
	float duration = 0;
	unsigned int ticksPerSec = 0;
	CU::GrowingArray<AnimationFrame, unsigned int> frames;
};

struct AnimationData
{
	std::map<ShortString, unsigned int> myBoneNameToIndex;
	CU::GrowingArray<Bone> myIndexedSkeleton;
	CU::GrowingArray<Animation, unsigned int> myAnimations;
	std::map<ShortString, unsigned short> myAnimationNameToIndex;
};

struct AnimatedModelData
{
	AnimatedModelData(unsigned int aNumbVertices = 1, unsigned int aNumberOfIndices = 1, unsigned short aAmountOfSubMeshes = 1) :
		myNumberOfVertices(aNumbVertices),
		myNumberOfIndices(aNumberOfIndices),
		myNumberOfSubmeshes(aAmountOfSubMeshes)
	{
		myID = myAnimModelIDCounter++;
		myVertices = new Vertex_PBR_Animated[aNumbVertices];
		myIndices = new unsigned int[aNumberOfIndices];
		myOffset = 0;
		myStride = sizeof(Vertex_PBR_Animated);
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
		myMaterial = nullptr;
		myID = 0;
		myRenderIndex = 0;
		myGUID = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	}
	AnimatedModelData(const AnimatedModelData& someData) {
		mySubModelName		= someData.mySubModelName;
		myVertices			= someData.myVertices;
		myIndices			= someData.myIndices;
		myOffset			= someData.myOffset;
		myNumberOfVertices	= someData.myNumberOfVertices;
		myNumberOfIndices	= someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride			= someData.myStride;
		myVertexBuffer		= someData.myVertexBuffer;
		myIndexBuffer		= someData.myIndexBuffer;
		myMaterial			= someData.myMaterial;
		myID				= someData.myID;
		myBoundingVolume	= someData.myBoundingVolume;
		myRenderIndex		= someData.myRenderIndex;
		myGUID				= someData.myGUID;
	}
	AnimatedModelData(AnimatedModelData&& someData) {
		mySubModelName = someData.mySubModelName;
		myVertices = someData.myVertices;
		myIndices = someData.myIndices;
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = someData.myVertexBuffer;
		myIndexBuffer = someData.myIndexBuffer;
		myMaterial = someData.myMaterial;
		myID = someData.myID;
		myBoundingVolume = someData.myBoundingVolume;
		myRenderIndex = someData.myRenderIndex;
		myGUID = someData.myGUID;
		someData.myVertices = nullptr;
		someData.myIndices = nullptr;
		someData.myVertexBuffer = nullptr;
		someData.myIndexBuffer = nullptr;
		someData.myMaterial = nullptr;
	}
	AnimatedModelData& operator=(const AnimatedModelData& someData) {
		mySubModelName = someData.mySubModelName;
		myVertices = (someData.myVertices);
		myIndices = (someData.myIndices);
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = someData.myVertexBuffer;
		myIndexBuffer = someData.myIndexBuffer;
		myMaterial = someData.myMaterial;
		myID = someData.myID;
		myBoundingVolume = someData.myBoundingVolume;
		myRenderIndex = someData.myRenderIndex;
		myGUID = someData.myGUID;
		return (*this);
	}
	AnimatedModelData& operator=(AnimatedModelData&& someData) {
		mySubModelName = someData.mySubModelName;
		myVertices = (someData.myVertices);
		myIndices = (someData.myIndices);
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = std::move(someData.myVertexBuffer);
		myIndexBuffer = std::move(someData.myIndexBuffer);
		myMaterial = std::move(someData.myMaterial);
		myID = someData.myID;
		myBoundingVolume = someData.myBoundingVolume;
		myRenderIndex = someData.myRenderIndex;
		myGUID = someData.myGUID;

		someData.myVertices = nullptr;
		someData.myIndices = nullptr;
		someData.myVertexBuffer = nullptr;
		someData.myIndexBuffer = nullptr;
		someData.myMaterial = nullptr;
		return (*this);
	}
	~AnimatedModelData()
	{
		delete[] myVertices;
		delete[] myIndices;
		myVertices = nullptr;
		myIndices = nullptr;
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
		myMaterial = nullptr;
	}
	bool operator> (const AnimatedModelData& someData)
	{
		if (myID > someData.myID )
		{
			return true;
		}
		return false;
	}
	GUID myGUID;
	static unsigned int myAnimModelIDCounter;
	inline const unsigned int GetID() const { return myID; }
	inline const unsigned short GetRenderIndex() const { return myRenderIndex; }
	CU::AABB3Df myBoundingVolume;
	ShortString mySubModelName;
	Vertex_PBR_Animated* myVertices;
	unsigned int* myIndices;
	unsigned int myOffset;
	unsigned int myNumberOfVertices;
	unsigned int myNumberOfIndices;
	unsigned int myStride;
	unsigned short myNumberOfSubmeshes;
	ID3D11Buffer* myVertexBuffer;
	ID3D11Buffer* myIndexBuffer;
	Material* myMaterial;
private:
	friend class Engine::ModelFromFile_Importer;
	friend class Engine::ModelManager;
	unsigned int myID;
	unsigned short myRenderIndex;
};
struct ModelData_Particle
{
	ModelData_Particle(unsigned int aNumbVertices = 1, unsigned int aNumberOfIndices = 1, unsigned short aAmountOfSubMeshes = 1) :
		myNumberOfVertices(aNumbVertices),
		myNumberOfIndices(aNumberOfIndices),
		myNumberOfSubmeshes(aAmountOfSubMeshes)
	{
		myVertices = new Vertex_Particle[aNumbVertices];
		myIndices = new unsigned int[aNumberOfIndices];
		myIndexBuffer;
		myOffset = 0;
		myStride = sizeof(Vertex_Particle);
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
	}
	ModelData_Particle(const ModelData_Particle& someData) {
		myVertices = someData.myVertices;
		myIndices = someData.myIndices;
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = someData.myVertexBuffer;
		myIndexBuffer = someData.myIndexBuffer;
	}

	Vertex_Particle* myVertices;
	unsigned int* myIndices;
	unsigned int myOffset;
	unsigned int myNumberOfVertices;
	unsigned int myNumberOfIndices;
	unsigned int myStride;
	unsigned short myNumberOfSubmeshes;
	ID3D11Buffer* myVertexBuffer;
	ID3D11Buffer* myIndexBuffer;
	Material* myMaterial;
};

struct ModelData_Debug
{
	ModelData_Debug(unsigned int aNumbVertices = 1, unsigned int aNumberOfIndices = 1, unsigned short aAmountOfSubMeshes = 1) :
		myNumberOfVertices(aNumbVertices),
		myNumberOfIndices(aNumberOfIndices),
		myNumberOfSubmeshes(aAmountOfSubMeshes)
	{
		myVertices = new Vertex_Debug_Mesh[aNumbVertices];
		myIndices = new unsigned int[aNumberOfIndices];
		myIndexBuffer;
		myOffset = 0;
		myStride = sizeof(Vertex_PBR);
		myVertexBuffer = nullptr;
		myIndexBuffer = nullptr;
	}
	ModelData_Debug(const ModelData_Debug& someData) {
		myVertices = someData.myVertices;
		myIndices = someData.myIndices;
		myOffset = someData.myOffset;
		myNumberOfVertices = someData.myNumberOfVertices;
		myNumberOfIndices = someData.myNumberOfIndices;
		myNumberOfSubmeshes = someData.myNumberOfSubmeshes;
		myStride = someData.myStride;
		myVertexBuffer = someData.myVertexBuffer;
		myIndexBuffer = someData.myIndexBuffer;
	}

	Vertex_Debug_Mesh* myVertices;
	unsigned int* myIndices;
	unsigned int myOffset;
	unsigned int myNumberOfVertices;
	unsigned int myNumberOfIndices;
	unsigned int myStride;
	unsigned short myNumberOfSubmeshes;
	ID3D11Buffer* myVertexBuffer;
	ID3D11Buffer* myIndexBuffer;
	Material myMaterial;
};


