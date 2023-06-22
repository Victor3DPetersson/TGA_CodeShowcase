#include "stdafx.h"
#include "RenderFunctions.h"
#include "../Engine/RenderData.h"
#include "MeshStruct.h"
#include "GameObjects\Model.h"

namespace Engine
{
	void ClearMeshes(MeshesToRender& aMeshList, size_t aNumOfSortedMeshes, size_t aNumOfSortedAnimMeshes)
	{
		for (unsigned short i = 0; i < aNumOfSortedMeshes; i++)
		{
			aMeshList.myNormalMeshes[i].numberOfModels = 0;
			aMeshList.myStaticMeshes[i].numberOfModels = 0;
			aMeshList.myForwardMeshes[i].numberOfModels = 0;
			aMeshList.myTransparentCutoutMeshes[i].numberOfModels = 0;
			aMeshList.myOutlineMeshes[i].numberOfModels = 0;
		}
		aMeshList.normalMeshListCount = 0;
		aMeshList.staticMeshCount = 0;
		aMeshList.forwardMeshListCount = 0;
		aMeshList.transparentCutoutCount = 0;
		aMeshList.outlineMeshListCount = 0;

		for (unsigned short i = 0; i < aNumOfSortedAnimMeshes; i++)
		{
			aMeshList.myNormalAnimMeshes[i].numberOfModels = 0;
			aMeshList.myFWD_AnimMeshes[i].numberOfModels = 0;
			aMeshList.animTCutOutMeshes[i].numberOfModels = 0;
			aMeshList.myOutlinedAnimMeshes[i].numberOfModels = 0;
		}
		aMeshList.animNormalCount = 0;
		aMeshList.animFwdListCount = 0;
		aMeshList.animTCutoutCount = 0;
		aMeshList.animOutlineCount = 0;

		aMeshList.myUniqueNormalMeshes.RemoveAll();
		aMeshList.myUniqueAnimatedNormalMeshes.RemoveAll();
		aMeshList.myUniqueStaticMeshes.RemoveAll();
		aMeshList.myUniqueForwardMeshes.RemoveAll();
		aMeshList.myUniqueTCutoutMeshes.RemoveAll();
		aMeshList.myUniqueFWD_AnimMeshes.RemoveAll();
		aMeshList.myUniqueTCutout_AnimMeshes.RemoveAll();
		aMeshList.myUniqueOutlineMeshes.RemoveAll();
		aMeshList.myUniqueOutlinedAnimMeshes.RemoveAll();
		aMeshList.myRenderTargetMeshes.RemoveAll();

		aMeshList.particlesGlowing.myNumberOfSystems = 0;
		aMeshList.particlesStandard.myNumberOfSystems = 0;

	}
}

namespace RenderDebugFunctions
{
	void DrawLine(Engine::RenderData* aBuffer, const v3f aFrom, const v3f aTo, CU::Color aColor, float aSize)
	{
#ifndef _DISTRIBUTION
		Vertex_Debug_Line command;
		command.myPosFrom.x = aFrom.x;
		command.myPosFrom.y = aFrom.y;
		command.myPosFrom.z = aFrom.z;
		command.myPosFrom.w = 1;

		command.myPosTo.x = aTo.x;
		command.myPosTo.y = aTo.y;
		command.myPosTo.z = aTo.z;
		command.myPosTo.w = 1;
		command.myColor = aColor;
		command.mySize = aSize;
		aBuffer->debugLines[aBuffer->debugLinesSize++] = command;
#endif
	}
	void DrawBox(Engine::RenderData* aBuffer, CU::AABB3Df aCollider, float aLineSize, CU::Color aColor)
	{
#ifndef _DISTRIBUTION
		//   Y TopBot | Z FrontBack | X LeftRight
		v3f tfr, tfl, tbr, tbl, bfr, bfl, bbl, bbr;
		tfr = aCollider.myMax;
		bbl = aCollider.myMin;
		tfl = tfr; tfl.x = bbl.x;
		tbr = tfr; tbr.z = bbl.z;
		tbl = bbl; tbl.y = tfr.y;

		bfr = tfr; bfr.y = bbl.y;
		bfl = bbl; bfl.z = tfr.z;
		bbr = bbl; bbr.x = tfr.x;

		DrawLine(aBuffer, tfr, tfl, aColor, aLineSize);
		DrawLine(aBuffer, tfr, bfr, aColor, aLineSize);
		DrawLine(aBuffer, tfr, tbr, aColor, aLineSize);
		DrawLine(aBuffer, tbr, bbr, aColor, aLineSize);
		DrawLine(aBuffer, tbr, tbl, aColor, aLineSize);
		DrawLine(aBuffer, bbr, bfr, aColor, aLineSize);
		DrawLine(aBuffer, bbr, bbl, aColor, aLineSize);
		DrawLine(aBuffer, bbl, tbl, aColor, aLineSize);
		DrawLine(aBuffer, bbl, bfl, aColor, aLineSize);
		DrawLine(aBuffer, bfl, bfr, aColor, aLineSize);
		DrawLine(aBuffer, bfl, tfl, aColor, aLineSize);
		DrawLine(aBuffer, tfl, tbl, aColor, aLineSize);
#endif
	}

