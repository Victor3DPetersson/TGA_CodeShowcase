#pragma once
#include <Windows.h>
#include "../CommonUtilities/CU/Math/Vector2.hpp"
#include "StartAttributes.h"
namespace CU
{
	class InputManager;
}
namespace Engine
{
	class WindowHandler
	{
	public:

		static LRESULT CALLBACK WinProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
		bool Init(StartupAttributes& someWindowData, CU::InputManager* aInputPtr);
		HWND GetWindowHandle() { return myWindowHandle; }
		v2ui GetWindowResolution();
		v2ui GetWindowPosition();
		v2ui GetMonitorResolution();
		void ChangeResolution(v2ui aNewResolution = { 0, 0 });
		void OnUpdateResolution();
		void OnChangeBorderStyle();
		void TabToggle(int);
		void OnFullScreen();
		void HandleMouseCursorVisibility(bool aCursorVisibility);
		
		void OnMove();
		const bool GetIsFullScreen() { return myIsFullScreen; }
		const bool GetIsBordered() { return myHasBorder; }
		const int GetTabState() { return myTabState; }
	private:
		v2ui GetWindowBorderSize();
		void InternalSetWindowPos();
		CU::InputManager* myInput = nullptr;;
		HWND myWindowHandle;
		bool myIsFullScreen = false;
		bool myHasBorder = true;
		v2ui myDefaultWindowPosition;
		v2ui myPreviousSize;
		v2ui myBorderSize;
		int myTabState = EIsTabbedIn;
	};
}