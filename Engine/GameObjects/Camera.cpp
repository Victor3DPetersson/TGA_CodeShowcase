#include "stdafx.h"
#include "GameObjects/Camera.h"
#include "../../CommonUtilities/CU/Collision/Intersection.hpp"
#include "CU/Noise.h"
#include "EngineInterface.h"
const float PI = 3.1415926f;

Camera::Camera()
{
	myFov = 0;
	myIsPerspective = false;
	myNear = 0; myFar = 0;

	myHalfHnear = 0;
	myHalfWnear = 0;
	myHfar = 0;
	myWfar = 0;
	farSquare = 0;
	CU::Planef plane;
	for (unsigned short i = 0; i < 6; i++)
	{
		myFrustum.Add(plane);
	}
}

Camera::Camera(const Camera& aCamera)
{
	myFov = aCamera.myFov;
	myIsPerspective = aCamera.myIsPerspective;
	myNear = aCamera.myNear;
	myFar = aCamera.myFar;

	myHalfHnear = aCamera.myHalfHnear;
	myHalfWnear = aCamera.myHalfWnear;
	myHfar = aCamera.myHfar;
	myWfar = aCamera.myWfar;

	myTransform = aCamera.myTransform;
	myTransformMatrix = aCamera.myTransformMatrix;
	myProjection = aCamera.myProjection;
	myInverseProjection = aCamera.myInverseProjection;
	myScreenResolution = aCamera.myScreenResolution;

	myFrustum = aCamera.myFrustum;
	myIsShaking = aCamera.myIsShaking;
	myShakeData = aCamera.myShakeData;
	farSquare = aCamera.farSquare;
	myClusterPreNumerator = aCamera.myClusterPreNumerator;
	myClusterPreDenominator = aCamera.myClusterPreDenominator;
	myClusterBounds = aCamera.myClusterBounds;
	memcpy(&myClusterDepths[0], &aCamera.myClusterDepths[0], sizeof(CU::AABB3Df) * CLUSTER_DEPTH);
	memcpy(&myOptimizedCluster[0], &aCamera.myOptimizedCluster[0], sizeof(CU::AABB3Df) * CLUSTER_DEPTH * CLUSTER_HEIGTH * CLUSTER_WIDTH);
}

Camera& Camera::operator=(const Camera& aCamera)
{
	myFov = aCamera.myFov;
	myIsPerspective = aCamera.myIsPerspective;
	myNear = aCamera.myNear;
	myFar = aCamera.myFar;

	myHalfHnear = aCamera.myHalfHnear;
	myHalfWnear = aCamera.myHalfWnear;
	myHfar = aCamera.myHfar;
	myWfar = aCamera.myWfar;

	myTransform = aCamera.myTransform;
	myTransformMatrix = aCamera.myTransformMatrix;
	myProjection = aCamera.myProjection;
	myInverseProjection = aCamera.myInverseProjection;
	myScreenResolution = aCamera.myScreenResolution;

	myFrustum = aCamera.myFrustum;
	myIsShaking = aCamera.myIsShaking;
	myShakeData = aCamera.myShakeData;
	farSquare = aCamera.farSquare;
	myClusterPreNumerator = aCamera.myClusterPreNumerator;
	myClusterPreDenominator = aCamera.myClusterPreDenominator;
	myClusterBounds = aCamera.myClusterBounds;
	memcpy(&myClusterDepths[0], &aCamera.myClusterDepths[0], sizeof(CU::AABB3Df) * CLUSTER_DEPTH);
	memcpy(&myOptimizedCluster[0], &aCamera.myOptimizedCluster[0], sizeof(CU::AABB3Df) * CLUSTER_DEPTH * CLUSTER_HEIGTH * CLUSTER_WIDTH);
	return (*this);
}

