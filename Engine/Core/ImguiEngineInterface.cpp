#include "stdafx.h"
#include "Core/ImguiEngineInterface.h"
#include "Core/WindowHandler.h"
#include "Core/DirectXFramework.h"
#include <string>
#include <CU\Utility\FixedString.hpp>

#include "../Externals/imgui/imnodes.h"

inline ImFont* ImGui_LoadFont(ImFontAtlas& atlas, const char* name, float size/*, const ImVec2& displayOffset = ImVec2(0, 0)*/)
{
	char* windir = nullptr;
	if (_dupenv_s(&windir, nullptr, "WINDIR") || windir == nullptr)
		return nullptr;

	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0104, 0x017C, // Polish characters and more
		0,
	};

	ImFontConfig config;
	config.OversampleH = 4;
	config.OversampleV = 4;
	config.PixelSnapH = false;

	auto path = std::string(windir) + "\\Fonts\\" + name;
	auto font = atlas.AddFontFromFileTTF(FixedString256("Editor/") + name, size, &config, ranges);
	/*if (font)
		font->DisplayOffset = displayOffset;*/

	free(windir);

	return font;
}
ImFontAtlas fontAtlas;
ImFont* fnt;

void Engine::ImguiEngineInterface::Init(DirectXFramework* aDirectXFramework, WindowHandler* aWindowHandler)
{
	WindowHandler* handler = nullptr;
	handler = aWindowHandler;
	DirectXFramework* DX11Framework = nullptr;
	DX11Framework = aDirectXFramework;

	//ImGui_LoadFont(fontAtlas, "MavenPro-Regular.ttf", 18.0f);//16.0f * 96.0f / 72.0f);
	//fontAtlas.Build();

	ImGui::CreateContext();
	ImNodes::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.IniFilename = nullptr;
	//io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // yay :)
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // yay :)
	//io.WantCaptureKeyboard = true;
	//io.WantCaptureMouse = true;
	//io.WantTextInput = true;

	// Setup ImGui binding
	ImGui_ImplWin32_Init(handler->GetWindowHandle());
	ImGui_ImplDX11_Init(DX11Framework->GetDevice(), DX11Framework->GetDeviceContext());
	ImGui::StyleColorsDark();

	//ImGui::GetIO().Fonts->AddFontDefault();
	ImFontConfig cfg;
	constexpr ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		//0x0104, 0x017C, // Polish characters and more
		0,
	};
	cfg.OversampleH = 4;
	cfg.OversampleV = 4;
	cfg.PixelSnapH = false;
	//cfg.MergeMode = true;
	//fnt = fontAtlas.AddFontFromFileTTF("Editor/MavenPro-Regular.ttf", 18.0f, &cfg, ranges);
	//fontAtlas.Build();
}

void Engine::ImguiEngineInterface::PreFrame()
{
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	//ImGui::PushFont(fnt);
}

void Engine::ImguiEngineInterface::Render()
{
	//ImGui::PopFont();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	/*ImGuiStyle* style = &ImGui::GetStyle();
	for (int i = 0; i < ImGuiCol_COUNT; ++i)
	{
		style->Colors[i] = { pow(style->Colors[i].x, 1.0f / 2.2f), pow(style->Colors[i].y, 1.0f / 2.2f), pow(style->Colors[i].z, 1.0f / 2.2f), style->Colors[i].w };
	}*/

	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();

	/*for (int i = 0; i < ImGuiCol_COUNT; ++i)
	{
		style->Colors[i] = { pow(style->Colors[i].x, 2.2f), pow(style->Colors[i].y, 2.2f), pow(style->Colors[i].z, 2.2f), style->Colors[i].w };
	}*/
}

void Engine::ImguiEngineInterface::Destroy()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImNodes::DestroyContext();
	ImGui::DestroyContext();
}



