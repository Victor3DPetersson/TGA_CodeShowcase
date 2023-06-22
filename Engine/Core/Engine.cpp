#include "stdafx.h"
#include "framework.h"
#include "Engine.h"

#include "CU/Math/Color.hpp"
#include "Includes.h"

#include "Managers\AudioManager.h"

#include <UI/Font.h>


#include "ToolsMenu/ToolsMenuFlag.h"
#include "UI/CursorStatus.h"
#include "Input/XInputWrapper.h"

#include "RenderData.h"
#include "../Engine/Network/Network.h"
#include "../Engine/Network/Client/Client.h"
#include "EngineInterface.h"

#include "Loa/EngineLog.hpp"
Engine::Log Engine::log;

std::atomic_int cursorStatus = 0;
bool lilja = true; // :')

namespace Engine
{
	EngineData globalEngineData;

	bool Core::Start(StartupAttributes& someWindowData)
	{
		/*
		char nameBuffer[SHRT_MAX] = "";
		GetModuleFileNameA(0, nameBuffer, SHRT_MAX);


		char ruleBuffer[SHRT_MAX] = "netsh advfirewall firewall add rule name=\"ToysRSus\" dir=in action=allow program=\"";
		strcat_s(ruleBuffer, nameBuffer);
		strcat_s(ruleBuffer, "\" enable=yes");

		system(ruleBuffer);
		*/

		MSG windowMsg;
		globalRenderData.renderBuffersData[0].name = "Buffer 0";
		globalRenderData.renderBuffersData[1].name = "Buffer 1";
		globalRenderData.renderBuffersData[2].name = "Buffer 2";
		if (Private::InternalInit(someWindowData))
		{
			globalEngineData.runApplication = true;
			globalEngineData.gamePlayDone = false;
			globalEngineData.hasStartedFrame = false;
			globalEngineData.gameHasUpdated = true;

			Font::LoadFonts();

			globalEngineData.isPlaying = true;

			globalEngineData.gameThread = std::thread(&Private::GameplayThread);
			auto handle = globalEngineData.gameThread.native_handle();
			HRESULT result = SetThreadDescription(handle, L"FuckyWacky");
			if (!SUCCEEDED(result))
			{
				assert(false && "Axel Sucks CYKA BLYAT");
			}
			globalEngineData.toolsMenu.Init();
			if (someWindowData.startupFlags & StartupFlags_SuppressToolsMenu)
			{
				globalEngineData.toolsMenu.Kill();
			}
#ifndef _DISTRIBUTION
			globalEngineData.showCursor = !(someWindowData.startupFlags & StartupFlags_HideCursor);
#endif
			if (someWindowData.myAcceptDragNDrop)
			{
				globalEngineData.dragNDropHandler.Subscribe(someWindowData.myDragNDropHandleFunction);
			}
			WindowsMessage wMessage;
			if (someWindowData.myStartInFullScreen)
			{
				UINT message = EFullscreen;
				wMessage.myMessage = message;
			}
			wMessage.myRenderResolution = (uint16_t)someWindowData.myWindowResolution;
			wMessage.myWindowResolution = (uint16_t)someWindowData.myWindowResolution;
			globalEngineData.userSetRes = (uint16_t)someWindowData.myWindowResolution;
			globalEngineData.userSetWindowRes = (uint16_t)someWindowData.myWindowResolution;
			Core::SetWindowSettings(wMessage);

			// Main message loop:
			while (globalEngineData.runApplication)
			{
				while (PeekMessage(&windowMsg, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&windowMsg);
					DispatchMessage(&windowMsg);
					if (!globalEngineData.applicationPaused)
					{
						globalEngineData.managers.myInputManager.ReadInputEvents(globalEngineData.windowHandler.GetWindowHandle(), windowMsg.message, windowMsg.wParam, windowMsg.lParam);
					}
				}
				Core::Private::RenderProcess();
			}
			globalEngineData.gameThread.join();
			Core::Private::InternalShutDown();
			return true;
		}
		return false;
	}

