#include "stdafx.h"
#include "Core/WindowHandler.h"
#include "resource.h"
#include "CU/Input/InputManager.h"
#include "imgui/imgui.h"
#include "CU\Utility\Timer\Timer.h"
#include "EngineInterface.h"
#include <filesystem>
#include <shellapi.h>
#include "Engine.h"
#include "DragnDropHandler.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Engine::WindowHandler::WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	static WindowHandler* windowHandler = nullptr;
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
		return true;
	if (uMsg == WM_DESTROY || uMsg == WM_CLOSE)
	{
		EngineInterface::ShutDownEngine();
		return 0;
	}
	else if (uMsg == WM_CREATE)
	{
		CREATESTRUCT* createstruct = reinterpret_cast<CREATESTRUCT*>(lParam);
		windowHandler = reinterpret_cast<WindowHandler*>(createstruct->lpCreateParams);
	}
	else if (uMsg == WM_DROPFILES) {
		HDROP drop = (HDROP)wParam;
		UINT num_paths = DragQueryFileW(drop, 0xFFFFFFFF, 0, 512);

		wchar_t* filename = nullptr;
		UINT max_filename_len = 0;

		for (UINT i = 0; i < num_paths; ++i) {
			UINT filename_len = DragQueryFileW(drop, i, nullptr, 512) + 1;
			if (filename_len > max_filename_len) {
				max_filename_len = filename_len;
				wchar_t* tmp = (wchar_t*)realloc(filename, max_filename_len * sizeof(*filename));
				if (tmp != nullptr) {
					filename = tmp;
				}
			}
			DragQueryFileW(drop, i, filename, filename_len);
			EngineInterface::GetDragNDropHandler()->Callback(filename);
		}
		free(filename);
		DragFinish(drop);
	}

	WindowsMessage message;

	switch (uMsg)
	{
	case WM_ENTERSIZEMOVE:
		message.myMessage = EIsResizing;
		EngineInterface::SetWindowSettings(message);
		break;
	case WM_EXITSIZEMOVE:
		message.myMessage = EIsDoneResizing | EMove;
		EngineInterface::SetWindowSettings(message);
		break;
	
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 720;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 425;
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			message.myMessage = EIsTabbedOut;
			EngineInterface::SetWindowSettings(message);

			globalEngineData.windowHandler.TabToggle(EIsTabbedOut);
		}
		else
		{
			message.myMessage = EIsTabbedIn;
			EngineInterface::SetWindowSettings(message);

			globalEngineData.windowHandler.TabToggle(EIsTabbedIn);
		}
		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_MINIMIZE)
		{
			message.myMessage = EIsTabbedOut;
			EngineInterface::SetWindowSettings(message);
		}
		if (wParam == SC_MAXIMIZE)
		{
			message.myMessage = EUpdateResolution_FromEvent;
			EngineInterface::SetWindowSettings(message);
		}
		if (wParam == SC_RESTORE)
		{
			message.myMessage = EUpdateResolution_FromEvent;
			EngineInterface::SetWindowSettings(message);
		}
		break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
		if (wParam == VK_RETURN)
		{
			message.myMessage = EAltTab;
			EngineInterface::SetWindowSettings(message);
		}
		break;
	default:
		break;
	}	  

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool Engine::WindowHandler::Init(StartupAttributes& someWindowData, CU::InputManager* aInputPtr)
{
	myDefaultWindowPosition = { someWindowData.myStartPositionX, someWindowData.myStartPositionY };
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
	myPreviousSize = resolution;
	myInput = aInputPtr;
	WNDCLASS windowClass = {};
	windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = WindowHandler::WinProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = L"AMFEngine";
	windowClass.hIcon = ::LoadIcon(nullptr, MAKEINTRESOURCE(IDI_ICON));
	RegisterClass(&windowClass);

	DWORD windowStyle = 0;
	//windowStyle = WS_POPUP | WS_VISIBLE;
	windowStyle = WS_OVERLAPPEDWINDOW | WS_POPUP | WS_VISIBLE;
	myWindowHandle = CreateWindowW(L"AMFEngine",
		someWindowData.myApplicationName.c_str(), //title of the window
		windowStyle, // window style
		someWindowData.myStartPositionX, someWindowData.myStartPositionY, //Position of Window
		resolution.x, resolution.y, //Size Of Window
		nullptr, // Parent Window (NULL) 
		nullptr, // extra menues (NULL) 
		nullptr, // application handle (NULL) 
		this); //Use with multiple Windows (NULL) 

	//SetWindowLongPtr(myWindowHandle, GWLP_USERDATA, (LONG_PTR)this);
	
	RECT rcClient, rcWind;
	POINT ptDiff;
	GetClientRect(myWindowHandle, &rcClient);
	GetWindowRect(myWindowHandle, &rcWind);
	ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
	ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;
	myBorderSize.x = ptDiff.x;
	myBorderSize.y = ptDiff.y;

	SetWindowPos(myWindowHandle, HWND_NOTOPMOST, myDefaultWindowPosition.x, myDefaultWindowPosition.y, myPreviousSize.x, myPreviousSize.y + (int)myBorderSize.y - (int)myBorderSize.x, SWP_SHOWWINDOW);
	if (someWindowData.myAcceptDragNDrop)
	{
		DragAcceptFiles(myWindowHandle, TRUE);
	}

	return true;
}

