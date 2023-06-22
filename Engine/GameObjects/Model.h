#pragma once
#include "ModelData.h"
#include "../../CommonUtilities/CU/Containers\GrowingArray.hpp"
#include "../../CommonUtilities/CU/Math/Matrix4x4.hpp"
#include "../../CommonUtilities/CU/Collision/AABB3D.hpp"
#include "../../CommonUtilities/CU/Utility/FixedString.hpp"
namespace Engine
{
	class ModelManager;
	class DebugRenderer;
	class ModelFromFile_Importer;
}
namespace MV
{
	class ModelManager;
	class ModelFromFile_Importer;
}

class Model
{
public:
	Model() { myModelData = nullptr;
	myIsInited = false;
	myAmountOfSubModels = 0;
	myRefCounter = 0;
	}
	~Model() {
		if (myIsInited)
		{
			delete[] myModelData;
			myModelData = nullptr;
		}
		assert(myModelData == nullptr && "Did not return model memory properly");
	};
	Model(const Model& aModel) {
		myModelData = aModel.myModelData;
		myIsInited = aModel.myIsInited;
		myAmountOfSubModels = aModel.myAmountOfSubModels;
		myAABB = aModel.myAABB;
		myRefCounter = aModel.myRefCounter;
		myName = aModel.myName;
	}
	Model(Model&& aModel)	{
		myAABB = aModel.myAABB;
		myIsInited = aModel.myIsInited;
		myAmountOfSubModels = aModel.myAmountOfSubModels;
		myModelData = aModel.myModelData;
		myName = aModel.myName;
		aModel.myModelData = nullptr;
		aModel.myIsInited = false;
		aModel.myAmountOfSubModels = 0;
		aModel.myRefCounter = 0;
		myRefCounter = aModel.myRefCounter;
	}
	inline Model& operator=(const Model& aModel)	{
		myAABB = aModel.myAABB;
		myModelData = aModel.myModelData;
		myIsInited = aModel.myIsInited;
		myAmountOfSubModels = aModel.myAmountOfSubModels;
		myRefCounter = aModel.myRefCounter;
		myName = aModel.myName;
		return (*this);
	}
	inline Model& operator=(Model&& aModel) {
		myAABB = aModel.myAABB;
		myIsInited = aModel.myIsInited;
		myAmountOfSubModels = aModel.myAmountOfSubModels;
		myModelData = aModel.myModelData;
		myRefCounter = aModel.myRefCounter;
		myName = aModel.myName;
		aModel.myModelData = nullptr;
		aModel.myIsInited = false;
		aModel.myAmountOfSubModels = 0;
		aModel.myRefCounter = 0;
		return (*this);
	}
	inline ModelData& GetModelData(unsigned short index = 0) { assert(index < myAmountOfSubModels && "Index out of range"); return myModelData[index]; }
	inline unsigned short GetAmountOfSubModels() { return myAmountOfSubModels; };
	void SetCollider(CU::AABB3Df aCollider);
	const CU::AABB3Df GetCollider() const { return myAABB; }
	const int GetRefCounter() { return myRefCounter; }
//private:
	void ReleaseResources();
	void Init(const ModelData& someModelData);
	void AddModelData(ModelData& someModelData);
	friend class Engine::ModelManager;
	CU::AABB3Df myAABB;
	bool myIsInited = false;
	unsigned short myAmountOfSubModels;
	ModelData* myModelData;
	FixedString256 myName;
private:
	int myRefCounter;
};


