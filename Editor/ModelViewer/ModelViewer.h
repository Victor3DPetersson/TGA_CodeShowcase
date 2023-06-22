#pragma once

#include "../Engine/ECS/Level/StructExporter.h"
namespace Windows
{
	void ModelViewerInit();
	void ModelViewerDeInit();
	bool ModelViewer(float aDT, void*);
	void SetMVModel(UUID id);
	void SetMVAnimModel(UUID id);

}