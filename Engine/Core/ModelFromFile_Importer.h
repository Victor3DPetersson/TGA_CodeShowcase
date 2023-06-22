#pragma once

#include "GameObjects/Model.h"
#include "CU/Utility/ShortString.h"
#include "CU/Containers\GrowingArray.hpp"
#include "CU/Math\Matrix4x4.hpp"

namespace Engine
{
	class TextureManager;
	class MaterialManager;
	class ModelFromFile_Importer
	{
	public:
		bool ImportGratFile(const ShortString& aModelName, ModelData** someModelDataToFill, unsigned short& aAmountOfSubModels, MaterialManager* aMaterialManager, bool& aHasGUID);
		bool ImportGratMotorikFile(const ShortString& aModelName, ModelAnimated* aModelToFill, MaterialManager* aMaterialManager, bool& aHasGUID);
		GUID GetGUIDFromGratFile(const ShortString& aModelName);
		GUID GetGUIDFromGratMotorikFile(const ShortString& aModelName);
	};
}

