#pragma once

struct MeshesToRender;
namespace Engine
{
	struct RenderData;
	class Renderer;
	class ConstantBufferManager;
	struct RenderStates;
	class EffectResourceManager;
	void HandleDataPackages(RenderData* aMainRenderBuffer, RenderData* aRenderBuffer, Renderer* aRenderer, ConstantBufferManager* aConstantBufferManager, MeshesToRender& meshesToFill, RenderStates* someRenderStates);
	void HandlePreRenderDataPackages(RenderData* aMainRenderBuffer, RenderData* aRenderBuffer, Renderer* aRenderer, ConstantBufferManager* aConstantBufferManager, MeshesToRender& meshesToFill, RenderStates* someRenderStates, EffectResourceManager* aEffectManager);
}