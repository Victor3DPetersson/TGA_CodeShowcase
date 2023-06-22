#include "stdafx.h"
#include "EngineInterface.h"
#include "Managers\Managers.h"
#include "Core\Engine.h"
#include "Core\Rendering/Renderer.h"
#include "Core/WindowHandler.h"
#include "../Engine/ToolsMenu/ToolsMenu.h"
#include "../CommonUtilities/CU/Math/Matrix3x3.hpp"
#include "RenderData.h"
#include "Core/StartAttributes.h"

void EngineInterface::ShutDownEngine()
{
	Engine::Core::ShutDownEngine();
}

Utility::DragNDropHandler* EngineInterface::GetDragNDropHandler()
{
	return &Engine::globalEngineData.dragNDropHandler;
}

v2ui EngineInterface::GetEnumResolution(uint16_t aResolution)
{
	v2ui resolution;
	switch (aResolution)
	{
	case Engine::E2160p:
		resolution = v2ui(3840, 2160);
		break;
	case Engine::E1440p:
		resolution = v2ui(2560, 1440);
		break;
	case Engine::E1080p:
		resolution = v2ui(1920, 1080);
		break;
	case Engine::E720p:
		resolution = v2ui(1280, 720);
		break;
	case Engine::E480p:
		resolution = v2ui(854, 480);
		break;
	case Engine::E360p:
		resolution = v2ui(640, 360);
		break;
	default:
		break;
	}
	return resolution;
}

uint16_t EngineInterface::GetERenderResolution()
{
	return Engine::globalEngineData.userSetRes;
}

uint16_t EngineInterface::GetEWindowResolution()
{
	return Engine::globalEngineData.userSetWindowRes;
}

Engine::EngineManagers* EngineInterface::GetManagers()
{
	return &Engine::globalEngineData.managers;
}

Engine::TextureManager* EngineInterface::GetTextureManager()
{
	return &Engine::globalEngineData.managers.myTextureManager;
}

Engine::MaterialManager* EngineInterface::GetMaterialManager()
{
	return &Engine::globalEngineData.managers.myMaterialManager;
}

Engine::ModelManager* EngineInterface::GetModelManager()
{
	return &Engine::globalEngineData.managers.myModelManager;
}

Engine::DirectXFramework* EngineInterface::GetDXFramework()
{
	return &Engine::globalEngineData.DXFramework;
}

Engine::SpriteRenderer* EngineInterface::GetSpriteRenderer()
{
	return Engine::globalEngineData.renderer.GetSpriteRenderer();
}

Engine::WindowHandler* EngineInterface::GetWindowHandler()
{
	return &Engine::globalEngineData.windowHandler;
}

CU::ThreadPool* EngineInterface::GetThreadPool()
{
	return &Engine::globalEngineData.threadPool;
}

Engine::Renderer* EngineInterface::GetRenderer()
{
	return &Engine::globalEngineData.renderer;
}


void EngineInterface::DrawLine(const v3f aFrom, const v3f aTo, CU::Color aColor, float aSize)
{
#ifndef _DISTRIBUTION
	Engine::RenderData* buf = Engine::GetWriteGameplayBuffer();
	Vertex_Debug_Line command;
	command.myPosFrom.x = aFrom.x;
	command.myPosFrom.y = aFrom.y;
	command.myPosFrom.z = aFrom.z;
	command.myPosFrom.w = 1;

	command.myPosTo.x = aTo.x;
	command.myPosTo.y = aTo.y;
	command.myPosTo.z = aTo.z;
	command.myPosTo.w = 1;
	command.myColor = aColor;
	command.mySize = aSize;
	buf->debugLines[buf->debugLinesSize++] = command;
#endif

}

