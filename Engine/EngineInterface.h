#pragma once
#include "..\CommonUtilities\CU\Math\Vector.h"
#include "..\CommonUtilities\CU\Math\Matrix3x3.hpp"
#include "..\CommonUtilities\CU\Collision\AABB3D.hpp"
#include "Core/StartAttributes.h"
namespace CU
{
	class Timer;
	class ThreadPool;
}
class ToolsMenu;
struct FrustumDrawData;
namespace Engine
{
	struct EngineManagers;
	struct WindowsMessage;
	class MaterialManager;
	class TextureManager;
	class ModelManager;
	class DirectXFramework;
	class SpriteRenderer;
	class Renderer;
	class WindowHandler;
}
namespace Utility
{
	class DragNDropHandler;
}
namespace EngineInterface
{
	Engine::EngineManagers* GetManagers();
	Engine::MaterialManager* GetMaterialManager();
	Engine::TextureManager* GetTextureManager();
	Engine::ModelManager* GetModelManager();
	Engine::DirectXFramework* GetDXFramework();
	Engine::SpriteRenderer* GetSpriteRenderer();
	Engine::WindowHandler* GetWindowHandler();
	Engine::Renderer* GetRenderer();
	CU::ThreadPool* GetThreadPool();
	CU::Timer* GetTimer();
	const v2ui GetRenderResolution();
	const v2ui GetWindowResolution();
	const v2ui GetWindowPosition();
	const v2ui GetMonitorResolution();
	ToolsMenu& GetToolsMenu();
	const bool GetIsFullScreen();
	const bool GetIsBordered();
	
	void DrawLine(const v3f aFrom, const v3f aTo, CU::Color aColor = { 255, 255, 255, 255 }, float aSize = 1);
	void DrawBox(v3f aPosition, const v3f aBoxSize, float aLineSize = 1, CU::Color aColor = { 255, 255, 255, 255 });
	void DrawBox(CU::AABB3Df aCollider, float aLineSize = 1, CU::Color aColor = { 255, 255, 255, 255 });
	void DrawBoxRotated(v3f aPosition, const v3f aBoxSize,const CU::Matrix3x3f aRotationMatrix, float aLineSize = 1, CU::Color aColor = { 255, 255, 255, 255 });
	void DrawBoxAxisColored(const v3f aMin, const v3f aMax);
	void DrawSphere(const v3f aPos, const float aRadius, CU::Color aColor = { 255, 255, 255, 255 });
	void DrawCapsule(const v3f aPos, const float aRadius, const float aHeight, const CU::Matrix3x3f aRotationMatrix, CU::Color aColor = { 255, 255, 255, 255 });
	void DrawCylinder(const v3f aPos, const float aRadius, const float aHeight, const CU::Matrix3x3f aRotationMatrix, CU::Color aColor = { 255, 255, 255, 255 });
	void DrawPlane(const v3f aPos, const v2f aSize,const CU::Matrix3x3f aRotationMatrix, CU::Color aColor = { 255, 255, 255, 255 });
	void DrawCameraFrustum(FrustumDrawData& someCamData);
	void ToggleCursorVisibility();

	void DrawSpotLight(const v3f aPos, const v3f aDirection, float aAngle, float aRange, float aLineSize = 1.0f, CU::Color aColor = { 255, 255, 255, 255 });
	void SetWindowSettings(const Engine::WindowsMessage& aWindowMessage);
	bool StartEngine(Engine::StartupAttributes& someWindowData);
	void ShutDownEngine();
	Utility::DragNDropHandler* GetDragNDropHandler();
	v2ui GetEnumResolution(uint16_t aResolution);
	uint16_t GetERenderResolution();
	uint16_t GetEWindowResolution();
}


