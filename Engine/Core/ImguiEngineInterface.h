#pragma once
#include "../Externals/imgui/imgui.h"
#include "../Externals/imgui/imgui_impl_dx11.h"
#include "../Externals/imgui/imgui_impl_win32.h"

namespace Engine
{
	class DirectXFramework;
	class WindowHandler;
	class ImguiEngineInterface
	{
	public:
		void Init(DirectXFramework* aDirectXFramework, WindowHandler* aWindowHandler);
		void PreFrame();
		void Render();
		void Destroy();
	};
}