Camera& Camera::operator=(Camera&& aCamera)
{
	myFov = aCamera.myFov;
	myIsPerspective = aCamera.myIsPerspective;
	myNear = aCamera.myNear;
	myFar = aCamera.myFar;

	myHalfHnear = aCamera.myHalfHnear;
	myHalfWnear = aCamera.myHalfWnear;
	myHfar = aCamera.myHfar;
	myWfar = aCamera.myWfar;

	myTransform = aCamera.myTransform;
	myTransformMatrix = aCamera.myTransformMatrix;
	myProjection = aCamera.myProjection;
	myInverseProjection = aCamera.myInverseProjection;
	//myRotationAngles = aCamera.myRotationAngles;
	myScreenResolution = aCamera.myScreenResolution;

	myFrustum = aCamera.myFrustum;
	myIsShaking = aCamera.myIsShaking;
	myShakeData = aCamera.myShakeData;
	farSquare = aCamera.farSquare;
	myClusterPreNumerator = aCamera.myClusterPreNumerator;
	myClusterPreDenominator = aCamera.myClusterPreDenominator;
	myClusterBounds = aCamera.myClusterBounds;
	memcpy(&myClusterDepths[0], &aCamera.myClusterDepths[0], sizeof(CU::AABB3Df) * CLUSTER_DEPTH);
	memcpy(&myOptimizedCluster[0], &aCamera.myOptimizedCluster[0], sizeof(CU::AABB3Df) * CLUSTER_DEPTH * CLUSTER_HEIGTH * CLUSTER_WIDTH);

	return (*this);
}

void Camera::SetTransform(const CU::Vector3f& aPos, const CU::Vector3f& aRotation)
{
	myTransform.SetPosition(aPos);
	myTransform.SetRotation(aRotation);
}

void Camera::SetPosition(const CU::Vector3f& aPos)
{
	myTransform.SetPosition(aPos);
}

void Camera::SetRotation(const CU::Vector3f& aRotation)
{
	myTransform.SetRotation(aRotation);
}

void Camera::SetFrustum(const Camera& aCamera)
{
	myProjection = aCamera.myProjection;

	myFov = aCamera.myFov;
	myNear = aCamera.myNear;
	myFar = aCamera.myFar;
	myHalfHnear = aCamera.myHalfHnear;
	myHalfWnear = aCamera.myHalfWnear;
	myHfar = aCamera.myHfar;
	myWfar = aCamera.myWfar;
	farSquare = aCamera.farSquare;
	myIsPerspective = aCamera.myIsPerspective;

}

void Camera::LookAt(const v3f aWorldPositionToLookAt, const v3f aCameraPosition)
{
	myTransform.LookAt(aWorldPositionToLookAt, aCameraPosition);
}

void Camera::Translate(const CU::Vector3f& aMovement, const CU::Vector3f&)
{
	myTransform.AddToPosition(aMovement);
}

void Camera::Move(const CU::Vector3f& aMovement)
{
	myTransform.AddToPosition(aMovement);
}

void Camera::Rotate(const CU::Vector3f& aRotation)
{
	myTransform.AddToRotation(aRotation);
}

void Camera::RecalculateProjectionMatrix(const float aFovInDegrees, const CU::Vector2f aResolution, bool isPerspective, const float aNearPlane, const float aFarPlane, bool aUpdateCluster)
{
	farSquare = aFarPlane * aFarPlane;
	myFov = aFovInDegrees;
	myIsPerspective = isPerspective;
	myNear = aNearPlane;
	myFar = aFarPlane;
	myScreenResolution = aResolution;
	myProjection = m4f::Identity;
	if (isPerspective)
	{
		float hFovRad = aFovInDegrees * (PI / 180);

		float vFovRad = 2 * atanf(tanf(hFovRad / 2) * (aResolution.y / aResolution.x));

		float myXScale = 1 / tanf(hFovRad * 0.5f);
		float myYScale = 1 / tanf(vFovRad * 0.5f);
		float Q = aFarPlane / (aFarPlane - aNearPlane);

		myProjection(1, 1) = myXScale;
		myProjection(2, 2) = myYScale;
		myProjection(3, 3) = Q;
		myProjection(3, 4) = 1.0f / Q;
		myProjection(4, 3) = -Q * aNearPlane;
		myProjection(4, 4) = 0.0f;

		float aspectRatio = (aResolution.x / aResolution.y);
		myHalfHnear = (tan(hFovRad / 2)) * myNear / aspectRatio;
		myHalfWnear = myHalfHnear * aspectRatio;

		myHfar = (tan(hFovRad / 2)) * myFar / aspectRatio;
		myWfar = myHfar * aspectRatio;
	}
	else
	{
		myProjection(1, 1) = 2.0f / aResolution.x;
		myProjection(2, 2) = 2.0f / aResolution.y;
		myProjection(3, 3) = 1.0f / (aFarPlane - aNearPlane);
		myProjection(4, 3) = -aNearPlane / (aFarPlane - aNearPlane);
		myProjection(4, 4) = 1.0f;
	}
	if (aUpdateCluster)
	{
		UpdateClusterBounds();
		myClusterPreNumerator = log10f(myFar / myNear);
		myClusterPreDenominator = CLUSTER_DEPTH_GPU * log10f(myNear);
	}
	myInverseProjection = CU::Matrix4x4<float>::GetInverse(myProjection);
}