	void Core::Private::RenderProcess()
	{
#ifdef _RELEASE
		globalEngineData.windowHandler.HandleMouseCursorVisibility(globalEngineData.showCursor);
#elif _DEBUG
		globalEngineData.windowHandler.HandleMouseCursorVisibility(true);
#else
		globalEngineData.windowHandler.HandleMouseCursorVisibility(false);
#endif
		globalEngineData.hasResized = false;
		{
			if (!globalEngineData.queuedCmdsLock.test_and_set())
			{
				while (globalEngineData.queuedCmds.Size())
				{
					auto cmd = globalEngineData.queuedCmds.Dequeue();
					Core::SetWindowSettings(cmd);
				}
				globalEngineData.queuedCmdsLock.clear();
			}
		}
		if (globalEngineData.isResizing)
		{
			OnResize();
		}
		//Pre frame stuff
		globalEngineData.timer.Update();
		const float dt = (float)globalEngineData.timer.GetDeltaTime();
		if (!globalEngineData.applicationPaused)
		{
			globalEngineData.imguiInterFace.PreFrame();
			Core::Private::BeginFrame();
			globalEngineData.drawCallCounter = 0;

			//Main Frame
			if (globalRenderData.frameTimer < 0 + dt)
			{
				if (globalEngineData.gameHasUpdated)
				{
					Core::Private::SwapBuffers();
					globalEngineData.managers.myInputManager.SwapBuffers();
					globalEngineData.managers.myModelManager.Update();
					globalEngineData.managers.myMaterialManager.Update();
					globalRenderData.frameTimer.store(INV_STEPF, std::memory_order_relaxed);
					globalEngineData.hasStartedFrame.store(true);
				}
			}

			Cursor(dt);
			Core::Private::RenderSyncedFrame();
			if (globalEngineData.renderFunctionToCall)
			{
				globalEngineData.renderFunctionToCall((float)globalEngineData.timer.GetDeltaTime());
			}
			globalEngineData.toolsMenu.Update((float)globalEngineData.timer.GetDeltaTime(), globalEngineData.drawCallCounter);
			globalEngineData.imguiInterFace.Render();

			//End of Frame
			Core::Private::EndFrame();
			globalRenderData.frameTimer.store(globalRenderData.frameTimer.load() - dt);
		}

		// TODO
		/*myManagers->myTextureManager.Flush();*/
	}

	void Core::Private::GameplayThread()
	{
		CU::Timer dtTimer;
		//Network::Start();
		//Client::Start();
		if (globalEngineData.initFunctionToCall != nullptr)
		{
			globalEngineData.initFunctionToCall();
		}

		while (globalEngineData.isPlaying)
		{
			Audio::Update((float)dtTimer.GetDeltaTime());
			if (globalEngineData.hasStartedFrame)
			{
				auto start = std::chrono::high_resolution_clock::now();
				globalEngineData.gameHasUpdated.store(false);
				dtTimer.Update();
				XInputWrapper::Refresh();
				GameplayThreadFrameStart();
				globalEngineData.updateFunctionToCall((float)dtTimer.GetDeltaTime());
				UpdateRenderTargetCameras();
				globalEngineData.gameHasUpdated.store(true);
				globalEngineData.hasStartedFrame.store(false);
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<float, std::ratio<1, 1>> dur = end - start;
				//SetFPSForProfiler(dur.count());
				SetFPSForProfiler((float)dtTimer.GetDeltaTime(), dur.count());
			}
		}
	}

	void Core::Private::GameplayThreadFrameStart()
	{
		RenderData* buf = GetWriteGameplayBuffer();
		buf->sortedMeshesSize = globalEngineData.managers.myModelManager.GetAmountOfModels();
		buf->sortedAnimMeshesSize = globalEngineData.managers.myModelManager.GetAmountOfAnimModels();

		globalRenderData.gameplayFlags |= GlobalRenderData::GameplayFlags_ModelBuffersAreAdjusted;

		for (size_t i = 0; i < buf->sortedMeshesSize; i++)
		{
			buf->sortedMeshes[i].numberOfModels = 0;
		}
		for (size_t i = 0; i < buf->sortedAnimMeshesSize; i++)
		{
			buf->sortedAnimMeshes[i].numberOfModels = 0;
		}

		buf->uniqueMeshesSize = 0;
		buf->uniqueAnimatedMeshesSize = 0;
		buf->pointLightsSize = 0;
		buf->spotLightsSize = 0;
		buf->particlesSize = 0;
		buf->spritesSize = 0;
		buf->wspritesSize = 0;
		buf->decalsSize = 0;
		buf->renderTargetcameraFlags.ResetAll();

#ifndef _DISTRIBUTION
		buf->debugLinesSize = 0;
		buf->debugSpheresSize = 0;
#endif // DEBUG
	}

