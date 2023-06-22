#include "stdafx.h"
#include "AnimationSystem.h"
#include "GameObjects\Model.h"
#include "ECS\Components.h"
#include "EngineInterface.h"
#include "Managers\ModelManager.h"

void AnimationSystem::UpdateMultiAnimation(float aDeltaTime, AnimatedMultiMeshComponent& aModelToAnimate, m4f* aFinalTransforms, unsigned char aAccessory)
{
	Engine::ModelManager* mm = EngineInterface::GetModelManager();
	ModelAnimated* model = mm->GetAnimatedModel(aModelToAnimate.model);
	if (model)
	{
		AnimatedMeshComponent aMC;
		aMC.activeAnimation1 = aModelToAnimate.activeAnimation1;
		aMC.activeAnimation2 = aModelToAnimate.activeAnimation2;
		aMC.blendFactor = aModelToAnimate.blendFactor;
		aMC.currentActiveTime1 = aModelToAnimate.currentActiveTime1;
		aMC.currentActiveTime2 = aModelToAnimate.currentActiveTime2;
		aMC.drawSkeleton = aModelToAnimate.drawSkeleton;
		aMC.effectColor = aModelToAnimate.effectColor;
		aMC.effectTValue = aModelToAnimate.effectTValue;
		aMC.isLooping = aModelToAnimate.isLooping;
		aMC.model = aAccessory ? aModelToAnimate.modelAccessory : aModelToAnimate.model;
		aMC.modelType = aModelToAnimate.modelType;
		aMC.outlineColor = aModelToAnimate.outlineColor;
		aMC.psEffectIndex = aModelToAnimate.psEffectIndex;
		aMC.renderUnique = aModelToAnimate.renderUnique;
		aMC.shouldAnimate = aModelToAnimate.shouldAnimate;

		if (aAccessory)
		{
			InternalUpdate(0.0f, aMC, aFinalTransforms, model);
		}
		else
		{
			InternalUpdate(aDeltaTime, aMC, aFinalTransforms, model);
		}

		aModelToAnimate.activeAnimation1 = aMC.activeAnimation1;
		aModelToAnimate.activeAnimation2 = aMC.activeAnimation2;
		aModelToAnimate.blendFactor = aMC.blendFactor;
		aModelToAnimate.currentActiveTime1 = aMC.currentActiveTime1;
		aModelToAnimate.currentActiveTime2 = aMC.currentActiveTime2;
		aModelToAnimate.drawSkeleton = aMC.drawSkeleton;
		aModelToAnimate.effectColor = aMC.effectColor;
		aModelToAnimate.effectTValue = aMC.effectTValue;
		aModelToAnimate.isLooping = aMC.isLooping;
		aModelToAnimate.modelType = aMC.modelType;
		aModelToAnimate.outlineColor = aMC.outlineColor;
		aModelToAnimate.psEffectIndex = aMC.psEffectIndex;
		aModelToAnimate.renderUnique = aMC.renderUnique;
		aModelToAnimate.shouldAnimate = aMC.shouldAnimate;
	}
}

void AnimationSystem::UpdateAnimation(float aDeltaTime, AnimatedMeshComponent& aModelToAnimate, m4f* aFinalTransforms)
{
	Engine::ModelManager* mm = EngineInterface::GetModelManager();
	ModelAnimated* model = mm->GetAnimatedModel(aModelToAnimate.model);
	if (model)
	{
		InternalUpdate(aDeltaTime, aModelToAnimate, aFinalTransforms, model);
	}
}

void AnimationSystem::UpdateAnimationViewer(float aDeltaTime, AnimatedMeshComponent& aModelToAnimate, m4f* aFinalTransforms, ModelAnimated* aModel)
{
	InternalUpdate(aDeltaTime, aModelToAnimate, aFinalTransforms, aModel);
}

