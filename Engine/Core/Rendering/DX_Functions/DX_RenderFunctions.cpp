#include "stdafx.h"
#include "DX_RenderFunctions.h"

#include <d3d11.h>

#include <fstream>
#include <cstdlib>
#include <filesystem>

#include <assert.h>

#include "RenderData.h"

#include "GameObjects\Material.h"
#include "GameObjects\Model.h"
#include "ECS\Systems\RenderCommands.h"
#include "..\Resources\ConstantBufferManager.h"
#include "..\Resources\FullScreenTexture.h"
#include "..\Resources\RenderStates.h"
#include "CU\Collision\Intersection.hpp"

#include "Managers\Managers.h"
#include "Managers\ModelManager.h"
#include "Managers\ParticleManager.h"

bool Engine::DX::MapUnmapConstantBuffer(ID3D11DeviceContext* aDeviceContext, ID3D11Buffer* aBufferToMap, void* someBufferData, size_t sizeOfData, unsigned int aVSSlot, unsigned int aGSSlot, unsigned int aPSSlot, unsigned int aCSSlot)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE resource;
	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	result = aDeviceContext->Map(aBufferToMap, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	if (FAILED(result))
	{
		assert(false && "Failed To Map Resources");
		return false;
	}
	memcpy(resource.pData, someBufferData, sizeOfData);
	aDeviceContext->Unmap(aBufferToMap, 0);
	if (aVSSlot != UINT_MAX)
	{
		aDeviceContext->VSSetConstantBuffers(aVSSlot, 1, &aBufferToMap);
	}
	if (aGSSlot != UINT_MAX)
	{
		aDeviceContext->GSSetConstantBuffers(aGSSlot, 1, &aBufferToMap);
	}
	if (aPSSlot != UINT_MAX)
	{
		aDeviceContext->PSSetConstantBuffers(aPSSlot, 1, &aBufferToMap);
	}
	if (aCSSlot != UINT_MAX)
	{
		aDeviceContext->CSSetConstantBuffers(aCSSlot, 1, &aBufferToMap);
	}
	return true;
}
bool Engine::DX::MapUnmapDynamicBuffer(ID3D11DeviceContext* aContext, ID3D11ShaderResourceView* aSRV, ID3D11Buffer* aBufferToMap, void* someBufferData, size_t sizeOfData, unsigned short aAmountOfItemsToMap, unsigned int aSRVSlot, bool aVSSlot, bool aGSSlot, bool aPSSlot, bool aCSSlot)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE resource;

	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	result = aContext->Map(aBufferToMap, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	if (FAILED(result))
	{
		assert(false && "Victor you have fucked up your structured homie");
		return false;
	}
	memcpy(resource.pData, someBufferData, aAmountOfItemsToMap * sizeOfData);
	aContext->Unmap(aBufferToMap, 0);

	if (aVSSlot)
	{
		aContext->VSSetShaderResources(aSRVSlot, 1, &aSRV);
	}
	if (aGSSlot)
	{
		aContext->GSSetShaderResources(aSRVSlot, 1, &aSRV);
	}
	if (aPSSlot)
	{
		aContext->PSSetShaderResources(aSRVSlot, 1, &aSRV);
	}
	if (aCSSlot)
	{
		aContext->CSSetShaderResources(aSRVSlot, 1, &aSRV);
	}
	return true;
}
bool Engine::DX::MapUnmapDynamicBufferWithRange(ID3D11DeviceContext* aContext, ID3D11ShaderResourceView* aSRV, ID3D11Buffer* aBufferToMap, void* someBufferData, size_t sizeOfData, unsigned int aAmountOfItemsToMap, unsigned int aStartSlot, unsigned int aSRVSlot, bool aVSSlot, bool aGSSlot, bool aPSSlot)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE resource;

	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	result = aContext->Map(aBufferToMap, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	if (FAILED(result))
	{
		assert(false && "Victor you have fucked up your structured homie");
		return false;
	}
	size_t startSlot = (aStartSlot * sizeOfData);
	memcpy((char*)resource.pData + startSlot, (char*)someBufferData + startSlot, aAmountOfItemsToMap * sizeOfData);
	aContext->Unmap(aBufferToMap, 0);

	if (aVSSlot)
	{
		aContext->VSSetShaderResources(aSRVSlot, 1, &aSRV);
	}
	if (aGSSlot)
	{
		aContext->GSSetShaderResources(aSRVSlot, 1, &aSRV);
	}
	if (aPSSlot)
	{
		aContext->PSSetShaderResources(aSRVSlot, 1, &aSRV);
	}
	return true;
}
bool Engine::DX::CreateStructuredBuffer(ID3D11Device* aDevice, ID3D11ShaderResourceView** aSRV, ID3D11Buffer** aBufferToMap, unsigned int aNumberOfItemsInBuffer, size_t sizeOfData)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = aNumberOfItemsInBuffer;

	HRESULT result;
	D3D11_BUFFER_DESC pointsBufferDesc = { 0 };
	pointsBufferDesc.ByteWidth = (unsigned int)sizeOfData * aNumberOfItemsInBuffer;
	pointsBufferDesc.StructureByteStride = (unsigned int)sizeOfData;
	pointsBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	pointsBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	pointsBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pointsBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	result = aDevice->CreateBuffer(&pointsBufferDesc, nullptr, &(*aBufferToMap));
	if (FAILED(result))
	{
		assert(false && "Failed To Create Structured Buffer");
		(*aBufferToMap) = nullptr;
		return false;
	}
	result = aDevice->CreateShaderResourceView((*aBufferToMap), &srvDesc, &(*aSRV));
	if (FAILED(result))
	{
		assert(false && "Failed To Create Structured Buffer Resource View");
		return false;
	}
	return true;
}

