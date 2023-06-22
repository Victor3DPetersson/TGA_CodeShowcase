#pragma once
#include "../CommonUtilities/CU/Math/Color.hpp"
#include "../Engine/GameObjects/Lights.h"

struct LoadedPointLightData
{
	CU::Color myColor;
	float myRange;
	float myIntensity;
	LightFlags myShadows;
};