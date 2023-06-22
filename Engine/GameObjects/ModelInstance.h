#pragma once
#include "../../CommonUtilities/CU/Math/Matrix4x4.hpp"
#include "../../CommonUtilities/CU/Utility/ShortString.h"
namespace Engine
{
	class ModelManager;
}
class Model;
class ModelInstance
{
public:
	ModelInstance();
	~ModelInstance();
	ModelInstance(const ModelInstance& aModelCopy)
	{
		myTransform = aModelCopy.myTransform;
		myModelPtr = aModelCopy.myModelPtr;
		myModelType = aModelCopy.myModelType;
		myID = aModelCopy.myID;
	}
	inline ModelInstance& operator=(const ModelInstance& aModel) {
		myTransform = aModel.myTransform;
		myModelPtr = aModel.myModelPtr;
		myModelType = aModel.myModelType;
		myID = aModel.myID;
		return (*this);
	}
	bool operator==(const ModelInstance& aModel)
	{
		if (aModel.myID == myID)
		{
			return true;
		}
		return false;
	}


	void SetTransform(const CU::Vector3f aPosition, const CU::Vector3f aRotation);
	void SetPosition(const CU::Vector3f aPosition);
	void SetRotation(const CU::Vector3f aRotation);
	void Move(const CU::Vector3f aPosition);
	void Rotate(const CU::Vector3f aRotation);
	void RenderLights();
	//Getters
public:
	const ShortString& GetModelType() { return myModelType; }
	inline const unsigned int GetID() { return myID; }
	inline Model* GetModel() const { return myModelPtr; }
	inline CU::Matrix4x4f& GetMatrix() { return myTransform; }

private:

	void Init(Model* aModelPtr, const ShortString& aModelType);
	void DeInit();
	friend class Engine::ModelManager;
	static unsigned int myIDCount;
	unsigned int myID;
	CU::Matrix4x4f myTransform;
	ShortString myModelType;
	Model* myModelPtr;
	bool myShouldRender;
};