bool Engine::DX::CreateStructuredUAVBuffer(ID3D11Device* aDevice, ID3D11ShaderResourceView** aSRV, ID3D11UnorderedAccessView** aUAV, ID3D11Buffer** aBufferToMap, unsigned int aNumberOfItemsInBuffer, size_t sizeOfData)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = aNumberOfItemsInBuffer;

	HRESULT result;
	D3D11_BUFFER_DESC pointsBufferDesc = { 0 };
	pointsBufferDesc.ByteWidth = (unsigned int)sizeOfData * aNumberOfItemsInBuffer;
	pointsBufferDesc.StructureByteStride = (unsigned int)sizeOfData;
	pointsBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	pointsBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	pointsBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	pointsBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	D3D11_BUFFER_UAV uavBuffer;
	uavBuffer.FirstElement = 0;
	uavBuffer.NumElements = aNumberOfItemsInBuffer;
	uavBuffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
	uavDesc.Buffer = uavBuffer;

	result = aDevice->CreateBuffer(&pointsBufferDesc, nullptr, &(*aBufferToMap));
	if (FAILED(result))
	{
		assert(false && "Failed To Create Structured Buffer");
		(*aBufferToMap) = nullptr;
		return false;
	}
	result = aDevice->CreateShaderResourceView((*aBufferToMap), &srvDesc, &(*aSRV));
	if (FAILED(result))
	{
		assert(false && "Failed To Create Structured Buffer Resource View");
		return false;
	}
	result = aDevice->CreateUnorderedAccessView((*aBufferToMap), &uavDesc, &(*aUAV));
	if (FAILED(result))
	{
		assert(false && "Failed To create UAV");
		return false;
	}
	return true;
}

bool Engine::DX::CreateStructuredStagingBuffer(ID3D11Device* aDevice, ID3D11ShaderResourceView** aSRV, ID3D11Buffer** aBufferToMap, unsigned int aNumberOfItemsInBuffer, size_t sizeOfData)
{
	return false;
}