void Camera::RecalculateProjectionMatrix(const v2f aResolution)
{
	if (aResolution.x != myScreenResolution.x || aResolution.y != myScreenResolution.y)
	{
		RecalculateProjectionMatrix(myFov, aResolution, myIsPerspective, myNear, myFar);
	}
}

void Camera::Update()
{
	CalculateViewFrustum();
}

void Camera::SetCameraShake(CameraShakeData someData)
{
	//if (myIsShaking) return;

	myIsShaking = true;
	myShakeData.myPerlinOffset = v3f();
	myShakeData.myOriginalPosition = GetTransform().GetPosition();
	myShakeData.myShakeForce = someData.myShakeForce;
	myShakeData.myShakeMultiplier = someData.myShakeMultiplier;
	myShakeData.myShakeMagnitude = someData.myShakeMagnitude;
	myShakeData.myShakeDepthMag = someData.myShakeDepthMag;
	myShakeData.myShakeDecay = someData.myShakeDecay;
	myShakeData.myCurrentLifeTime = 0.0f;
}

void Camera::UpdateCameraShake(const float& aDeltaTime)
{
	if (myIsShaking)
	{
		CU::PerlinNoise perlin;
		//increase the time counter (how fast the position changes) based off the traumaMult and some root of the Trauma
		myShakeData.myCurrentLifeTime += aDeltaTime * std::powf(myShakeData.myShakeForce, 0.3f) * myShakeData.myShakeMultiplier;
		//Bind the movement to the desired range
		myShakeData.myPerlinOffset = v3f(perlin.Perlin(1, myShakeData.myCurrentLifeTime) * 2.0f, perlin.Perlin(10, myShakeData.myCurrentLifeTime) * 2.0f,
			myShakeData.myShakeDepthMag * perlin.Perlin(100, myShakeData.myCurrentLifeTime)) * myShakeData.myShakeMagnitude * myShakeData.myShakeForce * 100.0f;

		//decay faster at higher values
		myShakeData.myShakeForce -= aDeltaTime * myShakeData.myShakeDecay * (myShakeData.myShakeForce + 0.3f);
		if (myShakeData.myShakeForce <= 0.0f)
		{
			myIsShaking = false;
			myShakeData.myPerlinOffset = v3f();
		}
	}
}

void Camera::DrawFrustum()
{
	FrustumDrawData drawData = GetFrustumDrawData();
	EngineInterface::DrawLine(drawData.fTR, drawData.nTR);
	EngineInterface::DrawLine(drawData.fTR, drawData.fTL);
	EngineInterface::DrawLine(drawData.fTR, drawData.fBR);

	EngineInterface::DrawLine(drawData.fBR, drawData.nBR);
	EngineInterface::DrawLine(drawData.fBR, drawData.fBL);

	EngineInterface::DrawLine(drawData.fBL, drawData.fTL);
	EngineInterface::DrawLine(drawData.fBL, drawData.nBL);

	EngineInterface::DrawLine(drawData.nBL, drawData.nBR);
	EngineInterface::DrawLine(drawData.nBL, drawData.nTL);

	EngineInterface::DrawLine(drawData.nTR, drawData.nBR);
	EngineInterface::DrawLine(drawData.nTR, drawData.nTL);
	EngineInterface::DrawLine(drawData.nTL, drawData.fTL);
}


