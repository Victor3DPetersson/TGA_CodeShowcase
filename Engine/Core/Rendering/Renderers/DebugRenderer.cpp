#include "stdafx.h"
#include "DebugRenderer.h"
#include <d3d11.h>
#include <fstream>
#include "Managers\ModelManager.h"
#include "GameObjects\Camera.h"
#include "GameObjects\ModelData.h"
#include "RenderData.h"
#include "..\DX_Functions\DX_RenderFunctions.h"
#include "imgui\imgui.h"

bool Engine::DebugRenderer::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
{
	myDevice = aDevice;
	myContext = aDeviceContext;
	myDebugSphere = nullptr;
	// Create Sphere here for debug drawer

	HRESULT result;

	std::ifstream vsFile;
	vsFile.open("Content/Shaders/DebugLine_VS.cso", std::ios::binary);
	std::string vsData = { std::istreambuf_iterator<char>(vsFile), std::istreambuf_iterator<char>() };
	result = myDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &myLineVertexShader);
	vsFile.close();
	if (FAILED(result))
		return false;

	std::ifstream psFile;
	psFile.open("Content/Shaders/DebugLine_PS.cso", std::ios::binary);
	std::string psData = { std::istreambuf_iterator<char>(psFile), std::istreambuf_iterator<char>() };
	result = myDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &myLinePixelShader);
	psFile.close();
	if (FAILED(result))
		return false;

	std::ifstream gsFile;
	gsFile.open("Content/Shaders/DebugLine_GS.cso", std::ios::binary);
	std::string gsData = { std::istreambuf_iterator<char>(gsFile), std::istreambuf_iterator<char>() };
	result = myDevice->CreateGeometryShader(gsData.data(), gsData.size(), nullptr, &myLineGeometryShader);
	gsFile.close();
	if (FAILED(result))
		return false;

	Vertex_Debug_Line* temp = new Vertex_Debug_Line[50000];
	D3D11_BUFFER_DESC lineVertexBufferDesc = { 0 };
	lineVertexBufferDesc.ByteWidth = sizeof(Vertex_Debug_Line) * 50000;
	lineVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lineVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	lineVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
	subresourceData.pSysMem = temp;

	ID3D11Buffer* vertexBuffer = nullptr;
	result = aDevice->CreateBuffer(&lineVertexBufferDesc, &subresourceData, &vertexBuffer);
	if (FAILED(result))
	{
		assert(false && "Failed to create Vertex Buffer");
	}
	delete[] temp;
	//Start Layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "COLOR",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//SIZE
		{ "POSITION",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION",1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TRASH",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	ID3D11InputLayout* inputLayout;
	result = aDevice->CreateInputLayout(layout, sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC), vsData.data(), vsData.size(), &inputLayout);
	if (FAILED(result))
	{
		assert(false && "Failed to assemble Line input layout");
	}
	myLineInputLayout = inputLayout;
	myLineVertexBuffer = vertexBuffer;
	if (!InitSphere())
	{
		assert(false && "I fucked up when creating my sphere");
	}

	if (!DX::CreateStructuredBuffer(myDevice, &mySphereDataSRV, &mySphereData, NUMB_DEBUGSPHERES, sizeof(SphereData)))
	{
		return false;
	}
	return true;
}