ID3D11VertexShader* Engine::DX::LoadVS(ID3D11Device* aDevice, ShortString aPath, std::string& someVSData)
{
	HRESULT result;

	std::ifstream file;
	file.open(aPath.GetString(), std::ios::binary);
	someVSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11VertexShader* VS;
	result = aDevice->CreateVertexShader(someVSData.data(), someVSData.size(), nullptr, &VS);
	if (FAILED(result))
	{
		return nullptr;
	}
	file.close();
	return VS;
}
ID3D11VertexShader* Engine::DX::LoadVS(ID3D11Device* aDevice, ShortString aPath)
{
	HRESULT result;

	std::ifstream file;
	std::string VSData;
	file.open(aPath.GetString(), std::ios::binary);
	VSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11VertexShader* VS;
	result = aDevice->CreateVertexShader(VSData.data(), VSData.size(), nullptr, &VS);
	if (FAILED(result))
	{
		return nullptr;
	}
	file.close();
	return VS;
}
ID3D11GeometryShader* Engine::DX::LoadGS(ID3D11Device* aDevice, ShortString aPath)
{
	HRESULT result;

	std::ifstream file;
	file.open(aPath.GetString(), std::ios::binary);
	std::string someGSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11GeometryShader* GS;
	result = aDevice->CreateGeometryShader(someGSData.data(), someGSData.size(), nullptr, &GS);
	if (FAILED(result))
	{
		return nullptr;
	}
	file.close();
	return GS;
}
ID3D11PixelShader* Engine::DX::LoadPS(ID3D11Device* aDevice, ShortString aPath)
{
	HRESULT result;

	std::ifstream file;
	file.open(aPath.GetString(), std::ios::binary);
	std::string somePSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11PixelShader* PS;
	result = aDevice->CreatePixelShader(somePSData.data(), somePSData.size(), nullptr, &PS);
	if (FAILED(result))
	{
		return nullptr;
	}
	file.close();
	return PS;
}

ID3D11ComputeShader* Engine::DX::LoadCS(ID3D11Device* aDevice, ShortString aPath)
{
	HRESULT result;

	std::ifstream file;
	file.open(aPath.GetString(), std::ios::binary);
	std::string someCSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11ComputeShader* CS;
	result = aDevice->CreateComputeShader(someCSData.data(), someCSData.size(), nullptr, &CS);
	if (FAILED(result))
	{
		return nullptr;
	}
	file.close();
	return CS;
}

void Engine::DX::ClearShaderResources(ID3D11DeviceContext* aDeviceContext, unsigned int aSlot)
{
	ID3D11ShaderResourceView* null[] = { nullptr };
	aDeviceContext->VSSetShaderResources(aSlot, 1, null);
	aDeviceContext->GSSetShaderResources(aSlot, 1, null);
	aDeviceContext->PSSetShaderResources(aSlot, 1, null);
}

void Engine::DX::ClearCSShaderResource(ID3D11DeviceContext* aDeviceContext, unsigned int aSlot)
{
	ID3D11ShaderResourceView* null[] = { nullptr };
	aDeviceContext->CSSetShaderResources(aSlot, 1, null);
}