bool Camera::IsPointInsideFrustum(const v3f aPosition)
{
	//const v3f camToModel = aPosition - myTransformMatrix.GetTranslationVector();
	//float distance = (aPosition - myTransformMatrix.GetTranslationVector()).LengthSqr();
	//if (distance < farSquare && camToModel.Dot(myTransformMatrix.GetForwardVector()) > 0)
	//{
	//	return true;
	//}
	for (unsigned short plane = 0; plane < 6; plane++)
	{
		if (!myFrustum[plane].IsInside(aPosition))
		{
			return false;
		}
	}
	return true;
}

bool Camera::IsAABBInsideFrustum(const CU::AABB3Df aCollider)
{
	const float scaleFactor = 3.0f;
	const v3f size = (aCollider.myMax - aCollider.myMin);
	v3f boundPoints[8];
	boundPoints[0] = aCollider.myMax;
	boundPoints[1] = { aCollider.myMax.x - size.x, aCollider.myMax.y, aCollider.myMax.z };
	boundPoints[2] = { aCollider.myMax.x, aCollider.myMax.y, aCollider.myMax.z - size.z };
	boundPoints[3] = { aCollider.myMax.x - size.x, aCollider.myMax.y, aCollider.myMax.z - size.z };
	boundPoints[4] = aCollider.myMin;
	boundPoints[5] = { aCollider.myMin.x + size.x, aCollider.myMin.y, aCollider.myMin.z };;
	boundPoints[6] = { aCollider.myMin.x, aCollider.myMin.y, aCollider.myMin.z + size.z };;
	boundPoints[7] = { aCollider.myMin.x + size.x, aCollider.myMin.y, aCollider.myMin.z + size.z };;

	int outCounter = 0;
	for (unsigned short i = 0; i < 6; i++)
	{
		int inCount = 8;
		for (unsigned int p = 0; p < 8; p++)
		{
			if (!myFrustum[i].IsInside(boundPoints[p]))
			{
				inCount--;
			}
		}
		if (inCount <= 0)
		{
			outCounter++;
		}
	}
	if (outCounter >= 1)
	{
		return false;
	}

	return true;
}

bool Camera::IsSphereInsideFrustum(const CU::Spheref aSphereToCheck)
{
	float distance;
	for (unsigned short int i = 0; i < 6; i++)
	{
		distance = (aSphereToCheck.myPos - myFrustum[i].myPosition).Dot(myFrustum[i].myNormal);
		if (distance < aSphereToCheck.myRadius)
		{
			return false;
		}
	}
	return true;
}

bool Camera::IsPointInsideFarRadius(const v3f aPosition)
{
	float distance = (aPosition - myTransformMatrix.GetTranslationVector()).LengthSqr();
	if (distance < farSquare)
	{
		return true;
	}
	return false;
}

const FrustumDrawData Camera::GetFrustumDrawData()
{
	FrustumDrawData data;
	v3f up = myTransform.GetUp();
	v3f right = myTransform.GetRight();
	v3f pos = myTransform.GetPosition();

	CU::Vector3f farCent = myTransform.GetForward() * myFar;
	//m3f rotation = myTransform.GetMatrix();
	//if (rotation(1, 1) == 0 && rotation(2, 2) == 0 && rotation(3, 3) == 0)
	//{
	//	rotation = m3f();
	//}
	data.fTR = (farCent + (up * (myHfar)+(right * myWfar))) /* rotation*/ + myTransform.GetPosition();
	data.fTL = (farCent + (up * (myHfar)-(right * myWfar))) /* rotation*/ + myTransform.GetPosition();
	data.fBL = (farCent - (up * (myHfar)+(right * myWfar))) /* rotation*/ + myTransform.GetPosition();
	data.fBR = (farCent - (up * (myHfar)-(right * myWfar))) /* rotation*/ + myTransform.GetPosition();


	CU::Vector3f nearCent = myTransform.GetForward() * myNear;

	data.nTL = (nearCent + (up * (myHalfHnear)-(right * myHalfWnear))) /* rotation */ + myTransform.GetPosition();
	data.nTR = (nearCent + (up * (myHalfHnear)+(right * myHalfWnear))) /* rotation */ + myTransform.GetPosition();
	data.nBL = (nearCent - (up * (myHalfHnear)+(right * myHalfWnear))) /* rotation */ + myTransform.GetPosition();
	data.nBR = (nearCent - (up * (myHalfHnear)-(right * myHalfWnear))) /* rotation */ + myTransform.GetPosition();
	return data;
}

