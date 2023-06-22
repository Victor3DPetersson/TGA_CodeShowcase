#pragma once
#include "../RenderConstants.hpp"
#include "..\CommonUtilities\CU\Containers\GrowingArray.hpp"
#include "..\CommonUtilities\CU\Containers\Dictionary.h"
#include "..\CommonUtilities\CU\Containers\Queue.hpp"
#include "..\CommonUtilities\CU\Utility\ShortString.h"

#include "..\GameObjects\ParticleEffectData.h"
#include "..\GameObjects\ParticleEmitter_Instance.h"
#include "..\ECS\Components.h"

#include "..\Core\Rendering\DX_Functions\DX_RenderFunctions.h"

struct ID3D11Device;
class ParticleEmitter;
class ParticleEmitter_Instance;
struct ParticleCommand;
struct MeshesToRender;

namespace MV
{
	class MV_ParticleEditor;
}
namespace Engine
{
	class TextureManager;
	class Renderer;
	class Engine;
	class MaterialManager;
	class ConstantBufferManager;
	struct ParticleSystem
	{
		ShortString myGratKey;
		ParticleEffect myParticleEffect;

		CU::MinHeap<unsigned short> myReturnedCommands;
		ParticleRenderCommand myRenderCommands[NUMB_PARTICLES_PERSYSTEM];
		ParticleRenderCommand mySystemRenderCommand;
		ParticleEmissionData myEmissionData[NUMB_PARTICLES_PERSYSTEM];
		unsigned short myFetchedCommandCount = 0;
		GUID mySystemIndex;
		ParticleMeshRenderCommand myMeshRenderC;
	};

	class ParticleManager
	{
	public:
	public:
		//void PreHeatParticleEmitter(const ParticleEmitterComponent aCommandToPreHeat, const int aAmountOfSecondsToPreHeat);
		bool ImportParticleSystem(const ShortString aGratKey);
		//void CreateEmptySystem(const ShortString aGratKey);
		bool CreateParticleEmitter(const ShortString aGratKey, ParticleEmitterComponent& aParticleCommandToFill);
		bool CreateParticleEmitter(const GUID aKey, ParticleEmitterComponent& aParticleCommandToFill);
		bool CreateParticleEmitter(const ShortString aGratKey, ParticleMultiEmitterComponent& aParticleCommandToFill, unsigned short aSubSystemToFill);
		bool CreateParticleEmitter(const GUID aKey, ParticleMultiEmitterComponent& aParticleCommandToFill, unsigned short aSubSystemToFill);
		
		void ResetParticleEmitter(ParticleEmitterComponent& aCommmandToReset);
		void ResetParticleEmitter(ParticleMultiEmitterComponent& aCommmandToReset, unsigned short aSubSystemToReset);

		void ReturnParticle(ParticleEmitterComponent& aSystemToReturn);
		void ReturnParticle(ParticleMultiEmitterComponent& aSystemToReturn, unsigned short aSubSystemIndex);

		void ClearAllParticles();
		//Support Function if one wants the ID handle for the system
		GUID GetGUIDFromName(const ShortString aGratKey);

		ParticleSystem* GetSystem(GUID aKey) { return mySystems.Get(aKey); }


		//Used when destroying the manager
		void ReleaseAllResources();

		void Update(ParticleCommand* commands, size_t commandSize, float aDeltaTime);
		void UpdateDepthSort(const v3f& aCameraPosition);

		void Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, TextureManager* aTextureManager, MaterialManager* aMaterialManager);
		const ParticleEmissionData GetEmissionData(GUID aKey, unsigned short aSubSystem);

		void FillMeshParticles(MeshesToRender& aListToFill);

	private:
		const unsigned int GetNumberOfRenderCommands() const { return myNumberOfRenderCommands; }
		bool ReadGratSprut(const ShortString aGratKey, ParticleEffect::ParticleSettings& someDataToFill, ShortString& aMaterialName, MaterialTypes& aMaterialType, GUID& aGUIDtoFill);
		bool ExportSystem(ParticleSystem* aSystemToTexport);
		void ClearEmission(ParticleEmissionData& someDataToClear);
		void InitMeshData(ParticleSystem* aSystem, unsigned short aNumberOfParticles);
		void ReInitSRVs();

		friend class Renderer;
		friend class Engine;
		friend class MV::MV_ParticleEditor;
		ID3D11Device* myDevice;
		TextureManager* myTextureManager;
		MaterialManager* myMaterialManager;
		CU::Dictionary<GUID, ParticleSystem> mySystems;
		ParticleRenderCommand myRenderCommands[MAX_PARTICLES];
		unsigned int myNumberOfRenderCommands;
		unsigned int myNumberOfSystemsCommands;
		unsigned int myMaxNumberOfVertices = 0;
		unsigned int myMaxNumberOfMeshes = 0;

		ID3D11DeviceContext* myContext = nullptr;

		ID3D11ShaderResourceView* myModelOBToWorldSRV = nullptr;
		ID3D11Buffer* myModelOBToWorldBuffer = nullptr;
		ID3D11ShaderResourceView* myModelEffectSRV = nullptr;
		ID3D11Buffer* myModelEffectBuffer = nullptr;
	};
}


