#pragma once
#include <..\CommonUtilities\CU\Utility\ShortString.h>
#include <..\CommonUtilities\CU\Math\Vector2.hpp>
#include <d3d11.h>

struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;
struct ID3D11InputLayout;
struct ID3D11ShaderResourceView;
struct ID3D11DeviceContext;
struct ID3D11Device;
struct D3D11_MAPPED_SUBRESOURCE;

struct MeshBuffererCommand;
struct MeshRenderCommand;
struct AnimatedMeshRenderCommand;
struct SortedModelDataForBuffers;
struct SortedModelDataForRendering;
struct SortedAnimationDataForBuffers;
struct Material;
class Model;
class Camera;
struct ParticleMeshRenderCommand;
namespace Engine
{
	struct RenderData;
	class ConstantBufferManager;
	class FullScreenTexture;
	struct RenderStates;
	class ModelManager;
	struct ParticleSystem;
	namespace DX
	{
		enum class EModelRenderMode
		{
			EWholeMaterial,
			EOnlyShadersAndTextures,
			EOnlyTextures,
			EOnlyVertexShader,
			ECustomShadersNoTextures,
			ENoMaterial
		};

		bool MapUnmapConstantBuffer(ID3D11DeviceContext* aDeviceContext, ID3D11Buffer* aBufferToMap,
			void* someBufferData, size_t sizeOfData, unsigned int aVSSlot = UINT_MAX, unsigned int aGSSlot = UINT_MAX, unsigned int aPSSlot = UINT_MAX, unsigned int aCSSlot = UINT_MAX);
		bool MapUnmapDynamicBuffer(ID3D11DeviceContext* aContext, ID3D11ShaderResourceView* aSRV, ID3D11Buffer* aBufferToMap, void* someBufferData,
			size_t sizeOfData, unsigned short aAmountOfItemsToMap, unsigned int aSRVSlot, bool aVSSlot = false, bool aGSSlot = false, bool aPSSlot = true, bool aCSSlot = true );
		bool MapUnmapDynamicBufferWithRange(ID3D11DeviceContext* aContext, ID3D11ShaderResourceView* aSRV, ID3D11Buffer* aBufferToMap, void* someBufferData,
			size_t sizeOfData, unsigned int aAmountOfItemsToMap, unsigned int aStartSlot, unsigned int aSRVSlot, bool aVSSlot = false, bool aGSSlot = false, bool aPSSlot = true);
		bool CreateStructuredBuffer(ID3D11Device* aDevice, ID3D11ShaderResourceView** aSRV, ID3D11Buffer** aBufferToMap, unsigned int aNumberOfItemsInBuffer, size_t sizeOfData);
		bool CreateStructuredUAVBuffer(ID3D11Device* aDevice, ID3D11ShaderResourceView** aSRV, ID3D11UnorderedAccessView** aUAV, ID3D11Buffer** aBufferToMap, unsigned int aNumberOfItemsInBuffer, size_t sizeOfData);
		bool CreateStructuredStagingBuffer(ID3D11Device* aDevice, ID3D11ShaderResourceView** aSRV, ID3D11Buffer** aBufferToMap, unsigned int aNumberOfItemsInBuffer, size_t sizeOfData);
		ID3D11VertexShader* LoadVS(ID3D11Device* aDevice, ShortString aPath, std::string& someVSData);
		ID3D11VertexShader* LoadVS(ID3D11Device* aDevice, ShortString aPath);
		ID3D11GeometryShader* LoadGS(ID3D11Device* aDevice, ShortString aPath);
		ID3D11PixelShader* LoadPS(ID3D11Device* aDevice, ShortString aPath);
		ID3D11ComputeShader* LoadCS(ID3D11Device* aDevice, ShortString aPath);

		void ClearShaderResources(ID3D11DeviceContext* aDeviceContext, unsigned int aSlot);
		void ClearCSShaderResource(ID3D11DeviceContext* aDeviceContext, unsigned int aSlot);

		void RenderModel(MeshRenderCommand& aModel, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aUniqueModel = false, bool aForceRender = false);
		void RenderRenderTargetModel(MeshRenderCommand& aModel, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, FullScreenTexture* aTextureToMap);
		void RenderAnimatedModel(AnimatedMeshRenderCommand& aModel, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aUniqueModel = false, bool aForceRender = false);

		void RenderDecals(Model* aBox, RenderData& someRenderData, ConstantBufferManager& aBufferManager, std::atomic<unsigned int>& aDrawcallCounter, Camera& aRenderCamera, RenderStates* someRenderStates);

		void RenderInstancedModelBatch(SortedModelDataForRendering& aMeshBuffer, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aForceRender = false);
		void RenderInstancedAnimatedModelBatch(SortedAnimationDataForBuffers& aAnimatedMeshBuffer, ConstantBufferManager& aBufferManager, EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter, bool aOpagueRender, bool aForceRender = false);

		void RenderParticleMeshBatch(ID3D11DeviceContext* aDeviceContext, ParticleMeshRenderCommand* aPMeshList, ConstantBufferManager& aBufferManager, DX::EModelRenderMode aRenderMode, std::atomic<unsigned int>& aDrawcallCounter);


		void SetMaterialSettings(ID3D11DeviceContext* aContext, Material* aMaterial, EModelRenderMode aRenderMode);
		void SampleTexture(ID3D11DeviceContext* aDeviceContext, ID3D11Device* aDevice,
			FullScreenTexture* aTextureToFindValuefrom, CU::Vector2ui aTextureCoordinate, void* aDataToFill, size_t aSizeOfData);

		void SetMRTAsTarget(ID3D11DeviceContext* aDeviceContext, FullScreenTexture** aTextureBatchToSetAsTargets, unsigned int aAmountOfTextures);

	}
}