const CameraData Camera::GetCameraData()
{
	CameraData data;
	data.CProjection = myProjection;
	data.CInvProjection = myInverseProjection;
	data.toCTransform = m4f::GetFastInverse(myTransformMatrix);
	data.fromCamera = myTransformMatrix;
	data.myNear = myNear;
	data.myFar = myFar;
	data.clusterPreDenominator = myClusterPreDenominator;
	data.clusterPreNumerator = myClusterPreNumerator;
	return data;
}

void Camera::CalculateViewFrustum()
{
	v3f up = myTransform.GetUp();
	v3f right = myTransform.GetRight();

	CU::Vector3f farCent = myTransform.GetPosition() + myTransform.GetForward() * myFar;

	CU::Vector3f farTopR = farCent + (up * (myHfar)+(right * myWfar));

	CU::Vector3f farBotL = farCent - (up * (myHfar)+(right * myWfar));

	CU::Vector3f farBotR = farCent - (up * (myHfar)-(right * myWfar));

	CU::Vector3f nearCent = myTransform.GetPosition() + myTransform.GetForward() * myNear;

	CU::Vector3f nearTopL = nearCent + (up * (myHalfHnear)-(right * myHalfWnear));

	CU::Vector3f nearTopR = nearCent + (up * (myHalfHnear)+(right * myHalfWnear));

	CU::Vector3f nearBotL = nearCent - (up * (myHalfHnear)+(right * myHalfWnear));

	CU::Vector3f nearBotR = nearCent - (up * (myHalfHnear)-(right * myHalfWnear));

	myFrustum[0].InitWithPointAndNormal(farCent, (myTransform.GetForward() * -1.0f));
	myFrustum[1].InitWithPointAndNormal(nearCent, myTransform.GetForward());

	myFrustum[2].InitWith3Points(nearBotR, farBotR, nearTopR);
	myFrustum[3].InitWith3Points(nearBotL, nearTopL, farBotL);

	myFrustum[4].InitWith3Points(nearTopR, farTopR, nearTopL);
	myFrustum[5].InitWith3Points(nearBotL, farBotR, nearBotR);
}


v4f Camera::clipToView(v4f aClipPos, m4f& aInverseProjection) {
	//View space transform
	v4f view = aInverseProjection * aClipPos;

	//Perspective projection
	view = view / view.w;

	return view;
}

v3f Camera::screen2View(v4f aScreenCoord, v2f someScreenDimensions, m4f& aInverseProjection) {
	//Convert to NDC
	v2f texCoord = v2f(aScreenCoord.x, aScreenCoord.y) / someScreenDimensions;

	//Convert to clipSpace
	// vec4 clip = vec4(vec2(texCoord.x, 1.0 - texCoord.y)* 2.0 - 1.0, screen.z, screen.w);
	v4f clip = v4f(texCoord.x * 2.0f - 1.0f, texCoord.y * 2.0f - 1.0f, aScreenCoord.z, aScreenCoord.w);
	//Not sure which of the two it is just yet
	v4f viewVector = clipToView(clip, aInverseProjection);

	return { viewVector.x, viewVector.y, viewVector.z };
}

v3f Camera::lineIntersectionToZPlane(v3f A, v3f B, float zDistance) {
	//Because this is a Z based normal this is fixed
	v3f normal = v3f(0.0, 0.0, 1.0);

	v3f ab = B - A;

	//Computing the intersection length for the line and the plane
	float t = (zDistance - normal.Dot(A)) / normal.Dot(ab);

	//Computing the actual xyz position of the point along the line
	v3f result = A + t * ab;

	return result;
}

