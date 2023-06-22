#pragma once
#include "CU\Containers\GrowingArray.hpp"
#include "CU\Containers\VectorOnStack.h"
#include "GameObjects\Vertex.h"
#include "../Engine/ECS/Systems/RenderCommands.h"
#include "RenderData.h"
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11Buffer;
struct ID3D11InputLayout;
struct ID3D11RasterizerState;
struct ID3D11ShaderResourceView;
class Model_Debug;
class Camera;
namespace Engine
{
	class ModelManager;
	struct DebugLineCommand;
	struct DebugSphereCommand;
	struct RenderData;
	class DebugRenderer
	{	
	public:
		struct SphereData
		{
			m4f MVP;
			v4f color;
			float radius;
			v3f padding;
		};	
		enum RasterizerState
		{
			RASTERIZERSTATE_DEFAULT,
			RASTERIZERSTATE_COUNTERCLOCKWISE,
			RASTERIZERSTATE_WIREFRAME,
			RASTERIZERSTATE_DOUBLESIDED,
			RASTERIZERSTATE_COUNT
		};

		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
		void Render(RenderData* aRenderBuffer, ID3D11RasterizerState* someRasterizerStates[RASTERIZERSTATE_COUNT], Camera& aRenderCamera);

	private:
		bool InitSphere();
		Model_Debug* myDebugSphere;
		ID3D11Device* myDevice;
		ID3D11DeviceContext* myContext;
		ID3D11Buffer* myLineVertexBuffer;

		ID3D11InputLayout* myLineInputLayout;
		ID3D11VertexShader* myLineVertexShader;
		ID3D11GeometryShader* myLineGeometryShader;
		ID3D11PixelShader* myLinePixelShader;

		ID3D11VertexShader* mySphereVShader;
		ID3D11PixelShader* mySpherePShader;
		ID3D11InputLayout* mySphereInputLayout;

		CU::VectorOnStack<SphereData, NUMB_DEBUGSPHERES> myStructuredSphereData;
		ID3D11ShaderResourceView* mySphereDataSRV;
		ID3D11Buffer* mySphereData;
	};
}


