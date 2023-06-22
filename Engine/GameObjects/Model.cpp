#include "stdafx.h"
#include "GameObjects/Model.h"
#include "EngineInterface.h"

void Model::SetCollider(CU::AABB3Df aCollider)
{
	myAABB = aCollider;
}

void Model::ReleaseResources()
{
	for (unsigned short i = 0; i < myAmountOfSubModels; i++)
	{
		SAFE_RELEASE(myModelData[i].myVertexBuffer);
		SAFE_RELEASE(myModelData[i].myIndexBuffer);
	}
	//if (myIsInited)
	//{
	//	delete[] myModelData;
	//	myModelData = nullptr;
	//}
	myIsInited = false;
}

void Model::Init(const ModelData& someModelData)
{
	if (myIsInited)
	{
		delete[] myModelData;
		myModelData = new ModelData[someModelData.myNumberOfSubmeshes];
	}
	else
	{
		myIsInited = true;
		myModelData = new ModelData[someModelData.myNumberOfSubmeshes];
	}
}

void Model::AddModelData(ModelData& someModelData)
{
	if (myIsInited)
	{
		myModelData[myAmountOfSubModels++] = std::move(someModelData);
	}
	else
	{
		assert(false && "Init model before adding to it");
	}
}

void ModelAnimated::ReleaseResources()
{
	for (unsigned short i = 0; i < myModelData.Size(); i++)
	{
		if (myModelData[i].myVertexBuffer)
		{
			myModelData[i].myVertexBuffer->Release();
		}
		if (myModelData[i].myIndexBuffer)
		{
			myModelData[i].myIndexBuffer->Release();
		}
		myModelData[i].myVertexBuffer = nullptr;
		myModelData[i].myIndexBuffer = nullptr;
	}
}

Animation* ModelAnimated::GetAnimation(const ShortString aAnimation)
{
	if (myAnimationData.myAnimationNameToIndex.find(aAnimation.GetString()) == myAnimationData.myAnimationNameToIndex.end())
	{
		return nullptr;
	}
	return &myAnimationData.myAnimations[myAnimationData.myAnimationNameToIndex[aAnimation.GetString()]];;
}

const unsigned short ModelAnimated::GetAnimationIndex(const ShortString aAnimation)
{
	if (myAnimationData.myAnimationNameToIndex.find(aAnimation.GetString()) == myAnimationData.myAnimationNameToIndex.end())
	{
		return 0;
	}
	return myAnimationData.myAnimationNameToIndex[aAnimation.GetString()];
}

void ModelAnimated::DrawSkeleton(m4f* someBonesFinalTransforms)
{
	DrawSkeletonRecursive(m4f::Identity, myAnimationData.myIndexedSkeleton, 0, someBonesFinalTransforms);
}


void ModelAnimated::SetCollider(CU::AABB3Df aCollider)
{
	myAABB = aCollider;
}

void ModelAnimated::DrawSkeletonRecursive(m4f aParentTransform, CU::GrowingArray<Bone>& aSkeleton, unsigned short aCurrentIndex, m4f* someBonesFinalTransforms)
{
	m4f NodeTransformation = m4f::Identity;

	//if (myAnimationData.myBoneNameToIndex.find(aSkeleton[aCurrentIndex].name.GetString()) != myAnimationData.myBoneNameToIndex.end()) {
	//	unsigned short BoneIndex = (unsigned short)myAnimationData.myBoneNameToIndex[aSkeleton[aCurrentIndex].name.GetString()];
	//	NodeTransformation = someBonesFinalTransforms[BoneIndex];
	//}
	NodeTransformation = myAnimationData.myIndexedSkeleton[aCurrentIndex];
	EngineInterface::DrawLine(m4f::Transpose(aParentTransform).GetTranslationVector(), m4f::Transpose((aParentTransform * NodeTransformation)).GetTranslationVector());

	//if (m4f::Transpose(NodeTransformation).GetTranslationVector() != m4f::Identity.GetTranslationVector())
	//{
	//}
	aParentTransform *= NodeTransformation;
	/*
	for (unsigned short i = 0; i < aSkeleton[aCurrentIndex].children.Size(); i++)
	{
		DrawSkeletonRecursive(aParentTransform, aSkeleton, (unsigned short)aSkeleton[aCurrentIndex].children[i], someBonesFinalTransforms);
	}
	*/
}

void ModelAnimated::Init(const AnimatedModelData someModelData)
{
	myModelData.ReInit(someModelData.myNumberOfSubmeshes);
}

void ModelAnimated::AddModelData(const AnimatedModelData someModelData)
{
	myModelData.Add(someModelData);
}




void Model_Debug::Init(const ModelData_Debug someModelData)
{
	myModelData.ReInit(someModelData.myNumberOfSubmeshes);
}

void Model_Debug::AddModelData(const ModelData_Debug someModelData)
{
	myModelData.Add(someModelData);
}