	void Core::Private::RenderSyncedFrame()
	{
		globalEngineData.renderer.Render(globalEngineData.drawCallCounter, globalEngineData.ignoreShadowRendering);
	}

	void Core::Private::SwapBuffers()
	{
		SwapAndClearGameplayBuffers();
		globalRenderData.currentRenderStep.store(globalRenderData.step.load());
		globalEngineData.renderer.SetFrameBuffersAreSwapped(true);
	}

	void Core::ShutDownEngine()
	{
		globalEngineData.isPlaying = false;
		//Client::CleanUp();
		//Network::Stop();
		globalEngineData.runApplication = false;
	}

	bool Core::Private::InternalInit(StartupAttributes& someWindowData)
	{
		if (!globalEngineData.windowHandler.Init(someWindowData, &globalEngineData.managers.myInputManager))
		{
			return false;
		}
		if (someWindowData.startupFlags & StartupFlags_MaximizeWindow)
		{
			ShowWindow(globalEngineData.windowHandler.GetWindowHandle(), SW_MAXIMIZE);
		}
		if (!globalEngineData.DXFramework.Init(globalEngineData.windowHandler))
		{
			return false;
		}
		v2ui resolution;
		switch (someWindowData.myWindowResolution)
		{
		case E2160p:
			resolution = v2ui(3840, 2160);
			break;
		case E1440p:
			resolution = v2ui(2560, 1440);
			break;
		case E1080p:
			resolution = v2ui(1920, 1080);
			break;
		case E720p:
			resolution = v2ui(1280, 720);
			break;
		case E480p:
			resolution = v2ui(854, 480);
			break;
		case E360p:
			resolution = v2ui(640, 360);
			break;
		default:
			break;
		}
		globalEngineData.userSetRes = (uint16_t)someWindowData.myWindowResolution;
		globalEngineData.userSetWindowRes = (uint16_t)someWindowData.myWindowResolution;
		globalEngineData.renderTargetResolution = resolution;
		if (!Core::Private::InitManagers())
		{
			return false;
		}
		if (!globalEngineData.renderer.Init(&globalEngineData.DXFramework, &globalEngineData.timer, someWindowData.myBackgroundColor, &globalEngineData.managers, globalEngineData.renderTargetResolution))
		{
			return false;
		}
		globalEngineData.managers.myInputManager.InitWindowhandle(globalEngineData.windowHandler.GetWindowHandle());
		globalEngineData.imguiInterFace.Init(&globalEngineData.DXFramework, &globalEngineData.windowHandler);
		globalEngineData.initFunctionToCall = someWindowData.myInitEditorFunctionToCall;
		if (globalEngineData.initFunctionToCall != nullptr)
		{
			globalEngineData.initFunctionToCall();
			globalEngineData.initFunctionToCall = nullptr;
		}
		else
		{
			globalEngineData.initFunctionToCall = someWindowData.myInitGameFunctionToCall;
		}
		globalEngineData.updateFunctionToCall = someWindowData.myUpdateFunctionToCall;
		globalEngineData.renderFunctionToCall = someWindowData.myRenderFunctionToCall;
		if (globalEngineData.renderFunctionToCall != nullptr)
		{
			globalEngineData.ignoreShadowRendering = true;
		}
		globalEngineData.shutdownFunctionToCall = someWindowData.myShutdownFunctionToCall;

		for (size_t i = 0; i < 3; i++)
		{
			globalRenderData.renderBuffersData[i].renderTargetcameraFlags.ResetAll();
			for (size_t camera = 0; camera < 8; camera++)
			{
				globalRenderData.renderBuffersData[i].renderTargetCameras[camera].camera.RecalculateProjectionMatrix(90, globalEngineData.renderTargetResolution, true, 50.0f, 5000.0f);
			}
		}



		return true;
	}

