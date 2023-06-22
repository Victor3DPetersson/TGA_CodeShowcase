#pragma once
#include "../CommonUtilities/CU/Math/Color.hpp"
#include "../CommonUtilities/CU/Math/Vector3.hpp"
#include "../Engine/GameObjects/Lights.h"

struct LoadedDirectionalLightData
{
	CU::Color myColor;
	v3f myDirection;
	float myIntensity;
	LightFlags myShadows;
};