void EngineInterface::DrawBox(v3f aPosition, const v3f aBoxSize, float aLineSize, CU::Color aColor)
{
#ifndef _DISTRIBUTION
	DrawLine({ aPosition.x + -aBoxSize.x * 0.5f, aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, { aPosition.x + +aBoxSize.x * 0.5f,aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + -aBoxSize.x * 0.5f, aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, { aPosition.x + -aBoxSize.x * 0.5f,aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + -aBoxSize.x * 0.5f, aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, { aPosition.x + -aBoxSize.x * 0.5f,aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + aBoxSize.x * 0.5f, aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, { aPosition.x + +aBoxSize.x * 0.5f,aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + aBoxSize.x * 0.5f, aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, { aPosition.x + +aBoxSize.x * 0.5f,aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + aBoxSize.x * 0.5f, aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, { aPosition.x + -aBoxSize.x * 0.5f,aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + aBoxSize.x * 0.5f, aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, { aPosition.x + +aBoxSize.x * 0.5f,aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + aBoxSize.x * 0.5f, aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, { aPosition.x + +aBoxSize.x * 0.5f,aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + aBoxSize.x * 0.5f, aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, { aPosition.x + -aBoxSize.x * 0.5f,aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + aBoxSize.x * 0.5f, aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, { aPosition.x + -aBoxSize.x * 0.5f,aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + -aBoxSize.x * 0.5f, aPosition.y + -aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, { aPosition.x + -aBoxSize.x * 0.5f,aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, aColor, aLineSize);
	DrawLine({ aPosition.x + -aBoxSize.x * 0.5f, aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + +aBoxSize.z * 0.5f }, { aPosition.x + -aBoxSize.x * 0.5f,aPosition.y + +aBoxSize.y * 0.5f, aPosition.z + -aBoxSize.z * 0.5f }, aColor, aLineSize);
#endif
}

void EngineInterface::DrawBox(CU::AABB3Df aCollider, float aLineSize, CU::Color aColor)
{
#ifndef _DISTRIBUTION
	//   Y TopBot | Z FrontBack | X LeftRight
	v3f tfr, tfl, tbr, tbl, bfr, bfl, bbl, bbr;
	tfr = aCollider.myMax;
	bbl = aCollider.myMin;
	tfl = tfr; tfl.x = bbl.x;
	tbr = tfl; tbr.z = bbl.z;
	tbl = bbl; tbl.y = tfr.y;

	bfr = tfr; bfr.y = bbl.y;
	bfl = bbl; bfl.z = tfr.z;
	bbr = bbl; bbr.x = tfr.x;

	DrawLine(tfr, tfl, aColor, aLineSize);
	DrawLine(tfr, bfr, aColor, aLineSize);
	DrawLine(tfr, tbr, aColor, aLineSize);
	DrawLine(tbr, bbr, aColor, aLineSize);
	DrawLine(tbr, tbl, aColor, aLineSize);
	DrawLine(bbr, bfr, aColor, aLineSize);
	DrawLine(bbr, bbl, aColor, aLineSize);
	DrawLine(bbl, tbl, aColor, aLineSize);
	DrawLine(bbl, bfl, aColor, aLineSize);
	DrawLine(bfl, bfr, aColor, aLineSize);
	DrawLine(bfl, tfl, aColor, aLineSize);
	DrawLine(tfl, tbl, aColor, aLineSize);
#endif
}

void EngineInterface::DrawBoxRotated(v3f aPosition, const v3f aBoxSize, const CU::Matrix3x3f aRotationMatrix, float aLineSize, CU::Color aColor)
{
#ifndef _DISTRIBUTION
	v3f corner1 = v3f(-aBoxSize.x * 0.5f, -aBoxSize.y * 0.5f, -aBoxSize.z * 0.5f) * aRotationMatrix;
	v3f corner2 = v3f(+aBoxSize.x * 0.5f, -aBoxSize.y * 0.5f, -aBoxSize.z * 0.5f) * aRotationMatrix;
	v3f corner3 = v3f(-aBoxSize.x * 0.5f, +aBoxSize.y * 0.5f, -aBoxSize.z * 0.5f) * aRotationMatrix;
	v3f corner4 = v3f(-aBoxSize.x * 0.5f, -aBoxSize.y * 0.5f, +aBoxSize.z * 0.5f) * aRotationMatrix;
	v3f corner5 = v3f(+aBoxSize.x * 0.5f, -aBoxSize.y * 0.5f, +aBoxSize.z * 0.5f) * aRotationMatrix;
	v3f corner6 = v3f(+aBoxSize.x * 0.5f, +aBoxSize.y * 0.5f, -aBoxSize.z * 0.5f) * aRotationMatrix;
	v3f corner7 = v3f(+aBoxSize.x * 0.5f, +aBoxSize.y * 0.5f, +aBoxSize.z * 0.5f) * aRotationMatrix;
	v3f corner8 = v3f(-aBoxSize.x * 0.5f, +aBoxSize.y * 0.5f, +aBoxSize.z * 0.5f) * aRotationMatrix;

	DrawLine(corner1 + aPosition, corner2 + aPosition, aColor, aLineSize);
	DrawLine(corner1 + aPosition, corner3 + aPosition, aColor, aLineSize);
	DrawLine(corner1 + aPosition, corner4 + aPosition, aColor, aLineSize);
	DrawLine(corner2 + aPosition, corner5 + aPosition, aColor, aLineSize);
	DrawLine(corner2 + aPosition, corner6 + aPosition, aColor, aLineSize);
	DrawLine(corner6 + aPosition, corner3 + aPosition, aColor, aLineSize);
	DrawLine(corner6 + aPosition, corner7 + aPosition, aColor, aLineSize);
	DrawLine(corner7 + aPosition, corner5 + aPosition, aColor, aLineSize);
	DrawLine(corner7 + aPosition, corner8 + aPosition, aColor, aLineSize);
	DrawLine(corner5 + aPosition, corner4 + aPosition, aColor, aLineSize);
	DrawLine(corner4 + aPosition, corner8 + aPosition, aColor, aLineSize);
	DrawLine(corner8 + aPosition, corner3 + aPosition, aColor, aLineSize);
#endif 
}

void EngineInterface::DrawBoxAxisColored(const v3f aMin, const v3f aMax)
{
#ifndef _DISTRIBUTION
	const v3f size = (aMax - aMin);
	v3f boundPoints[8];
	boundPoints[0] = aMax; //tFR
	boundPoints[1] = { aMax.x - size.x, aMax.y, aMax.z }; //tFL
	boundPoints[2] = { aMax.x, aMax.y, aMax.z - size.z }; //tBR
	boundPoints[3] = { aMax.x - size.x, aMax.y, aMax.z - size.z }; // tBL
	boundPoints[4] = aMin; //bBL
	boundPoints[5] = { aMin.x + size.x, aMin.y, aMin.z };//bBR
	boundPoints[6] = { aMin.x, aMin.y, aMin.z + size.z };//bFL
	boundPoints[7] = { aMin.x + size.x, aMin.y, aMin.z + size.z };//bFR
	CU::Color red = { 255, 0, 0, 255 };
	CU::Color green = { 0, 255, 0, 255 };
	CU::Color blue = { 0, 0, 255, 255 };
	float lineSize = 2.0f;
	DrawLine(boundPoints[0], boundPoints[1], blue, lineSize);
	DrawLine(boundPoints[0], boundPoints[2], red, lineSize);
	DrawLine(boundPoints[0], boundPoints[7], green, lineSize);
	DrawLine(boundPoints[7], boundPoints[6], blue, lineSize);
	DrawLine(boundPoints[7], boundPoints[5], red, lineSize);
	DrawLine(boundPoints[5], boundPoints[4], blue, lineSize);
	DrawLine(boundPoints[5], boundPoints[2], green, lineSize);
	DrawLine(boundPoints[2], boundPoints[3], blue, lineSize);
	DrawLine(boundPoints[3], boundPoints[1], red, lineSize);
	DrawLine(boundPoints[3], boundPoints[4], green, lineSize);
	DrawLine(boundPoints[4], boundPoints[6], red, lineSize);
	DrawLine(boundPoints[6], boundPoints[1], green, lineSize);
#endif
}

void EngineInterface::DrawSphere(const v3f aPos, const float aRadius, CU::Color aColor)
{
#ifndef _DISTRIBUTION
	Engine::DebugSphereCommand command;
	command.myPosition = aPos;
	command.myRadius = aRadius;
	command.mySphereColor = aColor;
	Engine::GetWriteGameplayBuffer()->debugSpheres[Engine::GetWriteGameplayBuffer()->debugSpheresSize++] = command;
#endif // DEBUG
}

//Radius is a multiplier but also not at the same time, its wierd ask Pu or Victor :)
void EngineInterface::DrawCapsule(const v3f aPos, const float aRadiusThatsNotARadius, const float aHeight, const CU::Matrix3x3f aRotationMatrix, CU::Color aColor)
{
#ifndef _DISTRIBUTION
	float convertedRadius = aRadiusThatsNotARadius * 0.01f;
	v3f pos;

	pos.y += (aHeight * 0.5f) - aRadiusThatsNotARadius;
	pos = pos * aRotationMatrix;
	pos += aPos;
	DrawSphere(pos, convertedRadius * 2.0f, aColor);
	pos = v3f();
	pos.y -= (aHeight * 0.5f) - aRadiusThatsNotARadius;
	pos = pos * aRotationMatrix;
	pos += aPos;
	DrawSphere(pos, convertedRadius * 2.0f, aColor);

	float bottomRadius, topRadius, height;
	UINT sliceCount, stackCount;
	bottomRadius = aRadiusThatsNotARadius;
	topRadius = bottomRadius;
	height = aHeight - aRadiusThatsNotARadius * 2.0f;
	sliceCount = 8;
	stackCount = 1;
	float stackHeight = height / (float)stackCount;
	float radiusStep = (topRadius - bottomRadius) / stackCount;
	UINT ringCount = stackCount + 1;

	//Vertices
	CU::GrowingArray<v3f> vertices;
	vertices.Init(40);
	for (UINT i = 0; i < ringCount + 1; i++)
	{
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		float dTheta = 2.0f * 3.1415f / float(sliceCount);
		for (UINT j = 0; j <= sliceCount; j++)
		{
			v3f vertex;

			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex = { r * c, y, r * s };

			vertices.Add(vertex);
		}
	}

	for (unsigned short i = 0; i < sliceCount; i++)
	{
		DrawLine((vertices[i] * aRotationMatrix) + aPos, (vertices[(unsigned short)(sliceCount + i + 1)] * aRotationMatrix) + aPos, aColor);
	}

#endif // DEBUG
}

void EngineInterface::DrawCylinder(const v3f aPos, const float aRadius, const float aHeight, const CU::Matrix3x3f aRotationMatrix, CU::Color aColor)
{
#ifndef _DISTRIBUTION
	float bottomRadius, topRadius, height;
	UINT sliceCount, stackCount;
	bottomRadius = aRadius;
	topRadius = bottomRadius;
	height = aHeight;
	sliceCount = 8;
	stackCount = 1;
	float stackHeight = height / (float)stackCount;
	float radiusStep = (topRadius - bottomRadius) / stackCount;
	UINT ringCount = stackCount + 1;

	//Vertices
	CU::GrowingArray<v3f> vertices;
	vertices.Init(40);
	for (UINT i = 0; i < ringCount + 1; i++)
	{
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		float dTheta = 2.0f * 3.1415f / float(sliceCount);
		for (UINT j = 0; j <= sliceCount; j++)
		{
			v3f vertex;

			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex = { r * c, y, r * s };

			vertices.Add(vertex);
		}
	}

	//Orkar inte tänka ut hur man ska göra fina trianglar där uppe :)
	for (unsigned short i = 0; i < sliceCount; i++)
	{
		DrawLine((vertices[i] * aRotationMatrix) + aPos, (vertices[(unsigned short)(sliceCount + i + 1)] * aRotationMatrix) + aPos, aColor);
		DrawLine((vertices[i] * aRotationMatrix) + aPos, (vertices[i + 1] * aRotationMatrix) + aPos, aColor);
		DrawLine((vertices[(unsigned short)(sliceCount + i + 1)] * aRotationMatrix) + aPos, (vertices[(unsigned short)(sliceCount + i + 2)] * aRotationMatrix) + aPos, aColor);
	}
	for (unsigned short i = 0; i < sliceCount; i += 2)
	{
		DrawLine((vertices[i] * aRotationMatrix) + aPos, (vertices[i + 2] * aRotationMatrix) + aPos, aColor);
		DrawLine((vertices[(unsigned short)(sliceCount + i + 1)] * aRotationMatrix) + aPos, (vertices[(unsigned short)(sliceCount + i + 3)] * aRotationMatrix) + aPos, aColor);
	}
#endif // DEBUG
}

void EngineInterface::DrawPlane(const v3f aPos, const v2f aSize, const CU::Matrix3x3f aRotationMatrix, CU::Color aColor)
{
	v3f corner0 = v3f(aSize.x, 0.0f, aSize.y ) * aRotationMatrix;
	v3f corner1 = v3f(aSize.x, 0.0f, aSize.y * -1.f) * aRotationMatrix;
	v3f corner2 = v3f(aSize.x * -1.f, 0.0f, aSize.y) * aRotationMatrix;
	v3f corner3 = v3f(aSize.x * -1.f, 0.0f, aSize.y * -1.f) * aRotationMatrix;

	DrawLine(corner0 + aPos, corner1 + aPos, aColor);
	DrawLine(corner1 + aPos, corner3 + aPos, aColor);
	DrawLine(corner2 + aPos, corner3 + aPos, aColor);
	DrawLine(corner0 + aPos, corner2 + aPos, aColor);
}

void EngineInterface::DrawCameraFrustum(FrustumDrawData& someCamData)
{	
	DrawLine(someCamData.fBL, someCamData.fBR, {255, 180, 0, 255}, 4.0f);
	DrawLine(someCamData.fBL, someCamData.fTL, {255, 180, 0, 255}, 4.0f);
	DrawLine(someCamData.fBL, someCamData.nBL, {255, 180, 0, 255}, 4.0f);
	DrawLine(someCamData.fBR, someCamData.fTR, {255, 180, 0, 255}, 4.0f);
	DrawLine(someCamData.fBR, someCamData.nBR, {255, 180, 0, 255}, 4.0f);
	DrawLine(someCamData.fTR, someCamData.fTL, {255, 180, 0, 255}, 4.0f);
	DrawLine(someCamData.fTR, someCamData.nTR, {255, 180, 0, 255}, 4.0f);
	DrawLine(someCamData.fTL, someCamData.nTL, {255, 180, 0, 255}, 4.0f);
}

void EngineInterface::ToggleCursorVisibility()
{
#ifndef _DISTRIBUTION
	Engine::globalEngineData.showCursor = !Engine::globalEngineData.showCursor;
#endif
}

void EngineInterface::DrawSpotLight(const v3f aPos, const v3f aDirection, float aAngle, float aRange, float aLineSize, CU::Color aColor)
{
#ifndef _DISTRIBUTION
	m4f lookAt;
	lookAt.LookAt(aDirection, { 0, 0, 0 });
	//CU::Transform trans;
	//trans.LookAt(aDirection, { 0, 0, 0 });
	const m3f rotationMatrix = lookAt;

	const float radius = aRange * tanf(aAngle * 0.5f);

	unsigned int amountOfLines = 8;
	float theta = 2.0f * CU::pif * (1.0f / (float)amountOfLines);
	for (unsigned int i = 0; i < amountOfLines; i++)
	{
		float x = radius * cosf(i * theta);
		float y = radius * sinf(i * theta);
		DrawLine(aPos, aPos + v3f( x, y, aRange) * rotationMatrix, aColor, aLineSize);
	}
#endif // DEBUG
}

CU::Timer* EngineInterface::GetTimer()
{
	return &Engine::globalEngineData.timer;
}
ToolsMenu& EngineInterface::GetToolsMenu()
{
	return Engine::globalEngineData.toolsMenu;
}
const bool EngineInterface::GetIsFullScreen()
{
	return Engine::globalEngineData.windowHandler.GetIsFullScreen();
}
const bool EngineInterface::GetIsBordered()
{
	return Engine::globalEngineData.windowHandler.GetIsBordered();
}
bool EngineInterface::StartEngine(Engine::StartupAttributes& someWindowData)
{
	return Engine::Core::Start(someWindowData);
}
void EngineInterface::SetWindowSettings(const Engine::WindowsMessage& aWindowMessage)
{
	Engine::Core::QueueWindowSettings(aWindowMessage);
}

const v2ui EngineInterface::GetRenderResolution()
{
	return Engine::globalEngineData.renderTargetResolution;
}

const v2ui EngineInterface::GetWindowResolution()
{
	return Engine::globalEngineData.windowHandler.GetWindowResolution();
}

const v2ui EngineInterface::GetWindowPosition()
{
	return Engine::globalEngineData.windowHandler.GetWindowPosition();
}

const v2ui EngineInterface::GetMonitorResolution()
{
	return Engine::globalEngineData.windowHandler.GetMonitorResolution();
}