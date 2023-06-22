#include "stdafx.h"
#include "TransformSystem.h"
#include "../../Components.h"
#include "../Engine/ECS/HashGrid/HashGrid.h"

void TransformSystem::RemoveEntityFromGrid(Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg)
{
	TransformComponent& comp = trs.GetComponent(anEntity);
	const v3f trans = comp.matrix.GetTranslationVector();
	CU::AABB2D boundingBox = comp.boundingBox;
	boundingBox.myMin.x += trans.x;
	boundingBox.myMin.y += trans.z;
	boundingBox.myMax.x += trans.x;
	boundingBox.myMax.y += trans.z;

	hg.Remove(boundingBox, anEntity);
}

void TransformSystem::AddEntityToGrid(Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg)
{
	TransformComponent& comp = trs.GetComponent(anEntity);
	const v3f trans = comp.matrix.GetTranslationVector();
	CU::AABB2D boundingBox = comp.boundingBox;
	boundingBox.myMin.x += trans.x;
	boundingBox.myMin.y += trans.z;
	boundingBox.myMax.x += trans.x;
	boundingBox.myMax.y += trans.z;

	hg.Insert(boundingBox, anEntity);
}

void TransformSystem::Translate(const v3f& aTranslation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg)
{
	RemoveEntityFromGrid(anEntity, trs, hg);
	trs.GetComponent(anEntity).matrix.AddToTranslation(aTranslation);
	AddEntityToGrid(anEntity, trs, hg);
}

void TransformSystem::Rotate(const m4f& aRotation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg)
{
	RemoveEntityFromGrid(anEntity, trs, hg);
	auto& component = trs.GetComponent(anEntity);
	component.matrix = aRotation * component.matrix;
	AddEntityToGrid(anEntity, trs, hg);
}

void TransformSystem::SetTranslation(const v3f& aTranslation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg)
{
	RemoveEntityFromGrid(anEntity, trs, hg);
	trs.GetComponent(anEntity).matrix.SetTranslation(aTranslation);
	AddEntityToGrid(anEntity, trs, hg);
}

void TransformSystem::SetRotation(const v3f& aRotation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg)
{
	RemoveEntityFromGrid(anEntity, trs, hg);
	trs.GetComponent(anEntity).matrix.SetRotation(aRotation);
	AddEntityToGrid(anEntity, trs, hg);
}

void TransformSystem::SetRotation(const m4f& aRotation, Entity anEntity, ComponentArray<TransformComponent>& trs, HashGrid& hg)
{
	RemoveEntityFromGrid(anEntity, trs, hg);
	trs.GetComponent(anEntity).matrix.SetRotation(aRotation);
	AddEntityToGrid(anEntity, trs, hg);
}
