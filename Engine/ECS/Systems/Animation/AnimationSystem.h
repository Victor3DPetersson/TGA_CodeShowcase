#pragma once
#include "../CommonUtilities/CU/Utility/Timer/Timer.h"
#include "../CommonUtilities/CU/Utility/ShortString.h"
struct BonesToAnimate
{
	float myCurrentTime = 0;
	m4f Bones[MAX_BONECOUNT];
};
struct AnimatedMultiMeshComponent;
struct AnimatedMeshComponent;
class ModelAnimated;

namespace AnimationSystem
{
	void UpdateMultiAnimation(float aDeltaTime, AnimatedMultiMeshComponent& aModelToAnimate, m4f* aFinalTransforms, unsigned char aAccessory);
	void UpdateAnimation(float aDeltaTime, AnimatedMeshComponent& aModelToAnimate, m4f* aFinalTransforms);
	void UpdateAnimationViewer(float aDeltaTime, AnimatedMeshComponent& aModelToAnimate, m4f* aFinalTransforms, ModelAnimated* aModel);

	void InternalUpdate(float aDeltaTime, AnimatedMeshComponent& aModelToAnimate, m4f* aFinalTransforms, ModelAnimated* aModel);
}