void Engine::DX::RenderModel(MeshRenderCommand& aModel, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aUniqueModel, bool aForceRender)
{
	ID3D11DeviceContext* context = aBufferManager.GetContext();
	Model* model = aModel.model;
	ObjectEffectData oldObjectEffect = aModel.effectData;
	if (aUniqueModel)
	{
		aModel.effectData.gBufferVSEffectIndex += 1;
		aModel.effectData.gBufferPSEffectIndex += 1;
	}
	aBufferManager.MapUnMapObjectToBuffers(aModel);
	for (unsigned short i = 0; i < model->GetAmountOfSubModels(); i++)
	{
		Material* material = model->GetModelData(i).myMaterial;
		if (material == nullptr)
		{
			continue;
		}
		bool render = true;
		if (aOpagueRender)
		{
			if (material->myIsCutOut == false && material->myMaterialType == MaterialTypes::EPBR_Transparent)
			{
				render = false;
			}
		}
		else
		{
			if (material->myMaterialType != MaterialTypes::EPBR_Transparent)
			{
				render = false;
			}
		}
		if (render || aForceRender)
		{
			if (material->myMaterialType != MaterialTypes::ERenderTarget)
			{
				bool updatedEffect = false;
				if (oldObjectEffect.gBufferPSEffectIndex == 0 && material->myPSEffectIndex != 0)
				{
					aModel.effectData.gBufferPSEffectIndex = material->myPSEffectIndex * 2;
					if (aUniqueModel)
					{
						aModel.effectData.gBufferPSEffectIndex += 1;
					}
					aBufferManager.MapUnMapObjectToBuffers(aModel);
					aModel.effectData.gBufferPSEffectIndex = oldObjectEffect.gBufferPSEffectIndex;
					updatedEffect = true;
				}
				SetMaterialSettings(context, material, aRenderMode);
				context->IASetVertexBuffers(0, 1, &model->GetModelData(i).myVertexBuffer, &model->GetModelData(i).myStride, &model->GetModelData(i).myOffset);
				context->IASetIndexBuffer(model->GetModelData(i).myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
				context->DrawIndexed(model->GetModelData(i).myNumberOfIndices, 0, 0);
				aDrawcallCounter++;
				if (updatedEffect)
				{
					aBufferManager.MapUnMapObjectToBuffers(aModel);
				}
			}
		}
	}
	aModel.effectData = oldObjectEffect;
}

void Engine::DX::RenderRenderTargetModel(MeshRenderCommand& aModel, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, FullScreenTexture* aTextureToMap)
{
	ID3D11DeviceContext* context = aBufferManager.GetContext();
	Model* model = aModel.model;
	ID3D11ShaderResourceView* SRV = aTextureToMap->GetSRV();
	ObjectEffectData oldObjectEffect = aModel.effectData;
	aModel.effectData.gBufferVSEffectIndex = 1;
	aModel.effectData.gBufferPSEffectIndex += 1;
	aBufferManager.MapUnMapObjectToBuffers(aModel);
	aModel.effectData = oldObjectEffect;
	for (unsigned short i = 0; i < model->GetAmountOfSubModels(); i++)
	{
		Material* material = model->GetModelData(i).myMaterial;
		if (material->myMaterialType == MaterialTypes::ERenderTarget)
		{
			SetMaterialSettings(context, material, aRenderMode);
			context->PSSetShaderResources(8, 1, &SRV);
			context->IASetVertexBuffers(0, 1, &model->GetModelData(i).myVertexBuffer, &model->GetModelData(i).myStride, &model->GetModelData(i).myOffset);
			context->IASetIndexBuffer(model->GetModelData(i).myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(model->GetModelData(i).myNumberOfIndices, 0, 0);
			aDrawcallCounter++;
		}
	}
}

void Engine::DX::RenderAnimatedModel(AnimatedMeshRenderCommand& aModel, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aUniqueModel, bool aForceRender)
{
	ID3D11DeviceContext* context = aBufferManager.GetContext();
	ModelAnimated* model = aModel.model;
	ObjectEffectData oldObjectEffect = aModel.effectData;
	if (aUniqueModel)
	{
		aModel.effectData.gBufferVSEffectIndex += 1;
		aModel.effectData.gBufferPSEffectIndex += 1;
	}
	aBufferManager.MapUnMapAnimatedObjectToBuffers(aModel);
	for (unsigned short i = 0; i < model->GetAmountOfSubModels(); i++)
	{
		Material* material = aModel.model->GetModelData(i).myMaterial;
		bool render = true;
		if (aOpagueRender)
		{
			if (material->myIsCutOut == false && material->myMaterialType == MaterialTypes::EPBRTransparent_Anim)
			{
				render = false;
			}
		}
		else
		{
			if (material->myMaterialType != MaterialTypes::EPBRTransparent_Anim)
			{
				render = false;
			}
		}
		if (render || aForceRender)
		{
			bool updatedEffect = false;
			if (oldObjectEffect.gBufferPSEffectIndex == 0 && material->myPSEffectIndex != 0)
			{
				aModel.effectData.gBufferPSEffectIndex = material->myPSEffectIndex * 2;
				if (aUniqueModel)
				{
					aModel.effectData.gBufferPSEffectIndex += 1;
				}
				aBufferManager.MapUnMapAnimatedObjectToBuffers(aModel);
				aModel.effectData.gBufferPSEffectIndex = oldObjectEffect.gBufferPSEffectIndex;
				updatedEffect = true;
			}
			SetMaterialSettings(context, material, aRenderMode);
			context->IASetVertexBuffers(0, 1, &model->GetModelData(i).myVertexBuffer, &model->GetModelData(i).myStride, &model->GetModelData(i).myOffset);
			context->IASetIndexBuffer(model->GetModelData(i).myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(model->GetModelData(i).myNumberOfIndices, 0, 0);
			aDrawcallCounter++;
			if (updatedEffect)
			{
				aModel.effectData.gBufferPSEffectIndex = oldObjectEffect.gBufferPSEffectIndex + 1;
				aBufferManager.MapUnMapAnimatedObjectToBuffers(aModel);
			}
		}
	}
	aModel.effectData = oldObjectEffect;
}

void Engine::DX::RenderInstancedModelBatch(SortedModelDataForRendering& aMeshBuffer, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aForceRender)
{
	const unsigned int amountOfInstances = aMeshBuffer.numberOfModels;
	if (amountOfInstances > 0)
	{
		ModelData& modelData = *aMeshBuffer.model;
		Material* material = modelData.myMaterial;
		bool render = true;
		if (aOpagueRender)
		{
			if (material->myIsCutOut == false && material->myMaterialType == MaterialTypes::EPBR_Transparent)
			{
				render = false;
			}
		}
		else
		{
			if (material->myMaterialType != MaterialTypes::EPBR_Transparent)
			{
				render = false;
			}
		}
		if (render || aForceRender)
		{
			ID3D11DeviceContext* context = aBufferManager.GetContext();
			aBufferManager.MapUnMapStructuredMeshBuffer(aMeshBuffer);
			SetMaterialSettings(context, material, aRenderMode);
			context->IASetVertexBuffers(0, 1, &modelData.myVertexBuffer, &modelData.myStride, &modelData.myOffset);
			context->IASetIndexBuffer(modelData.myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexedInstanced(modelData.myNumberOfIndices, amountOfInstances, 0, 0, 0);
			aDrawcallCounter++;
		}
	}
}

void Engine::DX::RenderDecals(Model* aBox, RenderData& someRenderData, ConstantBufferManager& aBufferManager, std::atomic<unsigned int>& aDrawcallCounter, Camera& aRenderCamera, RenderStates* someRenderStates )
{
	ID3D11DeviceContext* context = aBufferManager.GetContext();

	ModelData* modelData = &aBox->GetModelData();
	context->IASetPrimitiveTopology(modelData->myMaterial->myPrimitiveTopology);
	context->IASetInputLayout(modelData->myMaterial->myInputLayout);
	context->IASetVertexBuffers(0, 1, &modelData->myVertexBuffer, &modelData->myStride, &modelData->myOffset);
	context->IASetIndexBuffer(modelData->myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	CU::AABB3Df collider;
	//const CU::Spheref cameraMinField = CU::Spheref(aRenderCamera.GetMatrix().GetTranslationVector(), aRenderCamera.GetNear() * 0.5f);
	const v3f cameraPosition = v3f(aRenderCamera.GetMatrix().GetTranslationVector().x, aRenderCamera.GetMatrix().GetTranslationVector().y, aRenderCamera.GetMatrix().GetTranslationVector().z + aRenderCamera.GetNear() * 0.99f);
	int isInsideDecal = 2;
	for (unsigned short decal = 0; decal < someRenderData.decalsSize; decal++)
	{
		collider = aBox->GetCollider();
		v4f min = { collider.myMin, 1 };
		min = min * someRenderData.decals[decal].matrix;
		v4f max = { collider.myMax, 1 };
		max = max * someRenderData.decals[decal].matrix;
		collider.myMin = { min.x, min.y, min.z };
		collider.myMax = { max.x, max.y, max.z };
		CU::AABB3Df::SortMinMax(collider);
		if (someRenderData.camera.IsAABBInsideFrustum(collider))
		{
			//To flip the winding order of the triangle
			if (collider.IsInside(cameraPosition))
			{
				if (isInsideDecal == 1 || isInsideDecal == 2)
				{
					context->OMSetDepthStencilState(someRenderStates->depthStencilStates[DEPTHSTENCILSTATE_OFF], 0);
					context->RSSetState(someRenderStates->rasterizerStates[RASTERIZERSTATE_COUNTERCLOCKWISE_NODEPTHPASS]);
				}
				isInsideDecal = 0;
			}
			else
			{
				if (isInsideDecal == 0 || isInsideDecal == 2)
				{
					context->RSSetState(someRenderStates->rasterizerStates[RASTERIZERSTATE_DEFAULT]);
					context->OMSetDepthStencilState(someRenderStates->depthStencilStates[DEPTHSTENCILSTATE_READONLY], 0xFF);
				}
				isInsideDecal = 1;
			}

			Material* material = someRenderData.decals[decal].material;
			aBufferManager.MapUnMapDecalToBuffers(someRenderData.decals[decal].matrix);

			context->PSSetShaderResources(8, material->myNumberOfTextures, material->myTextures);
			context->PSSetShader(material->myPixelShader, nullptr, 0);
			aDrawcallCounter++;
			context->DrawIndexed(aBox->GetModelData(0).myNumberOfIndices, 0, 0);
		}
	}
	context->RSSetState(someRenderStates->rasterizerStates[RASTERIZERSTATE_DEFAULT]);
	context->OMSetDepthStencilState(someRenderStates->depthStencilStates[DEPTHSTENCILSTATE_READONLY], 0xFF);
}

void Engine::DX::RenderInstancedAnimatedModelBatch(SortedAnimationDataForBuffers& aAnimatedMeshBuffer, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aForceRender)
{
	const unsigned int amountOfInstances = aAnimatedMeshBuffer.numberOfModels;
	if (amountOfInstances > 0)
	{
		AnimatedModelData& modelData = *aAnimatedMeshBuffer.model;
		Material* material = modelData.myMaterial;
		bool render = true;
		if (aOpagueRender)
		{
			if (material->myIsCutOut == false && material->myMaterialType == MaterialTypes::EPBRTransparent_Anim)
			{
				render = false;
			}
		}
		else
		{
			if (material->myMaterialType != MaterialTypes::EPBRTransparent_Anim)
			{
				render = false;
			}
		}
		if (render || aForceRender)
		{
			ID3D11DeviceContext* context = aBufferManager.GetContext();
			aBufferManager.MapUnMapStructuredAnimatedMeshBuffer(aAnimatedMeshBuffer);
			SetMaterialSettings(context, material, aRenderMode);
			context->IASetVertexBuffers(0, 1, &modelData.myVertexBuffer, &modelData.myStride, &modelData.myOffset);
			context->IASetIndexBuffer(modelData.myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexedInstanced(modelData.myNumberOfIndices, amountOfInstances, 0, 0, 0);
			aDrawcallCounter++;
		}
	}
}

void Engine::DX::RenderParticleMeshBatch(ID3D11DeviceContext* aDeviceContext, ParticleMeshRenderCommand* aPMeshList, ConstantBufferManager& aBufferManager, DX::EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter)
{
	const unsigned int amountOfInstances = aPMeshList->numberOfModels;
	if (amountOfInstances > 0)
	{
		MapUnmapDynamicBuffer(aDeviceContext, aPMeshList->modelOBToWorldSRV, aPMeshList->modelOBToWorldBuffer, &aPMeshList->modelTransforms[0], sizeof(m4f), aPMeshList->numberOfModels, 4, true, true, true);
		MapUnmapDynamicBuffer(aDeviceContext, aPMeshList->modelEffectSRV, aPMeshList->modelEffectBuffer, &aPMeshList->modelsEffectData[0], sizeof(ObjectEffectData), aPMeshList->numberOfModels, 6, true, true, true);
		ModelData& modelData = *aPMeshList->model;
		Material* material = modelData.myMaterial;
		bool render = true;
		if (material->myMaterialType == MaterialTypes::EPBR_Transparent)
		{
			render = false;
		}
		if (render)
		{
			ID3D11DeviceContext* context = aBufferManager.GetContext();
			DX::SetMaterialSettings(context, material, aRenderMode);
			context->IASetVertexBuffers(0, 1, &modelData.myVertexBuffer, &modelData.myStride, &modelData.myOffset);
			context->IASetIndexBuffer(modelData.myIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexedInstanced(modelData.myNumberOfIndices, amountOfInstances, 0, 0, 0);
			aDrawcallCounter++;
		}
	}
}

void Engine::DX::SetMaterialSettings(ID3D11DeviceContext* aContext, Material* aMaterial, EModelRenderMode aRenderMode)
{
	switch (aRenderMode)
	{
	case DX::EModelRenderMode::EWholeMaterial:
		aContext->IASetPrimitiveTopology(aMaterial->myPrimitiveTopology);
		aContext->IASetInputLayout(aMaterial->myInputLayout);
		aContext->VSSetShader(aMaterial->myVertexShader, nullptr, 0);
		aContext->VSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);
		aContext->GSSetShader(aMaterial->myGeometryShader, nullptr, 0);
		aContext->GSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);
		aContext->PSSetShader(aMaterial->myPixelShader, nullptr, 0);
		aContext->PSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);
		break;
	case DX::EModelRenderMode::EOnlyShadersAndTextures:
		aContext->VSSetShader(aMaterial->myVertexShader, nullptr, 0);
		aContext->VSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);

		aContext->GSSetShader(aMaterial->myGeometryShader, nullptr, 0);
		aContext->GSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);

		aContext->PSSetShader(aMaterial->myPixelShader, nullptr, 0);
		aContext->PSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);
		break;
	case DX::EModelRenderMode::EOnlyTextures:
		aContext->IASetPrimitiveTopology(aMaterial->myPrimitiveTopology);
		aContext->IASetInputLayout(aMaterial->myInputLayout);
		aContext->VSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);
		aContext->GSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);
		aContext->PSSetShaderResources(8, aMaterial->myNumberOfTextures, aMaterial->myTextures);
		break;
	case DX::EModelRenderMode::EOnlyVertexShader:
		aContext->VSSetShader(aMaterial->myVertexShader, nullptr, 0);
		aContext->IASetPrimitiveTopology(aMaterial->myPrimitiveTopology);
		aContext->IASetInputLayout(aMaterial->myInputLayout);
		break;
	case DX::EModelRenderMode::ECustomShadersNoTextures:
		aContext->IASetPrimitiveTopology(aMaterial->myPrimitiveTopology);
		aContext->IASetInputLayout(aMaterial->myInputLayout);
		break;
	case DX::EModelRenderMode::ENoMaterial:
		//It does nothing here as Intended
		break;
	default:
		break;
	}
}

