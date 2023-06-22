#include "stdafx.h"
#include "GameObjects/ModelInstance.h"
#include "GameObjects/Model.h"

unsigned int ModelInstance::myIDCount = UINT_MAX;
ModelInstance::ModelInstance()
{
	myModelPtr = nullptr;
	myShouldRender = false;
	myIDCount++;
	myID = myIDCount;
}

ModelInstance::~ModelInstance()
{
	myModelPtr = nullptr;
}

void ModelInstance::Init(Model* aModelPtr, const ShortString& aModelType)
{
	myShouldRender = true;
	myModelType = aModelType;
	myModelPtr = aModelPtr;
}

void ModelInstance::DeInit()
{
	myModelPtr = nullptr;
	myTransform = CU::Matrix4x4f();
	myModelType = "";
	myShouldRender = false;
}

void ModelInstance::SetTransform(const CU::Vector3f aPosition, const CU::Vector3f )
{
	myTransform.SetTranslation(aPosition);
	//myTransform.SetRotation(aRotation);
}

void ModelInstance::SetPosition(const CU::Vector3f aPosition)
{
	myTransform.SetTranslation(aPosition);
}

void ModelInstance::SetRotation(const CU::Vector3f aRotation)
{
	myTransform.SetRotation(CU::Matrix4x4f::Identity);
	myTransform.SetRotation(myTransform * myTransform.CreateRotationAroundX(aRotation.x));
	myTransform.SetRotation(myTransform * myTransform.CreateRotationAroundY(aRotation.y));
	myTransform.SetRotation(myTransform * myTransform.CreateRotationAroundZ(aRotation.z));
}

void ModelInstance::Move(const CU::Vector3f aPosition)
{
	myTransform.SetTranslation(myTransform.GetTranslationVector() + aPosition);
}

void ModelInstance::Rotate(const CU::Vector3f aRotation)
{
	myTransform.SetRotation(myTransform * myTransform.CreateRotationAroundX(aRotation.x));
	myTransform.SetRotation(myTransform * myTransform.CreateRotationAroundY(aRotation.y));
	myTransform.SetRotation(myTransform * myTransform.CreateRotationAroundZ(aRotation.z));
}