	void Core::Private::InternalShutDown()
	{
		if (globalEngineData.shutdownFunctionToCall)
		{
			globalEngineData.shutdownFunctionToCall();
		}
		Audio::Shutdown();
		globalEngineData.imguiInterFace.Destroy();
		globalEngineData.renderer.ShutDownRenderer();
		globalEngineData.managers.myPhysicsManager.ShutDown();
		globalEngineData.managers.myParticleManager.ReleaseAllResources();
		globalEngineData.managers.myMaterialManager.ReleaseMaterials();
		globalEngineData.managers.myModelManager.ReleaseAllResources();
		//globalEngineData.managers.myTextureManager.ReleaseAllResources();
		globalEngineData.DXFramework.ShutDownDX();
	}

	void Core::Private::Cursor(float dt)
	{
		constexpr float fps = 25.0f;
		constexpr float frames = 9.0f;
		constexpr float paddingFrames = 7.0f;
		constexpr float frameUVWidth = 1.0f / (frames + paddingFrames);

		if ((cursorStatus & 1) == 0) return;

		v2f mousePos;
		{
			POINT p;
			GetCursorPos(&p);

			RECT r;
			GetWindowRect(globalEngineData.windowHandler.GetWindowHandle(), &r);

			mousePos = { float(p.x - r.left), float(p.y - r.top) };

			const v2f avail = (v2f)globalEngineData.windowHandler.GetWindowResolution();
			const v2f viewportSizeX = { avail.x, avail.x / (16.0f / 9.0f) };
			const v2f viewportSizeY = { avail.y * (16.0f / 9.0f) , avail.y };
			v2f viewportSize = avail.x < avail.y* (16.0f / 9.0f) ? viewportSizeX : viewportSizeY;
			v2f border = { (avail.x - viewportSize.x) * .5f, (avail.y - viewportSize.y) * .5f };
			mousePos -= border;
			v2f screenSize = viewportSize * .5f;
			mousePos.x /= screenSize.x; mousePos.y /= screenSize.y;
			mousePos -= {1, 1};
			mousePos.y *= -1.0f;
		}

		static float timer = 0.0f;
		if (cursorStatus & 2)
		{
			timer = (1.0f / fps) * frames;
		}
		timer -= dt;

		v2f uvTL = { 0,0 };
		v2f uvBR = { frameUVWidth, 1 };
		if (timer > 0.0f)
		{
			uvTL.x = frameUVWidth + frameUVWidth * floorf(frames - timer * fps);
			uvBR.x = frameUVWidth + uvTL.x;
		}

		static SpriteID spr = globalEngineData.managers.myTextureManager.GetSpriteID("Content/UI/cursorAtlasP8.dds");
		Vertex_Sprite vtx;
		vtx.myPosition = mousePos;
		vtx.mySize = { 128.0f, 128.0f };
		vtx.myUVOffsetTopL = uvTL;
		vtx.myUVOffsetBotR = uvBR;
		vtx.myZIndex = 100000000;
		if (globalEngineData.windowHandler.GetIsBordered())
		{
			vtx.myPivotOffset = { .65f, .7f };
		}
		else
		{
			vtx.myPivotOffset = { .85f, .15f };
		}
		SpriteCommand cmd = {
			spr, vtx, 1337420
		};
		GetReadBuffer()->sprites[GetReadBuffer()->spritesSize++] = cmd;
		GetLastReadBuffer()->sprites[GetLastReadBuffer()->spritesSize++] = cmd;
	}

	bool Core::Private::InitManagers()
	{
		if (!globalEngineData.managers.myTextureManager.Init(globalEngineData.DXFramework.GetDevice(), globalEngineData.DXFramework.GetDeviceContext()))
		{
			return false;
		}
		globalEngineData.managers.myMaterialManager.Init(globalEngineData.DXFramework.GetDevice(), globalEngineData.DXFramework.GetDeviceContext(), &globalEngineData.managers.myTextureManager);
		globalEngineData.managers.myModelManager.Init(globalEngineData.DXFramework.GetDevice(), &globalEngineData.managers.myTextureManager, &globalEngineData.managers.myMaterialManager);

		globalEngineData.managers.myParticleManager.Init(globalEngineData.DXFramework.GetDevice(), globalEngineData.DXFramework.GetDeviceContext(), &globalEngineData.managers.myTextureManager, &globalEngineData.managers.myMaterialManager);
		Audio::Init();
		globalEngineData.managers.myPhysicsManager.Init();
		return true;
	}

