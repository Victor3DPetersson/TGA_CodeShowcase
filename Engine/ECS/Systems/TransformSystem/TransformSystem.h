#pragma once
#include "../Engine/ECS/ECS.h"
#include "../Engine/ECS/Components.h"
#include "../Engine/ECS/ComponentArray.h"

class HashGrid;

namespace TransformSystem
{
	void RemoveEntityFromGrid(Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg);
	void AddEntityToGrid(Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg);

	void Translate(const v3f& aTranslation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg);
	void Rotate(const m4f& aRotation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg);
	void SetTranslation(const v3f& aTranslation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg);
	void SetRotation(const v3f& aRotation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg);
	void SetRotation(const m4f& aRotation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg);
}