v2ui Engine::WindowHandler::GetWindowResolution()
{
	RECT res = {};
	if (GetClientRect(myWindowHandle, &res))
	{
		return { (unsigned int)(res.right - res.left), (unsigned int)(res.bottom - res.top) };
	}
	
	return {0, 0};
}

v2ui Engine::WindowHandler::GetWindowPosition()
{
	RECT res = {};
	if (GetClientRect(myWindowHandle, &res))
	{
		return { uint32_t(res.left), uint32_t(res.top) };
	}

	return {};
}

v2ui Engine::WindowHandler::GetMonitorResolution()
{
	RECT res = {};
	const HWND hDesktop = GetDesktopWindow();
	if (GetWindowRect(hDesktop, &res))
	{
		return { (unsigned int)res.right, (unsigned int)res.bottom };
	}
	return { 0, 0 };
}


void Engine::WindowHandler::ChangeResolution(v2ui aNewResolution)
{
	if (myIsFullScreen == false)
	{
		myPreviousSize = aNewResolution;
		myBorderSize = GetWindowBorderSize();
		InternalSetWindowPos();
	}
}

void Engine::WindowHandler::OnUpdateResolution()
{
	ChangeResolution(myPreviousSize);
}

void Engine::WindowHandler::OnChangeBorderStyle()
{
	DWORD windowStyle = 0;
	myBorderSize = GetWindowBorderSize();
	myHasBorder = !myHasBorder;
	if (myHasBorder)
	{
		windowStyle = WS_OVERLAPPEDWINDOW | WS_POPUP | WS_VISIBLE;
		SetWindowLongPtr(myWindowHandle, GWL_STYLE, windowStyle);
		InternalSetWindowPos();
	}
	else
	{
		windowStyle = WS_POPUP | WS_VISIBLE;
		SetWindowLongPtr(myWindowHandle, GWL_STYLE, windowStyle);
		InternalSetWindowPos();
	}
}

void Engine::WindowHandler::TabToggle(int a)
{
	switch (a)
	{
	case EIsTabbedIn:
	{
		DWORD windowStyle = 0;
		if (myIsFullScreen || !myHasBorder)
		{
			myDefaultWindowPosition = { 0, 0 };
		}

		if (myHasBorder)
		{
			windowStyle = WS_OVERLAPPEDWINDOW | WS_POPUP | WS_VISIBLE;
			SetWindowLongPtr(myWindowHandle, GWL_STYLE, windowStyle);
			SetWindowPos(myWindowHandle, HWND_TOPMOST, myDefaultWindowPosition.x, myDefaultWindowPosition.y, myPreviousSize.x + (int)myBorderSize.x, myPreviousSize.y + (int)myBorderSize.y, SWP_SHOWWINDOW);
		}
		else
		{
			SetWindowPos(myWindowHandle, HWND_TOPMOST, myDefaultWindowPosition.x, myDefaultWindowPosition.y, myPreviousSize.x - (int)myBorderSize.x, myPreviousSize.y, SWP_SHOWWINDOW);
		}

		return;
	}
	case EIsTabbedOut:
	{
		DWORD windowStyle = 0;
		if (myIsFullScreen || !myHasBorder)
		{
			myDefaultWindowPosition = { 0, 0 };
		}

		if (myHasBorder)
		{
			windowStyle = WS_POPUP | WS_VISIBLE;
			SetWindowLongPtr(myWindowHandle, GWL_STYLE, windowStyle);
			SetWindowPos(myWindowHandle, HWND_NOTOPMOST, myDefaultWindowPosition.x, myDefaultWindowPosition.y, 0, 0, SWP_SHOWWINDOW);
		}
		else
		{
			SetWindowPos(myWindowHandle, HWND_NOTOPMOST, myDefaultWindowPosition.x, myDefaultWindowPosition.y, 0, 0, SWP_SHOWWINDOW);
		}
		return;
	}
	}
	myTabState = a;
}

