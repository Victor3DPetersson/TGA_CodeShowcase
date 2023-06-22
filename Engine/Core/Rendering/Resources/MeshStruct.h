#pragma once
#include "..\Engine\ECS\Systems\RenderCommands.h"

struct MeshesToRender
{
	SortedModelDataForRendering myNormalMeshes[MAX_SORTED_MESHES];
	unsigned short normalMeshListCount = 0;
	SortedModelDataForRendering myStaticMeshes[MAX_SORTED_MESHES];
	unsigned short staticMeshCount = 0;
	SortedModelDataForRendering myForwardMeshes[MAX_SORTED_MESHES];
	unsigned short forwardMeshListCount = 0;
	SortedModelDataForRendering myTransparentCutoutMeshes[MAX_SORTED_MESHES];
	unsigned short transparentCutoutCount = 0;
	SortedModelDataForRendering myOutlineMeshes[MAX_SORTED_MESHES];
	unsigned short outlineMeshListCount = 0;

	SortedAnimationDataForBuffers myNormalAnimMeshes[MAX_ANIM_SORTED_MESHES];
	unsigned short animNormalCount = 0;
	SortedAnimationDataForBuffers myFWD_AnimMeshes[MAX_ANIM_SORTED_MESHES];
	unsigned short animFwdListCount = 0;
	SortedAnimationDataForBuffers animTCutOutMeshes[MAX_ANIM_SORTED_MESHES];
	unsigned short animTCutoutCount = 0;
	SortedAnimationDataForBuffers myOutlinedAnimMeshes[MAX_ANIM_SORTED_MESHES];
	unsigned short animOutlineCount = 0;

	CU::GrowingArray<MeshRenderCommand> myUniqueNormalMeshes;
	CU::GrowingArray<MeshRenderCommand> myUniqueStaticMeshes;
	CU::GrowingArray<MeshRenderCommand> myUniqueForwardMeshes;
	CU::GrowingArray<MeshRenderCommand> myUniqueTCutoutMeshes;
	CU::GrowingArray<MeshRenderCommand> myUniqueOutlineMeshes;
	CU::GrowingArray<MeshRenderCommand> myRenderTargetMeshes;

	CU::GrowingArray<AnimatedMeshRenderCommand> myUniqueAnimatedNormalMeshes;
	CU::GrowingArray<AnimatedMeshRenderCommand> myUniqueFWD_AnimMeshes;
	CU::GrowingArray<AnimatedMeshRenderCommand> myUniqueTCutout_AnimMeshes;
	CU::GrowingArray<AnimatedMeshRenderCommand> myUniqueOutlinedAnimMeshes;

	ParticleBuffer particlesGlowing;
	ParticleBuffer particlesStandard;
	unsigned short particleMeshCount = 0;
	ParticleMeshRenderCommand particlesMesh[32];
};