void Engine::DX::SampleTexture(ID3D11DeviceContext* aDeviceContext, ID3D11Device* aDevice, FullScreenTexture* aTextureToFindValuefrom, CU::Vector2ui aTextureCoordinate, void* aDataToFill, size_t aSizeOfData)
{	
	HRESULT result;
	ID3D11Texture2D* texture = nullptr;
	D3D11_TEXTURE2D_DESC StagedDesc = {
	1, 1, 1, 1,
	DXGI_FORMAT_R32_UINT,
	1, 0,
	D3D11_USAGE_STAGING,
	0,
	D3D11_CPU_ACCESS_READ,
	0
	};
	result = aDevice->CreateTexture2D(&StagedDesc, NULL, &texture);
	if (FAILED(result))
	{
		assert(false && "Failed to create picking texture");
	}
	D3D11_BOX pickingBox;
	pickingBox.left = aTextureCoordinate.x;
	pickingBox.right = aTextureCoordinate.x + 1;
	pickingBox.top = aTextureCoordinate.y; 
	pickingBox.bottom = aTextureCoordinate.y + 1; 
	pickingBox.front = 0; 
	pickingBox.back = 1; 

	aDeviceContext->CopySubresourceRegion(texture, 0, 0, 0, 0, aTextureToFindValuefrom->GetTexture(), 0, &pickingBox);


	D3D11_MAPPED_SUBRESOURCE MappingDesc;
	result = aDeviceContext->Map(texture, 0, D3D11_MAP_READ, 0, &MappingDesc);
	if (FAILED(result))
	{
		assert(false && "Failed to map texture");
	}
	unsigned int data;
	if (MappingDesc.pData != NULL) {
		data = *((unsigned int*)MappingDesc.pData);
	}
	else
	{
		data = 0;
	}

	aDeviceContext->Unmap(texture, 0);

	texture->Release();
	memcpy(aDataToFill, &data, sizeof(aSizeOfData));
}

void Engine::DX::SetMRTAsTarget(ID3D11DeviceContext* aDeviceContext, FullScreenTexture** aTextureBatchToSetAsTargets, unsigned int aAmountOfTextures)
{
	ID3D11RenderTargetView* viewsToMap[16];
	for (unsigned int RTVs = 0; RTVs < aAmountOfTextures; RTVs++)
	{
		viewsToMap[RTVs] = aTextureBatchToSetAsTargets[RTVs]->GetRTV();
	}
	aDeviceContext->OMSetRenderTargets(aAmountOfTextures, &viewsToMap[0], nullptr);
	aDeviceContext->RSSetViewports(1, aTextureBatchToSetAsTargets[0]->GetViewPort());

}