void Camera::UpdateClusterBounds()
{
	//Create every AABB in the cluster
	const unsigned short widthHeigth = (unsigned short)(CLUSTER_WIDTH * CLUSTER_HEIGTH);

	m4f inverseProjection = m4f::GetInverse(myProjection);

	v3f min, max;

	const v3f eyePos;
	for (unsigned short d = 0; d < CLUSTER_DEPTH; d++)
	{
		const float Z = myNear * powf(myFar / myNear, (((float)d) / (float)CLUSTER_DEPTH));
		const float nextZ = myNear * powf(myFar / myNear, (((float)d + 1) / (float)CLUSTER_DEPTH));
		v3f minDepth, maxDepth;
		minDepth.z = Z;
		for (unsigned short h = 0; h < CLUSTER_HEIGTH; h++)
		{
			for (unsigned short w = 0; w < CLUSTER_WIDTH; w++)
			{
				v4f maxPointSS = v4f((myScreenResolution.x / CLUSTER_WIDTH) * (w + 1), (myScreenResolution.y / CLUSTER_HEIGTH) * (CLUSTER_HEIGTH - h), nextZ, 1);
				v4f minPointSS = v4f((myScreenResolution.x / CLUSTER_WIDTH) * w, (myScreenResolution.y / CLUSTER_HEIGTH) * (CLUSTER_HEIGTH - h - 1), Z, 1);

				v3f minPointVS = screen2View(minPointSS, myScreenResolution, inverseProjection);
				v3f maxPointVS = screen2View(maxPointSS, myScreenResolution, inverseProjection);
				v3f minPointNear = lineIntersectionToZPlane(eyePos, minPointVS, Z);
				v3f minPointFar = lineIntersectionToZPlane(eyePos, minPointVS, nextZ);
				v3f maxPointNear = lineIntersectionToZPlane(eyePos, maxPointVS, Z);
				v3f maxPointFar = lineIntersectionToZPlane(eyePos, maxPointVS, nextZ);

				min.x = CU::Min(minPointNear.x, minPointFar.x);
				min.y = CU::Min(minPointNear.y, minPointFar.y);
				max.x = CU::Max(maxPointNear.x, maxPointFar.x);
				max.y = CU::Max(maxPointNear.y, maxPointFar.y);

				min.z = CU::Min(minPointNear.z, minPointFar.z) - nextZ * 0.15f; // some compensation to create a tiny overlap
				max.z = CU::Max(maxPointNear.z, maxPointFar.z) + nextZ * 0.05f;

				if (min.x < myClusterBounds.myMin.x) { myClusterBounds.myMin.x = min.x; }
				if (min.y < myClusterBounds.myMin.y) { myClusterBounds.myMin.y = min.y; }
				if (min.z < myClusterBounds.myMin.z) { myClusterBounds.myMin.z = min.z; }
				if (max.x > myClusterBounds.myMax.x) { myClusterBounds.myMax.x = max.x; }
				if (max.y > myClusterBounds.myMax.y) { myClusterBounds.myMax.y = max.y; }
				if (max.z > myClusterBounds.myMax.z) { myClusterBounds.myMax.z = max.z; }

				if (min.x < minDepth.x) { minDepth.x = min.x; }
				if (min.y < minDepth.y) { minDepth.y = min.y; }
				if (min.z < minDepth.z) { minDepth.z = min.z; }
				if (max.x > maxDepth.x) { maxDepth.x = max.x; }
				if (max.y > maxDepth.y) { maxDepth.y = max.y; }
				if (max.z > maxDepth.z) { maxDepth.z = max.z; }

				//myCluster[d * widthHeigth + h * (unsigned short)(CLUSTER_WIDTH) + w] = CU::AABB3Df(min, max);
				myOptimizedCluster[d][h * (unsigned short)(CLUSTER_WIDTH)+w] = CU::AABB3Df(min, max);
			}
		}
		myClusterDepths[d] = CU::AABB3Df(minDepth, maxDepth);
	}
}
