#pragma once
#include "..\CommonUtilities\CU\Math\Vector.h"
#include "..\CommonUtilities\CU\Math\Matrix3x3.hpp"
#include "..\CommonUtilities\CU\Collision\AABB3D.hpp"
#include "..\CommonUtilities\CU\Math\Color.hpp"
#include "..\CommonUtilities\CU\Containers\GrowingArray.hpp"
#include "..\..\..\ECS\Systems\RenderCommands.h"
enum BlendState
{
	BLENDSTATE_DISABLE,
	BLENDSTATE_ALPHABLEND,
	BLENDSTATE_ALPHABLEND_NOCOVERAGE,
	BLENDSTATE_ADDITIVE,
	BLENDSTATE_ALPHABLEND_DECALS,
	BLENDSTATE_COUNT
};

enum DepthStencilState
{
	DEPTHSTENCILSTATE_DEFAULT,
	DEPTHSTENCILSTATE_READONLY,
	DEPTHSTENCILSTATE_READONLY_STENCIL_WRITE,
	DEPTHSTENCILSTATE_READONLY_STENCIL_WRITEDISCARDED,
	DEPTHSTENCILSTATE_READONLY_STENCIL_READ,
	DEPTHSTENCILSTATE_STENCILONLY_WRITE,
	DEPTHSTENCILSTATE_STENCILONLY_READ,
	DEPTHSTENCILSTATE_OFF,
	DEPTHSTENCILSTATE_COUNT
};

enum RasterizerState
{
	RASTERIZERSTATE_DEFAULT,
	RASTERIZERSTATE_COUNTERCLOCKWISE,
	RASTERIZERSTATE_WIREFRAME,
	RASTERIZERSTATE_DOUBLESIDED,
	RASTERIZERSTATE_COUNTERCLOCKWISE_NODEPTHPASS,
	RASTERIZERSTATE_COUNT
};

enum SamplerState
{
	SAMPLERSTATE_TRILINEAR,
	SAMPLERSTATE_TRILINEARWRAP,
	SAMPLERSTATE_POINT,
	SAMPLERSTATE_CLAMP_COMPARISON,
	SAMPLERSTATE_COUNT
};

enum ETextureUsageFlags
{
	TEXTUREUSAGEFLAG_DEFAULT,
	TEXTUREUSAGEFLAG_IMMUTABLE,
	TEXTUREUSAGEFLAG_DYNAMIC,
	TEXTUREUSAGEFLAG_STAGING
};

struct MeshesToRender;
namespace Engine
{
	struct RenderData;

	inline bool TestConeVsSphere(v3f aOrigin, v3f aSpotFwd, float aSpotLength, float aSpotAngle, v4f aTestSphere)
	{
		const v3f V = v3f{ aTestSphere.x, aTestSphere.y, aTestSphere.z } - aOrigin;
		const float  VlenSq = V.Dot(V);
		const float  V1len = V.Dot(aSpotFwd);
		const float  distanceClosestPoint = cosf(aSpotAngle * 0.5f) * sqrtf(VlenSq - V1len * V1len) - V1len * sinf(aSpotAngle * 0.5f);
		const bool angleCull = distanceClosestPoint > aTestSphere.w;
		const bool frontCull = V1len > aTestSphere.w + aSpotLength;
		const bool backCull = V1len < -aTestSphere.w;
		return !(angleCull || frontCull || backCull);
	}

	inline float sqDistPointAABB(v3f aPoint, CU::AABB3Df& aTile)
	{
		float sqDist = 0.0;
		for (int i = 0; i < 3; ++i) {
			const float v = aPoint[i];
			if (v < aTile.myMin[i])
			{
				sqDist += (aTile.myMin[i] - v) * (aTile.myMin[i] - v);
			}
			if (v > aTile.myMax[i])
			{
				sqDist += (v - aTile.myMax[i]) * (v - aTile.myMax[i]);
			}
		}
		return sqDist;
	}

	inline bool testSphereAABB(float aRadius, CU::AABB3Df& aTile, v3f aPos)
	{
		return sqDistPointAABB(aPos, aTile) <= ((aRadius) * (aRadius));
	}

	void ClearMeshes(MeshesToRender& aMeshList, size_t aNumOfSortedMeshes, size_t aNumOfSortedAnimMeshes);

}
class ModelAnimated;
namespace RenderDebugFunctions
{
	void DrawLine(Engine::RenderData* aBuffer, const v3f aFrom, const v3f aTo, CU::Color aColor = { 255, 255, 255, 255 }, float aSize = 1);

	void DrawBox(Engine::RenderData* aBuffer, CU::AABB3Df aCollider, float aLineSize = 1, CU::Color aColor = { 255, 255, 255, 255 });

	void DrawBoxAxisColored(Engine::RenderData* aBuffer, const v3f aMin, const v3f aMax);

	void  DrawSphere(Engine::RenderData* aBuffer, const v3f aPos, const float aRadius, CU::Color aColor = { 255, 255, 255, 255 });

	void  DrawSphereSquared(Engine::RenderData* aBuffer, const v4f aSphere, CU::Color aColor = { 255, 255, 255, 255 });

	void DrawSpot(Engine::RenderData* aBuffer, const v3f aPos, const v3f aDirection, float aAngle, float aRange, float aLineSize, CU::Color aColor = { 255, 255, 255, 255 });

	void DrawSkeletonRecursiveEngine(Engine::RenderData* aBuffer, m4f aParentTransform, CU::GrowingArray<Bone>& aSkeleton, unsigned short aCurrentIndex, m4f* someBonesFinalTransforms, ModelAnimated* aModel);

}