void Engine::DebugRenderer::Render(RenderData* aRenderBuffer, ID3D11RasterizerState* someRasterizerStates[RASTERIZERSTATE_COUNT], Camera& aRenderCamera)
{
#ifndef _DISTRIBUTION
	if (aRenderBuffer->debugLinesSize)
	{
		//memcpy(&myDebugVertices[0], &aLineCommandList[0], aLineCommandList.Size());
		myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		myContext->IASetInputLayout(nullptr);
		myContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

		myContext->VSSetShader(myLineVertexShader, nullptr, 0);
		myContext->PSSetShader(myLinePixelShader, nullptr, 0);
		myContext->GSSetShader(myLineGeometryShader, nullptr, 0);

		D3D11_MAPPED_SUBRESOURCE res;
		myContext->Map(myLineVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, &aRenderBuffer->debugLines[0], sizeof(Vertex_Debug_Line) * aRenderBuffer->debugLinesSize);
		myContext->Unmap(myLineVertexBuffer, 0);
		unsigned int stride = sizeof(Vertex_Debug_Line);
		unsigned int offset = 0;
		myContext->IASetInputLayout(myLineInputLayout);
		myContext->IASetVertexBuffers(0, 1, &myLineVertexBuffer, &stride, &offset);
		myContext->Draw((unsigned int)aRenderBuffer->debugLinesSize, 0);
	}
	myContext->GSSetShader(nullptr, nullptr, 0);
	if (aRenderBuffer->debugSpheresSize > 0)
	{
		myStructuredSphereData.Clear();
		myContext->VSSetShader(mySphereVShader, nullptr, 0);
		myContext->PSSetShader(mySpherePShader, nullptr, 0);
		myContext->IASetInputLayout(mySphereInputLayout);
		myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		myContext->IASetVertexBuffers(0, 1, &myDebugSphere->myModelData[0].myVertexBuffer, &myDebugSphere->myModelData[0].myStride, &myDebugSphere->myModelData[0].myOffset);
		myContext->IASetIndexBuffer(myDebugSphere->myModelData[0].myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		myContext->RSSetState(someRasterizerStates[RasterizerState::RASTERIZERSTATE_WIREFRAME]);

		m4f transform;
		SphereData data;
		const m4f VP = m4f::GetFastInverse(aRenderCamera.GetMatrix()) * aRenderCamera.GetProjection();
		for (unsigned short i = 0; i < aRenderBuffer->debugSpheresSize; i++)
		{
			transform.SetTranslation(aRenderBuffer->debugSpheres[i].myPosition);
			data.MVP = transform * VP;
			data.color = aRenderBuffer->debugSpheres[i].mySphereColor.GetRGBANormalized();
			data.radius = aRenderBuffer->debugSpheres[i].myRadius;
			myStructuredSphereData.Add(data);
		}
		DX::MapUnmapDynamicBuffer(myContext, mySphereDataSRV, mySphereData, &myStructuredSphereData[0], sizeof(SphereData), myStructuredSphereData.Size(), 29, true, false, false);
		myContext->DrawIndexedInstanced(myDebugSphere->myModelData[0].myNumberOfIndices, (unsigned int)myStructuredSphereData.Size(), 0, 0, 0);
	}
	myContext->RSSetState(someRasterizerStates[RasterizerState::RASTERIZERSTATE_DEFAULT]);
#endif
}
const float PI = 3.1415926f;
bool Engine::DebugRenderer::InitSphere()
{
	float radius = 0.5f;
	UINT sliceCount = 20;
	UINT stackCount = 12;

	float phiStep = PI / stackCount;
	float thetaStep = 2.0f * PI / sliceCount;

	CU::GrowingArray<Vertex_Debug_Mesh> vertices;
	vertices.Init(40);
	Vertex_Debug_Mesh topVertex;
	topVertex.myPosition = { 0, radius, 0 , 1 };
	vertices.Add(topVertex);

	for (UINT i = 1; i <= stackCount - 1; i++)
	{
		float phi = i * phiStep;
		for (UINT j = 0; j <= sliceCount; j++) {
			float theta = j * thetaStep;
			CU::Vector4f p =
			{
				(radius * sinf(phi) * cosf(theta)),
				(radius * cosf(phi)),
				(radius * sinf(phi) * sinf(theta)),
				1
			};
			Vertex_Debug_Mesh vertex;
			vertex.myPosition = p;
			vertices.Add(vertex);
		}
	}
	Vertex_Debug_Mesh botVertex;
	botVertex.myPosition = { 0, -radius, 0 , 1 };
	vertices.Add(botVertex);

	CU::GrowingArray<UINT, unsigned int> vertexIndices;
	vertexIndices.Init(vertices.Size() * 3);

	for (UINT i = 1; i <= sliceCount; i++) {
		vertexIndices.Add(0);
		vertexIndices.Add(i + 1);
		vertexIndices.Add(i);
	}
	UINT baseIndex = 1;
	UINT ringVertexCount = sliceCount + 1;
	for (UINT i = 0; i < stackCount - 2; i++) {
		for (UINT j = 0; j < sliceCount; j++) {
			vertexIndices.Add(baseIndex + i * ringVertexCount + j);
			vertexIndices.Add(baseIndex + i * ringVertexCount + j + 1);
			vertexIndices.Add(baseIndex + (i + 1) * ringVertexCount + j);

			vertexIndices.Add(baseIndex + (i + 1) * ringVertexCount + j);
			vertexIndices.Add(baseIndex + i * ringVertexCount + j + 1);
			vertexIndices.Add(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}
	UINT southPoleIndex = vertices.Size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for (UINT i = 0; i < sliceCount; i++) {
		vertexIndices.Add(southPoleIndex);
		vertexIndices.Add(baseIndex + i);
		vertexIndices.Add(baseIndex + i + 1);
	}

	ModelData_Debug modelData(vertices.Size(), vertexIndices.Size());
	memcpy(&modelData.myVertices[0], &vertices[0], sizeof Vertex_Debug_Mesh * vertices.Size());
	memcpy(&modelData.myIndices[0], &vertexIndices[0], sizeof(unsigned int) * vertexIndices.Size());

	HRESULT result;	

	D3D11_BUFFER_DESC vertexBufferDescription = { 0 };
	vertexBufferDescription.ByteWidth = sizeof(Vertex_Debug_Mesh) * modelData.myNumberOfVertices;
	vertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
	subresourceData.pSysMem = modelData.myVertices;

	ID3D11Buffer* vertexBuffer;
	result = myDevice->CreateBuffer(&vertexBufferDescription, &subresourceData, &vertexBuffer);

	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDescription = { 0 };
	indexBufferDescription.ByteWidth = sizeof(modelData.myIndices[0]) * modelData.myNumberOfIndices;
	indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA indexSubresourceData = { 0 };
	indexSubresourceData.pSysMem = modelData.myIndices;

	ID3D11Buffer* indexBuffer;
	result = myDevice->CreateBuffer(&indexBufferDescription, &indexSubresourceData, &indexBuffer);
	if (FAILED(result))
	{
		return false;
	}
	//Done Vertex

	//Shaders
	std::ifstream vsFile;
	vsFile.open("Content/Shaders/DebugSphere_VS.cso", std::ios::binary);
	std::string vsData = { std::istreambuf_iterator<char>(vsFile), std::istreambuf_iterator<char>() };
	ID3D11VertexShader* vertexShader;
	result = myDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &vertexShader);
	if (FAILED(result))
	{
		return false;
	}
	vsFile.close();
	mySphereVShader = vertexShader;
	std::ifstream psFile;
	psFile.open("Content/Shaders/DebugSphere_PS.cso", std::ios::binary);
	std::string psData = { std::istreambuf_iterator<char>(psFile), std::istreambuf_iterator<char>() };
	ID3D11PixelShader* pixelShader;
	result = myDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &pixelShader);
	if (FAILED(result))
	{
		return false;
	}
	psFile.close();
	mySpherePShader = pixelShader;
	//End Shader

	//Start Layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	result = myDevice->CreateInputLayout(layout, sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC), vsData.data(), vsData.size(), &mySphereInputLayout);
	//End Layout
	modelData.myStride = sizeof(Vertex_Debug_Mesh);
	modelData.myOffset = 0;
	modelData.myVertexBuffer = vertexBuffer;
	modelData.myIndexBuffer = indexBuffer;

	myDebugSphere = new Model_Debug();
	myDebugSphere->Init(modelData);
	myDebugSphere->AddModelData(modelData);
	return true;
}
