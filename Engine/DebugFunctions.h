#ifdef _DEBUG
#pragma once
#include <vector>

struct NavMesh;

namespace Debug
{
	void DrawNavMesh(const NavMesh& aNavMesh);
	void DrawTriangleNeighbors(const NavMesh& aNavMesh, const int& aTriangle);
	void DrawPortals(const NavMesh& aNavMesh);
	void DrawPath(const std::vector<v3f>& aPath);
	void DrawTrianglePath(const NavMesh& aNavMesh);
	//void DrawBlockage();
}
#endif //DEBUG
