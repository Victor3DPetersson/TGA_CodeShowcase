#pragma once
#include "../RenderConstants.hpp"
#include "../../CommonUtilities/CU/Math/Matrix4x4.hpp"
#include "../../CommonUtilities/CU/Math/Vector2.hpp"
#include "../../CommonUtilities/CU/Containers/VectorOnStack.h"
#include "../../CommonUtilities/CU/Collision/Plane.hpp"
#include "../../CommonUtilities/CU/Collision/AABB3D.hpp"
#include "../../CommonUtilities/CU/Collision/Sphere.hpp"
struct FrustumDrawData
{
	v3f fTL, fTR, fBL, fBR, nTL, nTR, nBL, nBR;
};

struct CameraData
{
	CU::Matrix4x4f toCTransform;
	m4f fromCamera;
	CU::Matrix4x4f CProjection;
	float myNear = 50;
	float myFar = 10000;
	float clusterPreNumerator = 0;
	float clusterPreDenominator = 0;
	CU::Matrix4x4f CInvProjection;
};

struct CameraShakeData
{
	v3f myOriginalPosition = v3f();
	v3f myPerlinOffset = v3f();
	float myShakeForce = 0.5f;
	float myShakeMultiplier = 16.0f;
	float myShakeMagnitude = 0.8f;
	float myShakeDepthMag = 0.6f;
	float myShakeDecay = 1.3f;
	float myCurrentLifeTime = 0.0f;
};
namespace Engine
{
	class ConstantBufferManager;
}
class Camera
{
public:
	Camera();
	~Camera() = default;
	Camera(const Camera& aCamera);
	Camera& operator=(const Camera& aCamera);
	Camera& operator=(Camera&& aCamera);
	void SetTransform(const CU::Vector3f& aPos, const CU::Vector3f& aRotation);
	void SetPosition(const CU::Vector3f& aPos);
	void SetRotation(const CU::Vector3f& aRotation);
	void SetFrustum(const Camera& aCamera);
	void LookAt(const v3f aWorldPositionToLookAt, const v3f aCameraPosition);

	void Translate(const CU::Vector3f& aMovement, const CU::Vector3f& aRotation);
	void Move(const CU::Vector3f& aMovement);
	void Rotate(const CU::Vector3f& aRotation);

	void RecalculateProjectionMatrix(const float aFovInDegrees, const CU::Vector2f aResolution, bool isPerspective = true, const float aNearPlane = 100.0f, const float aFarPlane = 28000.f, bool aUpdateCluster = true);
	void RecalculateProjectionMatrix(const v2f aResolution);
	void Update();
	bool IsPointInsideFrustum(const v3f aPosition);
	bool IsAABBInsideFrustum(const CU::AABB3Df aCollider);
	bool IsSphereInsideFrustum(const CU::Spheref aSphereToCheck);
	bool IsPointInsideFarRadius(const v3f aPosition);
	const FrustumDrawData GetFrustumDrawData();


	void SetCameraShake(CameraShakeData someData);
	void UpdateCameraShake(const float& aDeltaTime);
	inline const bool& IsShaking() { return myIsShaking; };
	inline const CameraShakeData& GetCameraShakeData() { return myShakeData; };
	void DrawFrustum();

public:
	const float GetNear() { return myNear; }
	const float GetFar() { return myFar; }
	const float GetHalfWNear() { return myHalfWnear; }
	const float GetHalfHNear() { return myHalfHnear; }
	const float GetHalfHFar() { return myHfar; }
	const float GetHalfWFar() { return myWfar ; }
	CU::Matrix4x4f& GetMatrix() { return myTransformMatrix; }
	CU::Transform& GetTransform() { return myTransform; }
	CU::Matrix4x4f& GetProjection() { return myProjection; }
	v2f& GetResolution() { return myScreenResolution; }
	const CameraData GetCameraData();
	CU::AABB3Df GetClusterBounds() { return myClusterBounds; }
	CU::AABB3Df* GetClusterDepth() { return myClusterDepths; }
	CU::AABB3Df* GetClusterWidthHeigth(unsigned int aIndex) { return myOptimizedCluster[aIndex]; }
private:
	friend class ConstantBufferManager;
	void CalculateViewFrustum();
	v4f clipToView(v4f aClipPos, m4f& aInverseProjection);
	v3f screen2View(v4f aScreenCoord, v2f someScreenDimensions, m4f& aInverseProjection);
	v3f lineIntersectionToZPlane(v3f A, v3f B, float zDistance);
	void UpdateClusterBounds();
	CameraShakeData myShakeData;
	CU::Matrix4x4f myTransformMatrix;
	CU::Matrix4x4f myProjection;
	CU::Matrix4x4f myInverseProjection;
	CU::Transform myTransform;
	v2f myScreenResolution;

	CU::VectorOnStack<CU::Planef, 6> myFrustum;
	float myFov;
	float myNear, myFar;
	
	float myHalfHnear;
	float myHalfWnear;
	float myHfar;
	float myWfar;
	float farSquare;

	bool myIsPerspective;
	bool myIsShaking = false;
	CU::AABB3Df myOptimizedCluster[CLUSTER_DEPTH][CLUSTER_HEIGTH * CLUSTER_WIDTH];
	CU::AABB3Df myClusterDepths[CLUSTER_DEPTH];

	CU::AABB3Df myClusterBounds;
	float myClusterPreNumerator = 0;
	float myClusterPreDenominator = 0;
};