void AnimationSystem::InternalUpdate(float aDeltaTime, AnimatedMeshComponent& aModelToAnimate, m4f* aFinalTransforms, ModelAnimated* aModel)
{
	AnimationData& data = aModel->GetAnimationData();
	if (data.myAnimations.Size() > 0)
	{
		if (!aModelToAnimate.shouldAnimate)
		{
			Animation* animation0 = aModel->GetAnimation(aModelToAnimate.activeAnimation1);
			if (animation0)
			{
				unsigned int animation0FrameIndex0 = (int)(aModelToAnimate.currentActiveTime1 * animation0->ticksPerSec) % animation0->frames.Size();
				unsigned int animation0FrameIndex1 = (int)(aModelToAnimate.currentActiveTime1 * animation0->ticksPerSec + 1) % animation0->frames.Size();
				if (animation0FrameIndex0 > animation0FrameIndex1 && !aModelToAnimate.isLooping)
				{
					animation0FrameIndex1 = animation0FrameIndex0;
				}

				const float t0 = fmodf(aModelToAnimate.currentActiveTime1 * animation0->ticksPerSec, 1.0f);

				const AnimationFrame& animation0Frame0 = animation0->frames[animation0FrameIndex0];
				const AnimationFrame& animation0Frame1 = animation0->frames[animation0FrameIndex1];

				for (unsigned short i = 0; i < animation0Frame0.myBonedata.Size(); ++i)
				{
					m4f::Lerp(aFinalTransforms[i], animation0Frame0.myBonedata[i], animation0Frame1.myBonedata[i], t0);
				}
			}
		}
		else
		{
			Animation* animation0 = aModel->GetAnimation(aModelToAnimate.activeAnimation1);
			if (animation0)
			{
				aModelToAnimate.currentActiveTime1 += aDeltaTime;
				unsigned int animation0FrameIndex0 = (int)(aModelToAnimate.currentActiveTime1 * animation0->ticksPerSec) % animation0->frames.Size();
				unsigned int animation0FrameIndex1 = (int)(aModelToAnimate.currentActiveTime1 * animation0->ticksPerSec + 1) % animation0->frames.Size();

				if (animation0FrameIndex0 > animation0FrameIndex1 && !aModelToAnimate.isLooping)
				{
					animation0FrameIndex1 = animation0FrameIndex0;
				}

				const float t0 = fmodf(aModelToAnimate.currentActiveTime1 * animation0->ticksPerSec, 1.0f);

				const AnimationFrame& animation0Frame0 = animation0->frames[animation0FrameIndex0];
				const AnimationFrame& animation0Frame1 = animation0->frames[animation0FrameIndex1];

				for (unsigned short i = 0; i < animation0Frame0.myBonedata.Size(); ++i)
				{
					m4f::Lerp(aFinalTransforms[i], animation0Frame0.myBonedata[i], animation0Frame1.myBonedata[i], t0);
				}

				if (aModelToAnimate.blendFactor > 0.0f)
				{

					Animation* animation1 = aModel->GetAnimation(aModelToAnimate.activeAnimation2);
					if (animation1)
					{
						aModelToAnimate.currentActiveTime2 += aDeltaTime;
						unsigned int animation1FrameIndex0 = (int)(aModelToAnimate.currentActiveTime2 * animation1->ticksPerSec) % animation1->frames.Size();
						unsigned int animation1FrameIndex1 = (int)(aModelToAnimate.currentActiveTime2 * animation1->ticksPerSec + 1) % animation1->frames.Size();

						if (animation1FrameIndex0 > animation1FrameIndex1 && !aModelToAnimate.isLooping)
						{
							animation1FrameIndex1 = animation1FrameIndex0;
						}

						const float t1 = fmodf(aModelToAnimate.currentActiveTime2 * animation1->ticksPerSec, 1.0f);

						const AnimationFrame& animation1Frame0 = animation1->frames[animation1FrameIndex0];
						const AnimationFrame& animation1Frame1 = animation1->frames[animation1FrameIndex1];

						static m4f blendBuffer[MAX_BONECOUNT];

						for (unsigned short i = 0; i < animation1Frame0.myBonedata.Size(); ++i)
						{
							m4f::Lerp(blendBuffer[i], animation1Frame0.myBonedata[i], animation1Frame1.myBonedata[i], t1);
						}

						for (unsigned short i = 0; i < animation1Frame0.myBonedata.Size(); ++i)
						{
							m4f::Lerp(aFinalTransforms[i], aFinalTransforms[i], blendBuffer[i], aModelToAnimate.blendFactor);
						}
					}
				}
			}
		}
	}
}
