#pragma once
#include "ForwardRenderer.h"
#include "FullScreenRenderer.h"
#include "DeferredRenderer.h"
#include "DebugRenderer.h"
#include "SpriteRenderer.h"
#include "ParticleRenderer.h"
#include "ShadowRenderer.h"
#include "CubemapRenderer.h"
namespace Engine
{
	struct Renderers
	{
		Renderers() = default;
		ForwardRenderer Forward;
		FullScreenRenderer Fullscreen;
		DeferredRenderer Deferred;
#ifndef _DISTRIBUTION
		DebugRenderer Debug;
#endif
		SpriteRenderer Sprite;
		ParticleRenderer Particle;
		ShadowRenderer Shadow;
		CubemapRenderer Cube;
	};
}
