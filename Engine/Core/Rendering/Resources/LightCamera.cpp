#include "stdafx.h"
#include "LightCamera.h"
#include <CU\Math\Math.h>

void Engine::RecalculateProjectionMatrix(const float aFovInDegrees, const CU::Vector2f aResolution, bool isPerspective, const float aNearPlane, const float aFarPlane, m4f& aProjection)
{
	aProjection = m4f();
	if (isPerspective)
	{
		float hFovRad = aFovInDegrees * (CU::pif / 180);

		float vFovRad = 2 * atanf(tanf(hFovRad / 2) * (aResolution.y / aResolution.x));

		float myXScale = 1 / tanf(hFovRad * 0.5f);
		float myYScale = 1 / tanf(vFovRad * 0.5f);
		float Q = aFarPlane / (aFarPlane - aNearPlane);

		aProjection(1, 1) = myXScale;
		aProjection(2, 2) = myYScale;
		aProjection(3, 3) = Q;
		aProjection(3, 4) = 1.0f / Q;
		aProjection(4, 3) = -Q * aNearPlane;
		aProjection(4, 4) = 0.0f;
	}
	else
	{
		aProjection(1, 1) = 2.0f / aResolution.x;
		aProjection(2, 2) = 2.0f / aResolution.y;
		aProjection(3, 3) = 1.0f / (aFarPlane - aNearPlane);
		aProjection(4, 3) = -aNearPlane / (aFarPlane - aNearPlane);
		aProjection(4, 4) = 1.0f;
	}
}

void Engine::LookAt(const v3f aWorldPositionToLookAt, const v3f aCameraPosition, m4f& aMatrix)
{
	v3f forward = aWorldPositionToLookAt - aCameraPosition;
	aMatrix.MakeRotationDir(forward);
	aMatrix.SetTranslation(aCameraPosition);
}
