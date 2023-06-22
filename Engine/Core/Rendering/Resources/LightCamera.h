#pragma once
#include <..\CommonUtilities\CU\Math\Matrix.hpp>
namespace Engine
{
	struct LightShadowCamera
	{
		CU::Matrix4x4f transform;
		CU::Matrix4x4f projection;
	};
	void RecalculateProjectionMatrix(const float aFovInDegrees, const CU::Vector2f aResolution, bool isPerspective, const float aNearPlane, const float aFarPlane, m4f& aProjection);
	void LookAt(const v3f aWorldPositionToLookAt, const v3f aCameraPosition, m4f& aMatrix);
}