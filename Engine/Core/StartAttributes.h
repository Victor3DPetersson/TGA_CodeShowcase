#pragma once
#include <functional>
#include "..\..\CommonUtilities/CU/Math/Color.hpp"
#include <filesystem>
#include <Windows.h>
#include <string>



namespace Engine
{
	using callback_function = std::function<void()>;
	using callback_function_dt = std::function<void(float)>;
	using dragNDrop_function = std::function<void(std::filesystem::path)>;
	enum EResolutions
	{
		E2160p,
		E1440p,
		E1080p,
		E720p,
		E480p,
		E360p,
		EResolutionCOUNT
	};
	enum EWindowSettings
	{
		ENone = 1 << 0,
		EFullscreen = 1 << 1,
		EIsResizing = 1 << 2,
		EIsDoneResizing = 1 << 3,
		EIsTabbedOut = 1 << 4,
		EIsTabbedIn = 1 << 5,
		EUpdateResolution_FromEvent = 1 << 6,
		EAltTab = 1 << 7,
		EBordered = 1 << 8,
		EChangeResolution = 1 << 9,
		EMove = 1 << 10
	};
	struct WindowsMessage
	{
		UINT myMessage = 0;
		uint16_t myRenderResolution = 0;
		uint16_t myWindowResolution = 0;
	};
	enum StartupFlags_
	{
		StartupFlags_None,
		StartupFlags_SuppressToolsMenu = 1 << 0,
		StartupFlags_MaximizeWindow = 1 << 1,
		StartupFlags_HideCursor = 1 << 2
	};

	struct StartupAttributes
	{
		callback_function myInitGameFunctionToCall = nullptr;
		callback_function myInitEditorFunctionToCall = nullptr;
		callback_function_dt myUpdateFunctionToCall = nullptr;
		callback_function_dt myRenderFunctionToCall = nullptr;
		callback_function myShutdownFunctionToCall = nullptr;
		/* How big should the window be? */
		EResolutions myWindowResolution = E720p;

		unsigned short myStartPositionX = 400;
		unsigned short myStartPositionY = 400;

		std::wstring myApplicationName;
		//bool myEnableVSync;
		bool myStartInFullScreen = false;
		CU::Color myBackgroundColor;
		bool myAcceptDragNDrop = false;
		dragNDrop_function myDragNDropHandleFunction;
		UINT myWindowSettings = 0;
		int startupFlags = StartupFlags_None;
	};
}