class Model_Particle
{
public:
	Model_Particle() { myModelData.Init(1); }
	~Model_Particle() {
		if (myModelData.IsInitialized())
		{
			for (unsigned short i = 0; i < myModelData.Size(); i++)
			{
				myModelData[i].myMaterial = nullptr;
				myModelData[i].myVertexBuffer = nullptr;
				myModelData[i].myIndexBuffer = nullptr;
			}
		}
	};
	Model_Particle(const Model_Particle& aModel) {
		myModelData = aModel.myModelData;
		myAABB = aModel.myAABB;
	}
	inline Model_Particle& operator=(const Model_Particle& aModel) {
		myAABB = aModel.myAABB;
		myModelData = aModel.myModelData;
		return (*this);
	}
	inline Model_Particle& operator=(const Model_Particle&& aModel) {
		myAABB = aModel.myAABB;
		myModelData = std::move(aModel.myModelData);
		return (*this);
	}
	inline ModelData_Particle& GetModelData(unsigned short index = 0) { return myModelData[index]; }
	inline unsigned short GetAmountOfSubModels() { return myModelData[0].myNumberOfSubmeshes; };
	void SetCollider(CU::AABB3Df aCollider);
	const CU::AABB3Df GetCollider() const { return myAABB; }
	//private:
	void ReleaseResources();
	void Init(const Model_Particle someModelData);
	void AddModelData(const Model_Particle someModelData);
	friend class Engine::ModelManager;
	friend class MV::ModelManager;
	CU::AABB3Df myAABB;
	CU::GrowingArray<ModelData_Particle> myModelData;
};

class ModelAnimated
{
public:
	ModelAnimated() { myModelData.Init(1); }
	~ModelAnimated() {
		if (myModelData.IsInitialized())
		{
			for (unsigned short i = 0; i < myModelData.Size(); i++)
			{
				myModelData[i].myMaterial = nullptr;
				myModelData[i].myVertexBuffer = nullptr;
				myModelData[i].myIndexBuffer = nullptr;
			}
		}
	};
	ModelAnimated(const ModelAnimated& aModel) {
		myModelData = aModel.myModelData;
		myAnimationData = aModel.myAnimationData;
		myAABB = aModel.myAABB;
	}
	inline ModelAnimated& operator=(const ModelAnimated& aModel) {
		myModelData = aModel.myModelData;
		myAnimationData = aModel.myAnimationData;
		myAABB = aModel.myAABB;
		return (*this);
	}
	ModelAnimated(ModelAnimated&& aModel) {
		myModelData = std::move(aModel.myModelData);
		myAnimationData = aModel.myAnimationData;
		myAABB = aModel.myAABB;
	}
	inline ModelAnimated& operator=(ModelAnimated&& aModel) {
		myModelData = std::move(aModel.myModelData);
		myAnimationData = aModel.myAnimationData;
		myAABB = aModel.myAABB;
		return (*this);
	}
	inline AnimatedModelData& GetModelData(unsigned short index = 0) { return myModelData[index]; }
	inline unsigned short GetAmountOfSubModels() { return myModelData.Size(); };
	AnimationData& GetAnimationData() { return myAnimationData; }
	Animation* GetAnimation(const ShortString aAnimation);
	const unsigned short GetAnimationIndex(const ShortString aAnimation);
	void DrawSkeleton(m4f* someBonesFinalTransforms);
	void SetCollider(CU::AABB3Df aCollider);
	const CU::AABB3Df GetCollider() const { return myAABB; }

	void DrawSkeletonRecursive(m4f aParentTransform, CU::GrowingArray<Bone>& aSkeleton, unsigned short myCurrentIndex, m4f* someBonesFinalTransforms);
	void Init(const AnimatedModelData someModelData);
	void ReleaseResources();
	void AddModelData(const AnimatedModelData someModelData);
	friend class Engine::ModelManager;
	friend class MV::ModelManager;
	friend class MV::ModelFromFile_Importer;
	friend class Engine::ModelFromFile_Importer;
	CU::GrowingArray<AnimatedModelData> myModelData;
	CU::AABB3Df myAABB;
	AnimationData myAnimationData;
};

class Model_Debug
{
public:
	Model_Debug() { myModelData.Init(1); }
	~Model_Debug() = default;
	Model_Debug(const Model_Debug& aModel) {
		myModelData = aModel.myModelData;
	}
	inline Model_Debug& operator=(const Model_Debug& aModel) {
		myModelData = aModel.myModelData;
		return (*this);
	}
	inline ModelData_Debug& GetModelData(unsigned short index = 0) { return myModelData[index]; }
	inline unsigned short GetAmountOfSubModels() { return myModelData[0].myNumberOfSubmeshes; };

private:
	void Init(const ModelData_Debug someModelData);
	void AddModelData(const ModelData_Debug someModelData);
	friend class Engine::DebugRenderer;
	CU::GrowingArray<ModelData_Debug> myModelData;
};