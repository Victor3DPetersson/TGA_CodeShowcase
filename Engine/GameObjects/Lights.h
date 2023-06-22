#pragma once
#include "../../CommonUtilities/CU/Math\Color.hpp"
#include "../../CommonUtilities/CU/Math\Vector3.hpp"
#include "Camera.h"

#include "../Engine/ECS/SerializedEnums.hpp"

class Camera; 
struct DirectionalLight_Data
{
	CU::Color myColor ={255, 240, 230, 128};
	CU::Vector4f myDirection = {1, 0, 0, 0};
};

struct PointLightRenderCommand
{
	CU::Color color;
	v3f position;
	float range;
	v3f pointPadding;
	float size;
};

struct SpotLightRenderCommand
{
	CU::Color color;
	v3f position;
	float range;
	v3f direction;
	float angle;
	v3f spotPadding;
	float size;
};