	void Core::SetWindowSettings(WindowsMessage aWindowMessage)
	{
		if (aWindowMessage.myMessage & EIsTabbedOut)
		{
			globalEngineData.applicationPaused = true;
			Audio::PauseAllEvents();
			globalEngineData.timer.Pause();
			globalEngineData.isResizing = true;
		}
		if (aWindowMessage.myMessage & EIsTabbedIn)
		{
			globalEngineData.applicationPaused = false;
			Audio::StartAllPausedEvents();
			globalEngineData.timer.Resume();
			globalEngineData.isResizing = true;
		}
		if (aWindowMessage.myMessage & EMove)
		{
			globalEngineData.windowHandler.OnMove();
		}
		if (aWindowMessage.myMessage & EUpdateResolution_FromEvent)
		{
			globalEngineData.isResizing = true;
		}
		if (aWindowMessage.myMessage & EIsResizing)
		{
			globalEngineData.applicationPaused = true;
			Audio::PauseAllEvents();
		}
		if (aWindowMessage.myMessage & EIsDoneResizing)
		{
			globalEngineData.applicationPaused = false;
			Audio::StartAllPausedEvents();
			globalEngineData.isResizing = true;
		}
		if (aWindowMessage.myMessage & EFullscreen)
		{
			globalEngineData.windowHandler.OnFullScreen();
			globalEngineData.isResizing = true;
		}
		if (aWindowMessage.myMessage & EBordered)
		{
			globalEngineData.windowHandler.OnChangeBorderStyle();
			globalEngineData.isResizing = true;
		}
		if (aWindowMessage.myMessage & EAltTab)
		{
			globalEngineData.isResizing = true;
		}
		if (aWindowMessage.myMessage & EChangeResolution)
		{
			globalEngineData.userSetRes = aWindowMessage.myRenderResolution;
			if (aWindowMessage.myWindowResolution == uint16_t(-2))
			{
				globalEngineData.windowHandler.ChangeResolution(globalEngineData.windowHandler.GetMonitorResolution());
			}
			else if (aWindowMessage.myWindowResolution != uint16_t(-1))
			{
				globalEngineData.userSetWindowRes = aWindowMessage.myWindowResolution;
				globalEngineData.windowHandler.ChangeResolution(EngineInterface::GetEnumResolution(aWindowMessage.myWindowResolution));
			}
			globalEngineData.isResizing = true;
		}
		globalEngineData.currentWindowSettings = aWindowMessage.myMessage;
	}

	void Core::QueueWindowSettings(WindowsMessage aWindowMessage)
	{
		while (globalEngineData.queuedCmdsLock.test_and_set());
		if (globalEngineData.queuedCmds.Size() < 32)
		{
			globalEngineData.queuedCmds.Enqueue(aWindowMessage);
		}
		globalEngineData.queuedCmdsLock.clear();
	}

	void Core::Private::OnResize()
	{
		globalEngineData.renderer.PrepareForResize();
		v2ui windowRes;
		if (globalEngineData.windowHandler.GetTabState() == EIsTabbedIn)
		{
			windowRes = globalEngineData.windowHandler.GetWindowResolution();
		}
		else
		{
			windowRes = { 0, 0 };
		}
		globalEngineData.DXFramework.OnResize(windowRes);
		globalEngineData.renderTargetResolution = globalEngineData.renderer.OnResize(EngineInterface::GetEnumResolution(globalEngineData.userSetRes), windowRes);
		globalEngineData.isResizing = false;
		globalEngineData.hasResized = true;
	}

	void Core::Private::BeginFrame()
	{
		CU::Color clearColor = CU::Color{ 41, 44, 48, 255 };
		globalEngineData.DXFramework.BeginFrame(clearColor);
	}



	void Core::Private::EndFrame()
	{
		globalEngineData.DXFramework.EndFrame();
	}
}