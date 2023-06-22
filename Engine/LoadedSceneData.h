#pragma once
#include "ObjectData.h"

struct LoadedSceneData
{
	v4f myAmbientLight;
	std::vector<ObjectData> myObjects;
};