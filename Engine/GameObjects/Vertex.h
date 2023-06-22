#pragma once
#include "../../CommonUtilities/CU/Math/Vector.h"


struct Vertex_PBR
{
	CU::Vector3f myPosition;
	CU::Vector3f myNormal;
	CU::Vector3f myBinormal;
	CU::Vector3f myTangent;
	CU::Color myColor;

	CU::Vector2f myUVCoords;
	CU::Vector2f myUVCoords1;
	
	CU::Vector2f myUVCoords2;
	CU::Vector2f myUVCoords3;
};

#define NUM_BONES_PER_VERTEX 4

struct Vertex_PBR_Animated
{
	CU::Color myColor;
	CU::Vector3f myPosition;
	CU::Vector3f myNormal;
	CU::Vector3f myBinormal;
	CU::Vector3f myTangent;

	CU::Vector2f myUVCoords;
	CU::Vector2f myUVCoords1;

	CU::Vector2f myUVCoords2;
	CU::Vector2f myUVCoords3;
	unsigned int IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];
};

struct Vertex_Particle
{
	CU::Color myColor;
	v3f myPosition;
	v3f myVelocity;
	v2f mySize;
	float myLifetime{};
	float myEndTime{};
	float myDistanceToCamera{};
	float myEmissiveStrength{};
	float myRotation{};
	float myRotationDir{};
	float myUVPanningSpeed{};
	unsigned int myCurrentColor;
	float myRotationX{};
	float myRotationY{};
	float mySizeZ{};
	unsigned int myBillBoardState{};
};
struct Vertex_Sprite
{
	CU::Color myColor;
	v2f myPosition;
	v2f mySize;
	v2f myUVOffsetTopL;
	v2f myUVOffsetBotR;
	v2f myPivotOffset;
	float myRotation{};
	v2f data;
	int myZIndex;
	v2i PADDING;
};
struct Vertex_World_Sprite
{
	CU::Color myColor;
	v3f myPosition;
	v2f mySize;
	v2f myUVOffsetTopL;
	v2f myUVOffsetBotR;
	v2f myPivotOffset;
	float myRotation{};
};
struct Vertex_Debug_Line
{
	CU::Color myColor;
	v4f myPosFrom;
	v4f myPosTo;
	float mySize{};
	v3f myTrashFill;
};
struct Vertex_Debug_Mesh
{
	v4f myPosition;
};