	void DrawBoxAxisColored(Engine::RenderData* aBuffer, const v3f aMin, const v3f aMax)
	{
#ifndef _DISTRIBUTION
		const v3f size = (aMax - aMin);
		v3f boundPoints[8];
		boundPoints[0] = aMax; //tFR
		boundPoints[1] = { aMax.x - size.x, aMax.y, aMax.z }; //tFL
		boundPoints[2] = { aMax.x, aMax.y, aMax.z - size.z }; //tBR
		boundPoints[3] = { aMax.x - size.x, aMax.y, aMax.z - size.z }; // tBL
		boundPoints[4] = aMin; //bBL
		boundPoints[5] = { aMin.x + size.x, aMin.y, aMin.z };//bBR
		boundPoints[6] = { aMin.x, aMin.y, aMin.z + size.z };//bFL
		boundPoints[7] = { aMin.x + size.x, aMin.y, aMin.z + size.z };//bFR
		CU::Color red = { 255, 0, 0, 255 };
		CU::Color green = { 0, 255, 0, 255 };
		CU::Color blue = { 0, 0, 255, 255 };
		float lineSize = 2.0f;
		DrawLine(aBuffer, boundPoints[0], boundPoints[1], blue, lineSize);
		DrawLine(aBuffer, boundPoints[0], boundPoints[2], red, lineSize);
		DrawLine(aBuffer, boundPoints[0], boundPoints[7], green, lineSize);
		DrawLine(aBuffer, boundPoints[7], boundPoints[6], blue, lineSize);
		DrawLine(aBuffer, boundPoints[7], boundPoints[5], red, lineSize);
		DrawLine(aBuffer, boundPoints[5], boundPoints[4], blue, lineSize);
		DrawLine(aBuffer, boundPoints[5], boundPoints[2], green, lineSize);
		DrawLine(aBuffer, boundPoints[2], boundPoints[3], blue, lineSize);
		DrawLine(aBuffer, boundPoints[3], boundPoints[1], red, lineSize);
		DrawLine(aBuffer, boundPoints[3], boundPoints[4], green, lineSize);
		DrawLine(aBuffer, boundPoints[4], boundPoints[6], red, lineSize);
		DrawLine(aBuffer, boundPoints[6], boundPoints[1], green, lineSize);
#endif
	}
	void DrawSphere(Engine::RenderData* aBuffer, const v3f aPos, const float aRadius, CU::Color aColor)
	{
#ifndef _DISTRIBUTION
		Engine::DebugSphereCommand command;
		command.myPosition = aPos;
		command.myRadius = aRadius * 0.01f;
		command.mySphereColor = aColor;
		aBuffer->debugSpheres[aBuffer->debugSpheresSize++] = command;
#endif
	}
	void DrawSphereSquared(Engine::RenderData* aBuffer, const v4f aSphere, CU::Color aColor)
	{
#ifndef _DISTRIBUTION
		Engine::DebugSphereCommand command;
		command.myPosition = v3f(aSphere.x, aSphere.y, aSphere.z);
		command.myRadius = sqrtf(aSphere.w) * 0.01f;
		command.mySphereColor = aColor;
		aBuffer->debugSpheres[aBuffer->debugSpheresSize++] = command;
#endif 
	}
	void DrawSpot(Engine::RenderData* aBuffer, const v3f aPos, const v3f aDirection, float aAngle, float aRange, float aLineSize, CU::Color aColor)
	{
#ifndef _DISTRIBUTION
		m4f lookAt;
		lookAt.LookAt(aDirection, { 0, 0, 0 });
		const m3f rotationMatrix = lookAt;

		const float radius = aRange * tanf(aAngle * 0.5f);
		unsigned int amountOfLines = 8;
		float theta = 2.0f * CU::pif * (1.0f / (float)amountOfLines);
		for (unsigned int i = 0; i < amountOfLines; i++)
		{
			float x = radius * cosf(i * theta);
			float y = radius * sinf(i * theta);
			DrawLine(aBuffer, aPos, aPos + v3f(x, y, aRange) * rotationMatrix, aColor, aLineSize);
		}
#endif
	}
	void DrawSkeletonRecursiveEngine(Engine::RenderData* aBuffer, m4f aParentTransform, CU::GrowingArray<Bone>& aSkeleton, unsigned short aCurrentIndex, m4f* someBonesFinalTransforms, ModelAnimated* aModel)
	{
#ifndef _DISTRIBUTION
		m4f NodeTransformation = m4f::Identity;
		AnimationData& animData = aModel->myAnimationData;
	/*	if (animData.myBoneNameToIndex.find(aSkeleton[aCurrentIndex].name.GetString()) != animData.myBoneNameToIndex.end()) {
			unsigned short BoneIndex = (unsigned short)myAnimationData.myBoneNameToIndex[aSkeleton[aCurrentIndex].name.GetString()];
			NodeTransformation = someBonesFinalTransforms[BoneIndex];
		}*/
		NodeTransformation = animData.myIndexedSkeleton[aCurrentIndex++];
		DrawLine(aBuffer, m4f::Transpose(aParentTransform).GetTranslationVector(), m4f::Transpose((aParentTransform * NodeTransformation)).GetTranslationVector());

		if (aCurrentIndex >= aSkeleton.Size())
		{
			return;
		}
		aParentTransform *= NodeTransformation;
		
		DrawSkeletonRecursiveEngine(aBuffer, aParentTransform, aSkeleton, aCurrentIndex, someBonesFinalTransforms, aModel);
#endif
	}
}