void Engine::WindowHandler::OnFullScreen()
{
	DWORD windowStyle = 0;
	myBorderSize = GetWindowBorderSize();
	if (myIsFullScreen)
	{
		if (myHasBorder)
		{
			windowStyle = WS_OVERLAPPEDWINDOW | WS_POPUP | WS_VISIBLE;
			SetWindowLongPtr(myWindowHandle, GWL_STYLE, windowStyle);
			myBorderSize = GetWindowBorderSize();
			InternalSetWindowPos();
		}
		else
		{
			windowStyle = WS_POPUP | WS_VISIBLE;
			SetWindowLongPtr(myWindowHandle, GWL_STYLE, windowStyle);
			myBorderSize = GetWindowBorderSize();
			InternalSetWindowPos();
		}
		myIsFullScreen = false;
	}
	else
	{
		windowStyle = WS_POPUP | WS_VISIBLE;
		SetWindowLongPtr(myWindowHandle, GWL_STYLE, windowStyle);
		myIsFullScreen = true;
		myHasBorder = false;
		RECT desktop;
		const HWND hDesktop = GetDesktopWindow();
		GetWindowRect(hDesktop, &desktop);
		SetWindowPos(myWindowHandle, HWND_TOPMOST, 0, 0, desktop.right, desktop.bottom, SWP_SHOWWINDOW);
		myBorderSize = GetWindowBorderSize();
	}
}

void Engine::WindowHandler::HandleMouseCursorVisibility(bool aCursorVisibility)
{
	if (aCursorVisibility == false)
	{
		if (myHasBorder)
		{
			const v2f mousePos = myInput->GetMousePosition();
			if (mousePos.x < (myBorderSize.x + 10) || mousePos.y < (myBorderSize.y + 10) ||
				mousePos.x > myPreviousSize.x - (myBorderSize.x + 10) || mousePos.y > myPreviousSize.y - (myBorderSize.y + 10))
			{
				ShowCursor(true);
				return;
			}
		}

		while (ShowCursor(false) >= 0);
	}
}

void Engine::WindowHandler::OnMove()
{
	RECT rcWind;
	GetWindowRect(myWindowHandle, &rcWind);
	myDefaultWindowPosition = { (unsigned int)rcWind.left, (unsigned int)rcWind.top };
}

v2ui Engine::WindowHandler::GetWindowBorderSize()
{
	RECT rcClient, rcWind;
	POINT ptDiff;
	GetClientRect(myWindowHandle, &rcClient);
	GetWindowRect(myWindowHandle, &rcWind);
	ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
	ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;
	return { (unsigned int)ptDiff.x, (unsigned int)ptDiff.y };
}

void Engine::WindowHandler::InternalSetWindowPos()
{
	if (myIsFullScreen || !myHasBorder)
	{
		myDefaultWindowPosition = { 0, 0 };
	}

	if (myHasBorder)
	{
		SetWindowPos(myWindowHandle, HWND_NOTOPMOST, myDefaultWindowPosition.x, myDefaultWindowPosition.y, myPreviousSize.x + (int)myBorderSize.x, myPreviousSize.y + (int)myBorderSize.y, SWP_SHOWWINDOW);
	}
	else
	{
		SetWindowPos(myWindowHandle, HWND_TOPMOST, myDefaultWindowPosition.x, myDefaultWindowPosition.y, myPreviousSize.x - (int)myBorderSize.x, myPreviousSize.y, SWP_SHOWWINDOW);
	}
}
