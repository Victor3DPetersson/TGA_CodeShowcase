#pragma once

#include <stdint.h>

#include "../CommonUtilities/CU/Utility/FixedString.hpp"
#include "../CommonUtilities/CU/Utility/SIMDString32.h"
#include "../CommonUtilities/CU/Containers/VectorOnStack.h"

#include "../Editor/Misc/IconsFontAwesome5.h"

namespace Filebrowser
{
	struct FBResult
	{
		FixedWString256 selected;
		FixedWString256 doubleClicked;
		FixedWString256 cwd;
		FixedWString256 hovered;
		bool close = false;
		FixedWString256 root;
	};

	FBResult Update(int id, FixedWString256 cwd, const VictorOnCrack<FixedWString256, 16, size_t>& filter);
	FBResult UpdateModal(FixedString256 title, int id, FixedWString256 cwd,
		const VictorOnCrack<FixedWString256, 16, size_t>& filter, const SIMDString32& okayLabel = ICON_FA_CHECK " Select");
	void PushModal(FixedString256 title, int id);
}