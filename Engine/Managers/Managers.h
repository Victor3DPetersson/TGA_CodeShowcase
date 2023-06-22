#pragma once
#include "ModelManager.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include "PhysxManager.h"
#include "..\..\CommonUtilities\CU\Input\InputManager.h"
#include "MaterialManager.h"

namespace Engine
{
	struct EngineManagers
	{
		TextureManager myTextureManager;
		ModelManager myModelManager;
		ParticleManager myParticleManager;
		MaterialManager myMaterialManager;
		CU::InputManager myInputManager;
		PhysxManager myPhysicsManager;
	};

}

