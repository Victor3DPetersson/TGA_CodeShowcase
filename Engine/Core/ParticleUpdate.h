#pragma once
#include "../../CommonUtilities/CU/Math/Matrix4x4.hpp"
struct ParticleRenderCommand;
struct ParticleEmitterComponent;
struct ParticleCommand;
struct ParticleEmissionData;
class ParticleEffect;
namespace Engine
{
	class ParticleManager;
	struct ParticleSystem;

	void UpdateParticleComponent(ParticleEffect& someSettings, ParticleEmissionData& someEmissionData, ParticleRenderCommand* aCommandToFill, ParticleCommand* aPC, const float aDT);
	void UpdateDepthFromCamera(ParticleRenderCommand* aCommandToSort, const v3f aCameraPosition);
}