#pragma once


struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11RasterizerState;
struct IDXGIAdapter;
namespace CU
{
	class Color;
}
namespace Engine
{
	struct RasterizerStates
	{
		ID3D11RasterizerState* myWireFrameRS;
		ID3D11RasterizerState* mySolidRS;
		ID3D11RasterizerState* mySolidNoCullRS;
	};
	class WindowHandler;
	class DirectXFramework
	{
	public:
		DirectXFramework();
		void ShutDownDX();
		bool Init(WindowHandler& aWindowHandler);
		void BeginFrame(CU::Color& aColor);
		void EndFrame();
		void OnResize(v2ui aWindowRes);
		inline ID3D11DeviceContext* GetDeviceContext() const { return myDeviceContext; }
		inline ID3D11Device* GetDevice() const { return myDevice; }
		ID3D11RenderTargetView* GetBackBuffer() { return myBackBuffer; }
		//RasterizerStates* GetRasterizerStates() { return &myRasterizerStates; }
		v2ui GetWindowResolution();

	private:
		//bool InitRasterizerStates();;
		bool CollectAdapters(v2ui aWindowSize, v2ui& aNumDenumerator, IDXGIAdapter*& outAdapter);

		friend class ModelManager;
		//RasterizerStates myRasterizerStates;
		WindowHandler* myWindowHandler;
		IDXGISwapChain* mySwapChain;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myDeviceContext;
		ID3D11RenderTargetView* myBackBuffer;
		int myVideoCardMemory = 0;
		bool myEnableVSync = false;

	};
}
