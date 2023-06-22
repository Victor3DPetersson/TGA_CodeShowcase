#pragma once
#include <thread>
#include <atomic>
#include "../Engine/resource.h"
#include "StartAttributes.h"
#include "../../CommonUtilities/CU/Containers/ThreadPool.h"
//#include "../../CommonUtilities/CU/Utility/Timer/Timer.h"
#include "../../CommonUtilities/CU\Input\InputManager.h"
#include "../../CommonUtilities/CU/Utility/Timer/Timer.h"

#include "../ToolsMenu/ToolsMenu.h"
#include "../CommonUtilities/CU/Containers/Queue.hpp"

#include "../Managers/Managers.h"
#include "DragnDropHandler.h"
#include "ImguiEngineInterface.h"
#include "WindowHandler.h"
#include "DirectXFramework.h"
#include "Rendering/Renderer.h"

namespace Engine
{
	struct EngineData
	{
		callback_function initFunctionToCall;
		callback_function_dt updateFunctionToCall;
		callback_function_dt renderFunctionToCall;
		callback_function shutdownFunctionToCall;
		CU::Timer timer;
		WindowHandler windowHandler;
		DirectXFramework DXFramework;
		Renderer renderer;
		ImguiEngineInterface imguiInterFace;
		EngineManagers managers;

		std::thread gameThread;
		std::thread timerThread;


		UINT currentWindowSettings = 0;
		v2ui renderTargetResolution;
		bool hasResized = false;
		bool applicationPaused = false;
		bool isResizing = false;
		bool ignoreShadowRendering = false;

		std::atomic<unsigned int> drawCallCounter;
		std::atomic<bool> runApplication;
		std::atomic<bool> gamePlayDone;
		std::atomic<bool> isPlaying;
		std::atomic<bool> hasStartedFrame;
		std::atomic<bool> gameHasUpdated;
		std::atomic<bool> gameTimerDone;
		//std::atomic<bool> myHasStarted;
		/* Tools */
		ToolsMenu toolsMenu;

		CU::Queue<WindowsMessage, 32> queuedCmds;
		std::atomic_flag queuedCmdsLock;
		Utility::DragNDropHandler dragNDropHandler;
		CU::ThreadPool threadPool;
		//used for the options menu
		uint16_t userSetRes;
		uint16_t userSetWindowRes;
		float volumeBeforeTab = 0.5f;
#ifndef _DISTRIBUTION
		bool showCursor = true;
#endif
	};
	extern EngineData globalEngineData;

	namespace Core
	{
		bool Start(StartupAttributes& someWindowData);
		void ShutDownEngine();
		void SetWindowSettings(WindowsMessage aWindowMessage);
		void QueueWindowSettings(WindowsMessage aWindowMessage);

		namespace Private
		{
			bool InitManagers();
			bool InternalInit(StartupAttributes& someWindowData);
			void InternalShutDown();
			void OnResize();
			void BeginFrame();
			void RenderSyncedFrame();
			void SwapBuffers();
			void EndFrame();
			void RenderProcess();
			void GameplayThread();
			void TimerThread();
			void GameplayThreadFrameStart();
			void Cursor(float dt);
		}
	}
}

