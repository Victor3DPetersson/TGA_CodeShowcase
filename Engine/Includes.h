#pragma once

#include <algorithm>

//Containers
#include "..\CommonUtilities/CU/Containers\VectorOnStack.h"
#include "..\CommonUtilities/CU/Containers\GrowingArray.hpp"
#include "..\CommonUtilities/CU/Containers\VectorOnStack.h"
#include "..\CommonUtilities/CU/Containers\Queue.hpp"
#include "..\CommonUtilities/CU/Containers\Stack.hpp"

#include "..\CommonUtilities/CU/Math\Vector.h"
#include "..\CommonUtilities/CU/Math\Math.h"
#include "..\CommonUtilities/CU/Math\Matrix.hpp"
#include "..\CommonUtilities/CU/Math\Quaternion.h"
#include "../CommonUtilities/CU/Math/Transform.hpp"
#include "..\CommonUtilities/CU/Math\Color.hpp"
#include "..\CommonUtilities/CU/Utility\Random\Random.h"

#include "..\CommonUtilities/CU/Utility\Timer\Timer.h"
#include "..\CommonUtilities/CU/Utility\ShortString.h"

#include "RenderConstants.hpp"

typedef m4f Bone;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define SAFE_DELETE(aPointer) delete aPointer; aPointer = nullptr;
#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(aPointer) if(aPointer){ aPointer->Release();} aPointer = nullptr;
#endif

static CU::RandomHandler Random;
