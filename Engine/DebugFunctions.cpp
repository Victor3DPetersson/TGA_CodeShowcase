#include "stdafx.h"
#ifdef _DEBUG
#include "DebugFunctions.h"
#include "../Engine/EngineInterface.h"
#include "../Engine/NavMesh.h"

void Debug::DrawNavMesh(const NavMesh& aNavMesh)
{
	for (unsigned int i = 0; i < aNavMesh.triCount; ++i)
	{
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[i].x], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[i].y]);
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[i].y], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[i].z]);
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[i].z], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[i].x]);
	}//
}

void Debug::DrawTriangleNeighbors(const NavMesh& aNavMesh, const int& aTriangle)
{
	if (aTriangle == -1)
	{
		return;
	}
	int* neighbors = &aNavMesh.nodeBuffer[aTriangle].myNeighbours[0];
	const unsigned int neighbourCount = aNavMesh.nodeBuffer[aTriangle].myNeighbourCount;

	for (unsigned int i = 0; i < neighbourCount; ++i)
	{
		//Drawing neighbors to triangle
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[neighbors[i]].x], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[neighbors[i]].y], { 255, 255, 0, 1 }, 2);
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[neighbors[i]].y], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[neighbors[i]].z], { 255, 255, 0, 1 }, 2);
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[neighbors[i]].z], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[neighbors[i]].x], { 255, 255, 0, 1 }, 2);
	}

	//Drawing current triangle
	EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aTriangle].x], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aTriangle].y], { 0, 255, 0, 1 }, 2);
	EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aTriangle].y], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aTriangle].z], { 0, 255, 0, 1 }, 2);
	EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aTriangle].z], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aTriangle].x], { 0, 255, 0, 1 }, 2);
}

void Debug::DrawPortals(const NavMesh& aNavMesh)
{
	for (unsigned int i = 0; i < aNavMesh.leftPortalSize; ++i)
	{
		EngineInterface::DrawLine(aNavMesh.leftPortals[i], aNavMesh.leftPortals[i] + v3f(0.0f, 100.0f, 0.0f), { 248, 168, 6, 1 });
	}

	for (unsigned int i = 0; i < aNavMesh.rightPortalSize; ++i)
	{
		EngineInterface::DrawLine(aNavMesh.rightPortals[i], aNavMesh.rightPortals[i] + v3f(0.0f, 100.0f, 0.0f), { 0, 0, 255, 1 });
	}
}

void Debug::DrawPath(const std::vector<v3f>& aPath)
{
	if (aPath.size() > 1)
	{
		for (int i = 0; i < aPath.size() - 1; ++i)
		{
			EngineInterface::DrawLine(aPath[i], aPath[i + 1], { 255, 0, 0, 1 }, 2.0f);
		}
	}
}

void Debug::DrawTrianglePath(const NavMesh& aNavMesh)
{

	for (unsigned int pathIndex = 0; pathIndex < aNavMesh.pathSize; ++pathIndex)
	{
		//Drawing current triangle
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aNavMesh.pathBuffer[pathIndex]].x], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aNavMesh.pathBuffer[pathIndex]].y], { 255, 0, 0, 1 }, 2);
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aNavMesh.pathBuffer[pathIndex]].y], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aNavMesh.pathBuffer[pathIndex]].z], { 255, 0, 0, 1 }, 2);
		EngineInterface::DrawLine(aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aNavMesh.pathBuffer[pathIndex]].z], aNavMesh.vertexBuffer[aNavMesh.triangleBuffer[aNavMesh.pathBuffer[pathIndex]].x], { 255, 0, 0, 1 }, 2);
	}
}

//void Debug::DrawBlockage(Scene* aScene)
//{
//	Engine::EngineInterface* engine = Engine::EngineInterface::GetInstance();
//
//	for (int i = 0; i < 32; ++i)
//	{
//		if (aScene->myBlockages[i].myIsActive)
//		{
//			const unsigned int blockageCount = aScene->myBlockages[i].myBlockageCount;
//			for (unsigned int blockage = 0; blockage < blockageCount; ++blockage)
//			{
//				engine->DrawLine(aScene->myBlockages[i].myTriangles[blockage][0], aScene->myBlockages[i].myTriangles[blockage][1], { 255, 0, 0, 1 }, 2);
//				engine->DrawLine(aScene->myBlockages[i].myTriangles[blockage][1], aScene->myBlockages[i].myTriangles[blockage][2], { 255, 0, 0, 1 }, 2);
//				engine->DrawLine(aScene->myBlockages[i].myTriangles[blockage][2], aScene->myBlockages[i].myTriangles[blockage][0], { 255, 0, 0, 1 }, 2);
//			}
//		}
//	}
//}
#endif //DEBUG
