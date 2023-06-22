#pragma once
#include "../CommonUtilities/CU/Math/Color.hpp"
#include "../CommonUtilities/CU/Math/Vector3.hpp"
#include "GameObjects\Lights.h"

struct LoadedSpotLightData
{
	CU::Color myColor;
	v3f myDirection;
	float myIntensity;
	float myRange;
	float myAngle;
	LightFlags myShadows;
};