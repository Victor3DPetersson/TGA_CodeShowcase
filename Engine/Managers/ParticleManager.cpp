#include "stdafx.h"
#include "ParticleManager.h"
#include "ModelManager.h"
#include "GameObjects\ParticleEmitter_Instance.h"
#include "TextureManager.h"
#include "MaterialManager.h"
#include "Core\ParticleUpdate.h"

#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <filesystem>

#include "../Core/Rendering/Resources/MeshStruct.h"

namespace Engine
{
	struct UpdateLoopData
	{
		unsigned int* numberOfRenderCommands = nullptr;
		ParticleRenderCommand* renderCommandsToRender = nullptr;
		ParticleManager* pm = nullptr;
	};
}


void Engine::ParticleManager::Update(ParticleCommand* commands, size_t commandSize, float aDeltaTime)
{
	GUID systemIndex = NIL_UUID;
	unsigned short emitterIndex = 0;

	myNumberOfRenderCommands = 0;
	mySystems.ForEach([](GUID&, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&)
		{
			cmd.mySystemRenderCommand.myAmountOfActiveVertices = 0;
		});
	for (unsigned short i = 0; i < commandSize; i++)
	{
		systemIndex = commands[i].mySystemIndex;
		ParticleSystem* system = mySystems[systemIndex];
		if (!system) { continue; }
		emitterIndex = (unsigned short)commands[i].myEmitterIndex;
		if (emitterIndex >= system->myFetchedCommandCount) { continue; }
		UpdateParticleComponent(system->myParticleEffect, system->myEmissionData[emitterIndex], &system->myRenderCommands[emitterIndex], &commands[i], aDeltaTime);
		//myRenderCommands[myNumberOfRenderCommands++] = system->myRenderCommands[emitterIndex];
		memcpy(&system->mySystemRenderCommand.myVertices[system->mySystemRenderCommand.myAmountOfActiveVertices], &system->myRenderCommands[emitterIndex].myVertices[0], sizeof(Vertex_Particle) * system->myRenderCommands[emitterIndex].myAmountOfActiveVertices);
		system->mySystemRenderCommand.myAmountOfActiveVertices += system->myRenderCommands[emitterIndex].myAmountOfActiveVertices;
		assert(system->mySystemRenderCommand.myAmountOfActiveVertices < system->myParticleEffect.GetData().myNumberOfParticles * NUMB_PARTICLES_PERSYSTEM && "Too many Active Vertices, something went wrong");
	}
	UpdateLoopData data;
	data.numberOfRenderCommands = &myNumberOfRenderCommands;
	data.renderCommandsToRender = myRenderCommands;
	data.pm = this;
	mySystems.ForEach([](GUID& key, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&, void* someLoopData)
		{
			UpdateLoopData* loopData = (UpdateLoopData*)someLoopData;
			if (cmd.mySystemRenderCommand.myAmountOfActiveVertices > 0)
			{
				if (cmd.mySystemRenderCommand.myIsMeshParticle)
				{
					m4f matrix;
					v4f min, max;
					CU::AABB3Df collider;
					cmd.myMeshRenderC.modelEffectBuffer = loopData->pm->myModelEffectBuffer;
					cmd.myMeshRenderC.modelEffectSRV = loopData->pm->myModelEffectSRV;
					cmd.myMeshRenderC.modelOBToWorldBuffer = loopData->pm->myModelOBToWorldBuffer;
					cmd.myMeshRenderC.modelOBToWorldSRV = loopData->pm->myModelOBToWorldSRV;
					cmd.myMeshRenderC.numberOfModels = (unsigned short)cmd.mySystemRenderCommand.myAmountOfActiveVertices;
					for (unsigned int i = 0; i < cmd.mySystemRenderCommand.myAmountOfActiveVertices; i++)
					{
						if (i >= cmd.myParticleEffect.GetData().myNumberOfParticles * NUMB_PARTICLES_PERSYSTEM)
						{
							matrix = matrix;
						}
						matrix = m4f::Identity;
						const Vertex_Particle& vert = cmd.mySystemRenderCommand.myVertices[i];
						matrix.SetScale(v3f(vert.mySize.x, vert.mySize.y, vert.mySizeZ), false);
						matrix.SetRotation(v3f(vert.myRotationX, vert.myRotationY, vert.myRotation));
						matrix.SetTranslation(vert.myPosition);
						cmd.myMeshRenderC.modelTransforms[i] = matrix;
						collider = cmd.myMeshRenderC.model->myBoundingVolume;
						min = { collider.myMin, 1 };
						min = min * matrix;
						max = { collider.myMax, 1 };
						max = max * matrix;
						collider.myMin = { min.x, min.y, min.z };
						collider.myMax = { max.x, max.y, max.z };
						cmd.myMeshRenderC.modelsColliders[i] = collider;
						cmd.myMeshRenderC.modelsEffectData[i].tValue = vert.myLifetime / vert.myEndTime;
						cmd.myMeshRenderC.modelsEffectData[i].gBufferPSEffectIndex = 140;
						cmd.myMeshRenderC.modelsEffectData[i].effectColor = vert.myColor;
					}
				}
				cmd.mySystemRenderCommand.mySystemIndex = key;
				loopData->renderCommandsToRender[*loopData->numberOfRenderCommands] = cmd.mySystemRenderCommand;
				*loopData->numberOfRenderCommands += 1;
			}
		}, &data);
}

void Engine::ParticleManager::UpdateDepthSort(const v3f& aCameraPosition)
{
	for (unsigned short i = 0; i < myNumberOfRenderCommands; i++)
	{
		UpdateDepthFromCamera(myRenderCommands, aCameraPosition);
	}
}

void Engine::ParticleManager::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, TextureManager* aTextureManager, MaterialManager* aMaterialManager)
{
	myDevice = aDevice;
	myContext = aContext;
	myTextureManager = aTextureManager;
	myMaterialManager = aMaterialManager;

	std::filesystem::path currentPath = std::filesystem::current_path();
	std::wstring outpathAbs = currentPath;
	outpathAbs.append(L"/Content/ParticleSystems/");
	//Preloading systems
	std::string fileName;
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
	{
		if (dirEntry.is_regular_file())
		{
			fileName = dirEntry.path().filename().string();
			fileName.erase(fileName.end() - 10, fileName.end());
			ImportParticleSystem(fileName);
		}
	}
}

const ParticleEmissionData Engine::ParticleManager::GetEmissionData(GUID aKey, unsigned short aSubSystem)
{
	assert(aSubSystem < MAX_COUNTMULTIEMITTER && "Index out of range for multi emitter component");
	return mySystems[aKey]->myEmissionData[aSubSystem];
}

void Engine::ParticleManager::FillMeshParticles(MeshesToRender& aListToFill)
{
	aListToFill.particleMeshCount = 0;
	for (unsigned short i = 0; i < myNumberOfRenderCommands; i++)
	{
		if (myRenderCommands[i].myIsMeshParticle)
		{
			aListToFill.particlesMesh[aListToFill.particleMeshCount++] = mySystems[myRenderCommands[i].mySystemIndex]->myMeshRenderC;
		}
	}
}

bool Engine::ParticleManager::ImportParticleSystem(const ShortString aGratKey)
{
	struct LoopData
	{
		ShortString gratKey;
		bool alreadyLoaded = false;
	}data;
	data.gratKey = aGratKey;
	mySystems.ForEach([](GUID&, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&, void* someLoopData)
		{
			LoopData* loopData = (LoopData*)someLoopData;
			if (cmd.myGratKey == loopData->gratKey)
			{
				loopData->alreadyLoaded = true;
			}
		}, &data);
	if (data.alreadyLoaded)
	{
		return true;
	}

	ParticleEffect::ParticleSettings settingsToFill;
	ShortString materialName;
	MaterialTypes materialType;
	GUID id = NIL_UUID;
	if (ReadGratSprut(aGratKey, settingsToFill, materialName, materialType, id))
	{	
		ParticleSystem system;
		system.myParticleEffect.GetSettings() = settingsToFill;
		system.mySystemIndex = id;
		system.myGratKey = aGratKey;
		if (settingsToFill.myIsMeshParticle == false)
		{
			system.myParticleEffect.GetData().myMaterial = myMaterialManager->GetGratPlat(materialName, materialType);
		}
		if (id == NIL_UUID)
		{
			ExportSystem(&system);
			ReadGratSprut(aGratKey, settingsToFill, materialName, materialType, id);
			system.myParticleEffect.GetSettings() = settingsToFill;
			system.mySystemIndex = id;
			system.myGratKey = aGratKey;
			system.myParticleEffect.GetData().myMaterial = myMaterialManager->GetGratPlat(materialName, materialType);
		}

		mySystems.Insert(system.mySystemIndex, system);	

		HRESULT result;
		ParticleSystem* systemToFill = mySystems[system.mySystemIndex];

		unsigned short amountOfParticles = (unsigned short)(settingsToFill.myParticleMaxLifeTime * settingsToFill.mySpawnRate) + 1;
		Vertex_Particle* vertices = new Vertex_Particle[amountOfParticles * NUMB_PARTICLES_PERSYSTEM];
		D3D11_BUFFER_DESC particleVertexBufferDesc = { 0 };
		particleVertexBufferDesc.ByteWidth = sizeof(Vertex_Particle) * amountOfParticles * NUMB_PARTICLES_PERSYSTEM;
		particleVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		particleVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		particleVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
		subresourceData.pSysMem = vertices;
		ID3D11Buffer* vertexBuffer = nullptr;
		result = myDevice->CreateBuffer(&particleVertexBufferDesc, &subresourceData, &vertexBuffer);
		if (FAILED(result))
		{
			assert(false && "Failed to create Vertex Buffer");
		}
		systemToFill->myParticleEffect.GetData().myParticleVertexBuffer = vertexBuffer;
		systemToFill->myParticleEffect.GetData().myIsInited = true;
		systemToFill->myParticleEffect.GetData().myNumberOfParticles = amountOfParticles;
		delete[] vertices;

		for (unsigned short i = 0; i < NUMB_PARTICLES_PERSYSTEM; i++)
		{
			systemToFill->myRenderCommands[i].myNumberOfParticles = amountOfParticles;
			systemToFill->myRenderCommands[i].myMaterial = system.myParticleEffect.GetData().myMaterial;
			systemToFill->myRenderCommands[i].myParticleVertexBuffer = vertexBuffer;
			systemToFill->myRenderCommands[i].myStride = sizeof(Vertex_Particle);
			systemToFill->myRenderCommands[i].myVertices = new Vertex_Particle[amountOfParticles];
		}
		systemToFill->mySystemRenderCommand.myNumberOfParticles = amountOfParticles * NUMB_PARTICLES_PERSYSTEM;
		systemToFill->mySystemRenderCommand.myMaterial = system.myParticleEffect.GetData().myMaterial;
		systemToFill->mySystemRenderCommand.myParticleVertexBuffer = vertexBuffer;
		systemToFill->mySystemRenderCommand.myStride = sizeof(Vertex_Particle);
		systemToFill->mySystemRenderCommand.myStride = sizeof(Vertex_Particle);
		systemToFill->mySystemRenderCommand.myVertices = new Vertex_Particle[amountOfParticles * NUMB_PARTICLES_PERSYSTEM];

		if (settingsToFill.myIsMeshParticle)
		{
			systemToFill->mySystemRenderCommand.myIsMeshParticle = true;
			InitMeshData(systemToFill, amountOfParticles);
		}
		vertices = nullptr;
		return true;
	}
	return false;
}

bool Engine::ParticleManager::CreateParticleEmitter(const ShortString aGratKey, ParticleEmitterComponent& aParticleComponentToFill)
{
	if (mySystems.Size() > 0)
	{
		ParticleSystem* system = nullptr;
		struct LoopData
		{
			GUID guid = NIL_UUID;
			ShortString gratKey;
		}data;
		data.gratKey = aGratKey;
		mySystems.ForEach([](GUID&, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&, void* someLoopData)
			{
				LoopData* loopData = (LoopData*)someLoopData;
				loopData->guid;
				if (cmd.myGratKey == loopData->gratKey)
				{
					loopData->guid = cmd.mySystemIndex;
				}
			}, &data);
		if (data.guid != NIL_UUID)
		{
			system = mySystems[data.guid];
			aParticleComponentToFill.particleSystem = system->mySystemIndex;
			if (system->myReturnedCommands.Size() != 0)
			{
				unsigned short index = system->myReturnedCommands.Dequeue();
				aParticleComponentToFill.renderIndex = index;
				system->myRenderCommands[index].myAmountOfActiveVertices = 0;
				ClearEmission(system->myEmissionData[index]);
				system->myFetchedCommandCount++;
				return true;
			}
			if (system->myFetchedCommandCount >= NUMB_PARTICLES_PERSYSTEM)
			{
				//assert()
				aParticleComponentToFill.particleSystem = NIL_UUID;
				aParticleComponentToFill.renderIndex = _UI16_MAX;
				return false;
			}
			aParticleComponentToFill.renderIndex = system->myFetchedCommandCount++;
			return true;
		}
	}
	aParticleComponentToFill.particleSystem = NIL_UUID;
	aParticleComponentToFill.renderIndex = _UI16_MAX;
	return false;
}

bool Engine::ParticleManager::CreateParticleEmitter(const GUID aKey, ParticleEmitterComponent& aParticleComponentToFill)
{
	if (mySystems.Size() > 0)
	{
		ParticleSystem* system = mySystems.Get(aKey);
		if (system)
		{
			if (aKey == aParticleComponentToFill.particleSystem && aParticleComponentToFill.renderIndex < system->myFetchedCommandCount)
			{
				return true;
			}
			if (system->myReturnedCommands.Size() != 0)
			{
				unsigned short index = system->myReturnedCommands.Dequeue();
				aParticleComponentToFill.renderIndex = index;
				aParticleComponentToFill.particleSystem = system->mySystemIndex;
				system->myRenderCommands[index].myAmountOfActiveVertices = 0;
				ClearEmission(system->myEmissionData[index]);
				system->myFetchedCommandCount++;
				return true;
			}
			if (system->myFetchedCommandCount >= NUMB_PARTICLES_PERSYSTEM)
			{
				//assert()
				aParticleComponentToFill.particleSystem = NIL_UUID;
				aParticleComponentToFill.renderIndex = _UI16_MAX;
				return false;
			}
			aParticleComponentToFill.particleSystem = system->mySystemIndex;
			aParticleComponentToFill.renderIndex = system->myFetchedCommandCount++;
			return true;
		}
	}
	aParticleComponentToFill.particleSystem = NIL_UUID;
	aParticleComponentToFill.renderIndex = _UI16_MAX;
	return false;
}

bool Engine::ParticleManager::CreateParticleEmitter(const ShortString aGratKey, ParticleMultiEmitterComponent& aParticleCommandToFill, unsigned short aSubSystemToFill)
{
	assert(aSubSystemToFill < MAX_COUNTMULTIEMITTER && "Index out of range for multi emitter component");
	if (mySystems.Size() > 0 && aSubSystemToFill < MAX_COUNTMULTIEMITTER)
	{
		ParticleSystem* system = nullptr;
		struct LoopData
		{
			GUID guid = NIL_UUID;
			ShortString gratKey;
		}data;
		data.gratKey = aGratKey;
		mySystems.ForEach([](GUID&, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&, void* someLoopData)
			{
				LoopData* loopData = (LoopData*)someLoopData;
				loopData->guid;
				if (cmd.myGratKey == loopData->gratKey)
				{
					loopData->guid = cmd.mySystemIndex;
				}
			}, &data);

		ParticleID* systemID = nullptr;
		uint16_t* renderIndex = nullptr;
		switch (aSubSystemToFill)
		{
		case 0:
			systemID = &aParticleCommandToFill.particleSystem1;
			renderIndex = &aParticleCommandToFill.renderIndex1;
			break;
		case 1:
			systemID = &aParticleCommandToFill.particleSystem2;
			renderIndex = &aParticleCommandToFill.renderIndex2;
			break;
		case 2:
			systemID = &aParticleCommandToFill.particleSystem3;
			renderIndex = &aParticleCommandToFill.renderIndex3;
			break;
		default:
			break;
		}
		if (data.guid != NIL_UUID)
		{
			system = mySystems[data.guid];

			*systemID = system->mySystemIndex;

			if (system->myReturnedCommands.Size() != 0)
			{
				unsigned short index = system->myReturnedCommands.Dequeue();
				*renderIndex = index;
				system->myRenderCommands[index].myAmountOfActiveVertices = 0;
				ClearEmission(system->myEmissionData[index]);
				system->myFetchedCommandCount++;
				return true;
			}
			if (system->myFetchedCommandCount >= NUMB_PARTICLES_PERSYSTEM)
			{
				//assert()
				*systemID = NIL_UUID;
				*renderIndex = _UI16_MAX;
				return false;
			}
			*renderIndex = system->myFetchedCommandCount++;
			return true;
		}
		*systemID = NIL_UUID;
		*renderIndex = _UI16_MAX;
	}
	return false;
}

bool Engine::ParticleManager::CreateParticleEmitter(const GUID aKey, ParticleMultiEmitterComponent& aParticleCommandToFill, unsigned short aSubSystemToFill)
{
	assert(aSubSystemToFill < MAX_COUNTMULTIEMITTER && "Index out of range for multi emitter component");
	if (mySystems.Size() > 0 && aSubSystemToFill < MAX_COUNTMULTIEMITTER)
	{
		ParticleID* systemID = nullptr;
		uint16_t* renderIndex = nullptr;
		switch (aSubSystemToFill)
		{
		case 0:
			systemID = &aParticleCommandToFill.particleSystem1;
			renderIndex = &aParticleCommandToFill.renderIndex1;
			break;
		case 1:
			systemID = &aParticleCommandToFill.particleSystem2;
			renderIndex = &aParticleCommandToFill.renderIndex2;
			break;
		case 2:
			systemID = &aParticleCommandToFill.particleSystem3;
			renderIndex = &aParticleCommandToFill.renderIndex3;
			break;
		default:
			break;
		}
		ParticleSystem* system = mySystems.Get(aKey);
		if (system)
		{
			*systemID = system->mySystemIndex;
			if (system->myReturnedCommands.Size() != 0)
			{
				unsigned short index = system->myReturnedCommands.Dequeue();
				*renderIndex = index;
				system->myRenderCommands[index].myAmountOfActiveVertices = 0;
				ClearEmission(system->myEmissionData[index]);
				system->myFetchedCommandCount++;
				return true;
			}
			if (system->myFetchedCommandCount >= NUMB_PARTICLES_PERSYSTEM)
			{
				//assert()
				*systemID = NIL_UUID;
				*renderIndex = _UI16_MAX;
				return false;
			}
			*renderIndex = system->myFetchedCommandCount++;
			return true;
		}
		*systemID = NIL_UUID;
		*renderIndex = _UI16_MAX;
	}
	return false;
}

void Engine::ParticleManager::ResetParticleEmitter(ParticleEmitterComponent& aCommmandToReset)
{
	if (aCommmandToReset.renderIndex == _UI16_MAX){	return; }
	ParticleSystem* system = mySystems.Get(aCommmandToReset.particleSystem);
	if (system)
	{
		ClearEmission(system->myEmissionData[aCommmandToReset.renderIndex]);
		system->myRenderCommands[aCommmandToReset.renderIndex].myAmountOfActiveVertices = 0;
	}
}

void Engine::ParticleManager::ResetParticleEmitter(ParticleMultiEmitterComponent& aCommmandToReset, unsigned short aSubSystemToReset)
{
	assert(aSubSystemToReset < MAX_COUNTMULTIEMITTER && "Index out of range for multi emitter component");
	if (aSubSystemToReset < MAX_COUNTMULTIEMITTER)
	{
		ParticleID* systemID = nullptr;
		uint16_t* renderIndex = nullptr;
		switch (aSubSystemToReset)
		{
		case 0:
			systemID = &aCommmandToReset.particleSystem1;
			renderIndex = &aCommmandToReset.renderIndex1;
			break;
		case 1:
			systemID = &aCommmandToReset.particleSystem2;
			renderIndex = &aCommmandToReset.renderIndex2;
			break;
		case 2:
			systemID = &aCommmandToReset.particleSystem3;
			renderIndex = &aCommmandToReset.renderIndex3;
			break;
		default:
			break;
		}
		ParticleSystem* system = mySystems.Get(*systemID);
		if (system && *renderIndex != _UI16_MAX)
		{
			ClearEmission(system->myEmissionData[*renderIndex]);
			system->myRenderCommands[*renderIndex].myAmountOfActiveVertices = 0;
		}
	}
}


GUID Engine::ParticleManager::GetGUIDFromName(const ShortString aGratKey)
{
	if (mySystems.Size() > 0)
	{
		ParticleSystem* system = nullptr;
		struct LoopData
		{
			GUID guid = NIL_UUID;
			ShortString gratKey;
		}data;
		data.gratKey = aGratKey;
		mySystems.ForEach([](GUID&, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&, void* someLoopData)
			{
				LoopData* loopData = (LoopData*)someLoopData;
				loopData->guid;
				if (cmd.myGratKey == loopData->gratKey)
				{
					loopData->guid = cmd.mySystemIndex;
				}
			}, &data);
		return data.guid;
	}
	return NIL_UUID;
}

bool Engine::ParticleManager::ReadGratSprut(const ShortString aGratKey, ParticleEffect::ParticleSettings& someDataToFill, ShortString& aMaterialName, MaterialTypes& aMaterialType, GUID& aGUIDtoFill)
{
	std::ifstream iMD;
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::wstring outpathAbs = currentPath;
	std::string sKey = aGratKey.GetString();
	std::wstring key(sKey.begin(), sKey.end());
	outpathAbs.append(L"/Content/ParticleSystems/" + key + L".gratsprut");

	iMD.open(outpathAbs, std::ios::in | std::ios::binary);

	if (iMD)
	{
		unsigned int particleVersionIndex;
		iMD.read((char*)&particleVersionIndex, sizeof(particleVersionIndex));
		if (particleVersionIndex == 1)
		{
			struct ParticleImportStruct
			{
				float startColor[4];
				float endColor[4];
				float force[3];
				float spawnRate{};

				float spawnAngle{};
				float speed{};
				float spawnSize{};
				float endSize{};

				float emissiveStrength{};
				float minLifeTime{};
				float maxLifeTime{};
				unsigned int emitterShape;

				unsigned int numberOfCharactersInName;
				char systemName[128];
				unsigned int numberOfCharactersInMaterialName;
				char materialName[128];
				unsigned int materialType;
			}data;
			iMD.read((char*)&data, sizeof(ParticleImportStruct));
			iMD.close();
			someDataToFill.myColorsToBlendBetween[0].myVector.x = data.startColor[0];
			someDataToFill.myColorsToBlendBetween[0].myVector.y = data.startColor[1];
			someDataToFill.myColorsToBlendBetween[0].myVector.z = data.startColor[2];
			someDataToFill.myColorsToBlendBetween[0].myVector.w = data.startColor[3];
			someDataToFill.myColorsToBlendBetween[1].myVector.x = data.endColor[0];
			someDataToFill.myColorsToBlendBetween[1].myVector.y = data.endColor[1];
			someDataToFill.myColorsToBlendBetween[1].myVector.z = data.endColor[2];
			someDataToFill.myColorsToBlendBetween[1].myVector.w = data.endColor[3];
			someDataToFill.myForce = { data.force[0], data.force[1], data.force[2]};
			someDataToFill.mySpawnRate = data.spawnRate;
			someDataToFill.myParticleSpeed = data.speed;
			someDataToFill.myParticleSpawnSizeZ = data.spawnSize;
			someDataToFill.myParticleEndSizeZ = data.endSize;
			someDataToFill.myParticleEmissiveStrength = data.emissiveStrength;
			someDataToFill.myParticleMinLifeTime = data.minLifeTime;
			someDataToFill.myParticleMaxLifeTime = data.maxLifeTime;
			someDataToFill.myEmitterShape = (ParticleEffect::EmitterShape)data.emitterShape;

			memcpy(&aMaterialName, &data.materialName, data.numberOfCharactersInMaterialName);
			aMaterialType = (MaterialTypes)data.materialType;
			//TODO Fixa så att denna exporterar en version 3 med GUID i
		}
		if (particleVersionIndex == 2)
		{
			struct ParticleImportStruct
			{
				unsigned int myNumberOfColors{};
				float myColorBlendTimers[5]{0};
				float myParticleColors[20]{0};
				float myForce[3]{0};
				float myDrag[3]{0};
				float myBoxSize[3]{0};
				float myOffset[3]{0};
				float mySpawnRate{};
				float mySpawnAngle{};
				float myParticleSpeed{};
				float myParticleSpawnSize{};
				float myParticleEndSize{};
				float myParticleEmissiveStrength{};
				float myParticleMinLifeTime{};
				float myParticleMaxLifeTime{};
				float myParticleSpawnMinRotationDirection{};
				float myParticleSpawnMaxRotationDirection{};
				float myParticleRotationSpeed{};
				float myMinRotationSpeed{};
				float myMaxRotationSpeed{};
				unsigned int mySpawnParticleWithRandomRotation{};
				unsigned int myRotateRandomRotation{};

				float myBurstLength{};
				float myBurstSpaceTime{};
				unsigned int myBurstMode{};
				unsigned int myIsContinouslyBursting{};
				float myRadius{};

				unsigned int myEmitterShape{};
				unsigned int myNumberOfCharactersInName{};
				char mySystemName[128]{0};
				unsigned int myNumberOfCharactersInMaterialName{};
				char myMaterialName[128]{0};
				unsigned int myMaterialType{};

				unsigned int myNumberOfSubEmitters{};
				unsigned int myNumberOfCharactersInSubEmitsName[4]{0};
				char mySubEmitNames[4][128]{0};
			}data;
			unsigned int numberOfSubEmitters = 0;
			iMD.read((char*)&numberOfSubEmitters, sizeof(unsigned int));

			iMD.read((char*)&data, sizeof(ParticleImportStruct));
			iMD.close();
			someDataToFill.myNumberOfColors = data.myNumberOfColors;
			for (unsigned int i = 0; i < data.myNumberOfColors; i++)
			{
				someDataToFill.myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
				someDataToFill.myColorBlendTimers[i] = data.myColorBlendTimers[i];
			}
			someDataToFill.myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
			someDataToFill.myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
			someDataToFill.myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
			someDataToFill.myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

			someDataToFill.mySpawnRate = data.mySpawnRate;
			//someDataToFill.mySpawnAngle = data.mySpawnAngle;
			someDataToFill.myParticleSpeed = data.myParticleSpeed;
			someDataToFill.myParticleSpawnSizeZ = data.myParticleSpawnSize;
			someDataToFill.myParticleEndSizeZ = data.myParticleEndSize;
			someDataToFill.myParticleEmissiveStrength = data.myParticleEmissiveStrength;
			someDataToFill.myParticleMinLifeTime = data.myParticleMinLifeTime;
			someDataToFill.myParticleMaxLifeTime = data.myParticleMaxLifeTime;
			someDataToFill.myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
			someDataToFill.myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
			someDataToFill.myMinRotationSpeed = data.myMinRotationSpeed;
			someDataToFill.myMaxRotationSpeed = data.myMaxRotationSpeed;
			someDataToFill.mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
			someDataToFill.myRotateRandomRotation = data.myRotateRandomRotation;
			someDataToFill.myBurstLength = data.myBurstLength;
			someDataToFill.myBurstSpaceTime = data.myBurstSpaceTime;
			someDataToFill.myBurstMode = (bool)data.myBurstMode;
			someDataToFill.myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
			someDataToFill.myRadius = data.myRadius;
			someDataToFill.myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;
			//char matName[128];
			//strcpy_s(matName, data.myMaterialName);
			aMaterialName = data.myMaterialName;
			aMaterialType = (MaterialTypes)data.myMaterialType;

		}
		if (particleVersionIndex == 3)
		{
			struct ParticleImportStruct
			{
				unsigned int myNumberOfColors{};
				float myColorBlendTimers[5]{ 0 };
				float myParticleColors[20]{ 0 };
				float myForce[3]{ 0 };
				float myDrag[3]{ 0 };
				float myBoxSize[3]{ 0 };
				float myOffset[3]{ 0 };
				float mySpawnRate{};
				float mySpawnAngle{};
				float myParticleSpeed{};
				float myParticleSpawnSize{};
				float myParticleEndSize{};
				float myParticleEmissiveStrength{};
				float myParticleMinLifeTime{};
				float myParticleMaxLifeTime{};
				float myParticleSpawnMinRotationDirection{};
				float myParticleSpawnMaxRotationDirection{};
				float myParticleRotationSpeed{};
				float myMinRotationSpeed{};
				float myMaxRotationSpeed{};
				unsigned int mySpawnParticleWithRandomRotation{};
				unsigned int myRotateRandomRotation{};

				float myBurstLength{};
				float myBurstSpaceTime{};
				unsigned int myBurstMode{};
				unsigned int myIsContinouslyBursting{};
				float myRadius{};

				unsigned int myEmitterShape{};
				unsigned int myNumberOfCharactersInName{};
				char mySystemName[128]{ 0 };
				unsigned int myNumberOfCharactersInMaterialName{};
				char myMaterialName[128]{ 0 };
				unsigned int myMaterialType{};

				unsigned int myNumberOfSubEmitters{};
				unsigned int myNumberOfCharactersInSubEmitsName[4]{ 0 };
				char mySubEmitNames[4][128]{ 0 };
				GUID mySystemUUID = NIL_UUID;
			}data;
			unsigned int numberOfSubEmitters = 0;
			iMD.read((char*)&numberOfSubEmitters, sizeof(unsigned int));

			iMD.read((char*)&data, sizeof(ParticleImportStruct));
			iMD.close();
			someDataToFill.myNumberOfColors = data.myNumberOfColors;
			for (unsigned int i = 0; i < data.myNumberOfColors; i++)
			{
				someDataToFill.myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
				someDataToFill.myColorBlendTimers[i] = data.myColorBlendTimers[i];
			}
			someDataToFill.myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
			someDataToFill.myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
			someDataToFill.myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
			someDataToFill.myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

			someDataToFill.mySpawnRate = data.mySpawnRate;
			//someDataToFill.mySpawnAngle = data.mySpawnAngle;
			someDataToFill.myParticleSpeed = data.myParticleSpeed;
			someDataToFill.myParticleSpawnSizeZ = data.myParticleSpawnSize;
			someDataToFill.myParticleEndSizeZ = data.myParticleEndSize;
			someDataToFill.myParticleEmissiveStrength = data.myParticleEmissiveStrength;
			someDataToFill.myParticleMinLifeTime = data.myParticleMinLifeTime;
			someDataToFill.myParticleMaxLifeTime = data.myParticleMaxLifeTime;
			someDataToFill.myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
			someDataToFill.myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
			someDataToFill.myMinRotationSpeed = data.myMinRotationSpeed;
			someDataToFill.myMaxRotationSpeed = data.myMaxRotationSpeed;
			someDataToFill.mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
			someDataToFill.myRotateRandomRotation = data.myRotateRandomRotation;
			someDataToFill.myBurstLength = data.myBurstLength;
			someDataToFill.myBurstSpaceTime = data.myBurstSpaceTime;
			someDataToFill.myBurstMode = (bool)data.myBurstMode;
			someDataToFill.myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
			someDataToFill.myRadius = data.myRadius;
			someDataToFill.myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;
			//char matName[128];
			//strcpy_s(matName, data.myMaterialName);
			aMaterialName = data.myMaterialName;
			aMaterialType = (MaterialTypes)data.myMaterialType;
			aGUIDtoFill = data.mySystemUUID;
		}
		if (particleVersionIndex == 4)
		{
			struct ParticleImportStruct
			{
				unsigned int myNumberOfColors{};
				float myColorBlendTimers[5]{ 0 };
				float myParticleColors[20]{ 0 };
				float myForce[3]{ 0 };
				float myDrag[3]{ 0 };
				float myBoxSize[3]{ 0 };
				float myOffset[3]{ 0 };
				float mySpawnRate{};
				float mySpawnAngle{};
				float myParticleSpeed{};
				float myParticleSpawnSize{};
				float myParticleEndSize{};
				float myParticleEmissiveStrength{};
				float myParticleMinLifeTime{};
				float myParticleMaxLifeTime{};
				float myParticleSpawnMinRotationDirection{};
				float myParticleSpawnMaxRotationDirection{};
				float myParticleRotationSpeed{};
				float myMinRotationSpeed{};
				float myMaxRotationSpeed{};
				unsigned int mySpawnParticleWithRandomRotation{};
				unsigned int myRotateRandomRotation{};

				float myBurstLength{};
				float myBurstSpaceTime{};
				unsigned int myBurstMode{};
				unsigned int myIsContinouslyBursting{};
				float myRadius{};

				unsigned int myEmitterShape{};
				unsigned int myNumberOfCharactersInName{};
				char mySystemName[128]{ 0 };
				unsigned int myNumberOfCharactersInMaterialName{};
				char myMaterialName[128]{ 0 };
				unsigned int myMaterialType{};

				GUID mySystemUUID = NIL_UUID;

				bool myIsBillboard = true;
				bool myIsMeshParticle = false;
				GUID myMeshGUID = NIL_UUID;
				unsigned short mySubmeshID = 0;
				float myParticleSpawnMinRotationDirectionXY[2];
				float myParticleSpawnMaxRotationDirectionXY[2];
				bool mySpawnParticleWithRandomRotationX = false;
				bool mySpawnParticleWithRandomRotationY = false;
				float myParticleSpawnSizeXY[2] = { 1, 1 };
				float myParticleEndSizeXY[2] = { 1.5f, 1.5f };
				bool myIsUniformScale = false;
				float mySpawnForceMin[3] = { -10, -10, -10 };
				float mySpawnForceMax[3] = { 10, 10, 10 };
				bool mySpawnForceRandX = true;
				bool mySpawnForceRandY = true;
				bool mySpawnForceRandZ = true;
				bool mySpawnForceMirrorX = true;
				bool mySpawnForceMirrorY = true;
				bool mySpawnForceMirrorZ = true;
				bool myRotateRandomRotationX = false;
				bool myRotateRandomRotationY = false;
				bool myRotateXAxis = false;
				bool myRotateYAxis = false;
				bool myRotateZAxis = true;
				bool myForwardIsDirection = false;
				bool myUseCurvesEmission = false;
				int myAmountOfEmissionPoints = 3;
				float myEmissionOverTimePoints[40];
				float myEmissiveCurveStrength = 1.0;
				float myEmissiveEndStrength = 1.0;
				bool myUseCurvesSize = false;
				int myAmountOfSizePoints = 3;
				float mySizeCurveStrength = 1.0;
				float mySizeOverTimePoints[40];
				float myBurstSpawnDelay = 0.f;
			}data;
			unsigned int numberOfSubEmitters = 0;
			iMD.read((char*)&data, sizeof(ParticleImportStruct));
			iMD.close();
			someDataToFill.myNumberOfColors = data.myNumberOfColors;
			for (unsigned int i = 0; i < data.myNumberOfColors; i++)
			{
				someDataToFill.myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
				someDataToFill.myColorBlendTimers[i] = data.myColorBlendTimers[i];
			}
			someDataToFill.myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
			someDataToFill.myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
			someDataToFill.myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
			someDataToFill.myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

			someDataToFill.mySpawnRate = data.mySpawnRate;
			//someDataToFill.mySpawnAngle = data.mySpawnAngle;
			someDataToFill.myParticleSpeed = data.myParticleSpeed;
			someDataToFill.myParticleSpawnSizeZ = data.myParticleSpawnSize;
			someDataToFill.myParticleEndSizeZ = data.myParticleEndSize;
			someDataToFill.myParticleEmissiveStrength = data.myParticleEmissiveStrength;
			someDataToFill.myParticleMinLifeTime = data.myParticleMinLifeTime;
			someDataToFill.myParticleMaxLifeTime = data.myParticleMaxLifeTime;
			someDataToFill.myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
			someDataToFill.myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
			someDataToFill.myMinRotationSpeed = data.myMinRotationSpeed;
			someDataToFill.myMaxRotationSpeed = data.myMaxRotationSpeed;
			someDataToFill.mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
			someDataToFill.myRotateRandomRotation = data.myRotateRandomRotation;
			someDataToFill.myBurstLength = data.myBurstLength;
			someDataToFill.myBurstSpaceTime = data.myBurstSpaceTime;
			someDataToFill.myBurstMode = (bool)data.myBurstMode;
			someDataToFill.myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
			someDataToFill.myRadius = data.myRadius;
			someDataToFill.myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;
			someDataToFill.myIsBillboard = data.myIsBillboard;
			someDataToFill.myIsMeshParticle = data.myIsMeshParticle;
			someDataToFill.myMeshGUID = data.myMeshGUID;
			someDataToFill.mySubmeshID = data.mySubmeshID;
			someDataToFill.myParticleSpawnMinRotationDirectionXY.x = data.myParticleSpawnMinRotationDirectionXY[0];
			someDataToFill.myParticleSpawnMinRotationDirectionXY.y = data.myParticleSpawnMinRotationDirectionXY[1];
			someDataToFill.myParticleSpawnMaxRotationDirectionXY.x = data.myParticleSpawnMaxRotationDirectionXY[0];
			someDataToFill.myParticleSpawnMaxRotationDirectionXY.y = data.myParticleSpawnMaxRotationDirectionXY[1];
			someDataToFill.mySpawnParticleWithRandomRotationX = data.mySpawnParticleWithRandomRotationX;
			someDataToFill.mySpawnParticleWithRandomRotationY = data.mySpawnParticleWithRandomRotationY;
			someDataToFill.myParticleSpawnSizeXY.x = data.myParticleSpawnSizeXY[0];
			someDataToFill.myParticleSpawnSizeXY.y = data.myParticleSpawnSizeXY[1];
			someDataToFill.myParticleEndSizeXY.x = data.myParticleEndSizeXY[0];
			someDataToFill.myParticleEndSizeXY.y = data.myParticleEndSizeXY[1];
			someDataToFill.myIsUniformScale = data.myIsUniformScale;
			someDataToFill.mySpawnForceMin.x = data.mySpawnForceMin[0];
			someDataToFill.mySpawnForceMin.y = data.mySpawnForceMin[1];
			someDataToFill.mySpawnForceMin.z = data.mySpawnForceMin[2];
			someDataToFill.mySpawnForceMax.x = data.mySpawnForceMax[0];
			someDataToFill.mySpawnForceMax.y = data.mySpawnForceMax[1];
			someDataToFill.mySpawnForceMax.z = data.mySpawnForceMax[2];
			someDataToFill.mySpawnForceRandX = data.mySpawnForceRandX;
			someDataToFill.mySpawnForceRandY = data.mySpawnForceRandY;
			someDataToFill.mySpawnForceRandZ = data.mySpawnForceRandZ;
			someDataToFill.mySpawnForceMirrorX = data.mySpawnForceMirrorX;
			someDataToFill.mySpawnForceMirrorY = data.mySpawnForceMirrorY;
			someDataToFill.mySpawnForceMirrorZ = data.mySpawnForceMirrorZ;
			someDataToFill.myRotateRandomRotationX = data.myRotateRandomRotationX;
			someDataToFill.myRotateRandomRotationY = data.myRotateRandomRotationY;
			someDataToFill.myRotateXAxis = data.myRotateXAxis;
			someDataToFill.myRotateYAxis = data.myRotateYAxis;
			someDataToFill.myRotateZAxis = data.myRotateZAxis;
			someDataToFill.myForwardIsDirection = data.myForwardIsDirection;
			someDataToFill.myUseCurvesEmission = data.myUseCurvesEmission;
			someDataToFill.myAmountOfEmissionPoints = data.myAmountOfEmissionPoints;
			for (int i = 0; i < data.myAmountOfEmissionPoints; i++)
			{
				someDataToFill.myEmissionOverTimePoints[i].x = data.myEmissionOverTimePoints[i * 2];
				someDataToFill.myEmissionOverTimePoints[i].y = data.myEmissionOverTimePoints[i * 2 + 1];
			}
			someDataToFill.myEmissiveCurveStrength = data.myEmissiveCurveStrength;
			someDataToFill.myEmissiveEndStrength = data.myEmissiveEndStrength;
			someDataToFill.myUseCurvesSize = data.myUseCurvesSize;
			someDataToFill.myAmountOfSizePoints = data.myAmountOfSizePoints;
			someDataToFill.mySizeCurveStrength = data.mySizeCurveStrength;
			for (int i = 0; i < data.myAmountOfSizePoints; i++)
			{
				someDataToFill.mySizeOverTimePoints[i].x = data.mySizeOverTimePoints[i * 2];
				someDataToFill.mySizeOverTimePoints[i].y = data.mySizeOverTimePoints[i * 2 + 1];
			}
			someDataToFill.myBurstSpawnDelay = data.myBurstSpawnDelay;
			//char matName[128];
			//strcpy_s(matName, data.myMaterialName);
			aMaterialName = data.myMaterialName;
			aMaterialType = (MaterialTypes)data.myMaterialType;
			aGUIDtoFill = data.mySystemUUID;
		}
		if (particleVersionIndex == 5)
		{
			struct ParticleImportStruct
			{
				unsigned int myNumberOfColors{};
				float myColorBlendTimers[5]{ 0 };
				float myParticleColors[20]{ 0 };
				float myForce[3]{ 0 };
				float myDrag[3]{ 0 };
				float myBoxSize[3]{ 0 };
				float myOffset[3]{ 0 };
				float mySpawnRate{};
				float mySpawnAngle{};
				float myParticleSpeed{};
				float myParticleSpawnSize{};
				float myParticleEndSize{};
				float myParticleEmissiveStrength{};
				float myParticleMinLifeTime{};
				float myParticleMaxLifeTime{};
				float myParticleSpawnMinRotationDirection{};
				float myParticleSpawnMaxRotationDirection{};
				float myParticleRotationSpeed{};
				float myMinRotationSpeed{};
				float myMaxRotationSpeed{};
				unsigned int mySpawnParticleWithRandomRotation{};
				unsigned int myRotateRandomRotation{};

				float myBurstLength{};
				float myBurstSpaceTime{};
				unsigned int myBurstMode{};
				unsigned int myIsContinouslyBursting{};
				float myRadius{};

				unsigned int myEmitterShape{};
				unsigned int myNumberOfCharactersInName{};
				char mySystemName[128]{ 0 };
				unsigned int myNumberOfCharactersInMaterialName{};
				char myMaterialName[128]{ 0 };
				unsigned int myMaterialType{};

				GUID mySystemUUID = NIL_UUID;

				bool myIsBillboard = true;
				bool myIsMeshParticle = false;
				GUID myMeshGUID = NIL_UUID;
				unsigned short mySubmeshID = 0;
				float myParticleSpawnMinRotationDirectionXY[2];
				float myParticleSpawnMaxRotationDirectionXY[2];
				bool mySpawnParticleWithRandomRotationX = false;
				bool mySpawnParticleWithRandomRotationY = false;
				float myParticleSpawnSizeXY[2] = { 1, 1 };
				float myParticleEndSizeXY[2] = { 1.5f, 1.5f };
				bool myIsUniformScale = false;
				float mySpawnForceMin[3] = { -10, -10, -10 };
				float mySpawnForceMax[3] = { 10, 10, 10 };
				bool mySpawnForceRandX = true;
				bool mySpawnForceRandY = true;
				bool mySpawnForceRandZ = true;
				bool mySpawnForceMirrorX = true;
				bool mySpawnForceMirrorY = true;
				bool mySpawnForceMirrorZ = true;
				bool myRotateRandomRotationX = false;
				bool myRotateRandomRotationY = false;
				bool myRotateXAxis = false;
				bool myRotateYAxis = false;
				bool myRotateZAxis = true;
				bool myForwardIsDirection = false;
				bool myUseCurvesEmission = false;
				int myAmountOfEmissionPoints = 3;
				float myEmissionOverTimePoints[40];
				float myEmissiveCurveStrength = 1.0;
				float myEmissiveEndStrength = 1.0;
				bool myUseCurvesSize = false;
				int myAmountOfSizePoints = 3;
				float mySizeCurveStrength = 1.0;
				float mySizeOverTimePoints[40];
				float myBurstSpawnDelay = 0.f;
				char modelName[256];
			}data;
			unsigned int numberOfSubEmitters = 0;
			iMD.read((char*)&data, sizeof(ParticleImportStruct));
			iMD.close();
			someDataToFill.myNumberOfColors = data.myNumberOfColors;
			for (unsigned int i = 0; i < data.myNumberOfColors; i++)
			{
				someDataToFill.myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
				someDataToFill.myColorBlendTimers[i] = data.myColorBlendTimers[i];
			}
			someDataToFill.myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
			someDataToFill.myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
			someDataToFill.myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
			someDataToFill.myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

			someDataToFill.mySpawnRate = data.mySpawnRate;
			//someDataToFill.mySpawnAngle = data.mySpawnAngle;
			someDataToFill.myParticleSpeed = data.myParticleSpeed;
			someDataToFill.myParticleSpawnSizeZ = data.myParticleSpawnSize;
			someDataToFill.myParticleEndSizeZ = data.myParticleEndSize;
			someDataToFill.myParticleEmissiveStrength = data.myParticleEmissiveStrength;
			someDataToFill.myParticleMinLifeTime = data.myParticleMinLifeTime;
			someDataToFill.myParticleMaxLifeTime = data.myParticleMaxLifeTime;
			someDataToFill.myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
			someDataToFill.myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
			someDataToFill.myMinRotationSpeed = data.myMinRotationSpeed;
			someDataToFill.myMaxRotationSpeed = data.myMaxRotationSpeed;
			someDataToFill.mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
			someDataToFill.myRotateRandomRotation = data.myRotateRandomRotation;
			someDataToFill.myBurstLength = data.myBurstLength;
			someDataToFill.myBurstSpaceTime = data.myBurstSpaceTime;
			someDataToFill.myBurstMode = (bool)data.myBurstMode;
			someDataToFill.myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
			someDataToFill.myRadius = data.myRadius;
			someDataToFill.myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;
			someDataToFill.myIsBillboard = data.myIsBillboard;
			someDataToFill.myIsMeshParticle = data.myIsMeshParticle;
			someDataToFill.myMeshGUID = data.myMeshGUID;
			someDataToFill.mySubmeshID = data.mySubmeshID;
			someDataToFill.myParticleSpawnMinRotationDirectionXY.x = data.myParticleSpawnMinRotationDirectionXY[0];
			someDataToFill.myParticleSpawnMinRotationDirectionXY.y = data.myParticleSpawnMinRotationDirectionXY[1];
			someDataToFill.myParticleSpawnMaxRotationDirectionXY.x = data.myParticleSpawnMaxRotationDirectionXY[0];
			someDataToFill.myParticleSpawnMaxRotationDirectionXY.y = data.myParticleSpawnMaxRotationDirectionXY[1];
			someDataToFill.mySpawnParticleWithRandomRotationX = data.mySpawnParticleWithRandomRotationX;
			someDataToFill.mySpawnParticleWithRandomRotationY = data.mySpawnParticleWithRandomRotationY;
			someDataToFill.myParticleSpawnSizeXY.x = data.myParticleSpawnSizeXY[0];
			someDataToFill.myParticleSpawnSizeXY.y = data.myParticleSpawnSizeXY[1];
			someDataToFill.myParticleEndSizeXY.x = data.myParticleEndSizeXY[0];
			someDataToFill.myParticleEndSizeXY.y = data.myParticleEndSizeXY[1];
			someDataToFill.myIsUniformScale = data.myIsUniformScale;
			someDataToFill.mySpawnForceMin.x = data.mySpawnForceMin[0];
			someDataToFill.mySpawnForceMin.y = data.mySpawnForceMin[1];
			someDataToFill.mySpawnForceMin.z = data.mySpawnForceMin[2];
			someDataToFill.mySpawnForceMax.x = data.mySpawnForceMax[0];
			someDataToFill.mySpawnForceMax.y = data.mySpawnForceMax[1];
			someDataToFill.mySpawnForceMax.z = data.mySpawnForceMax[2];
			someDataToFill.mySpawnForceRandX = data.mySpawnForceRandX;
			someDataToFill.mySpawnForceRandY = data.mySpawnForceRandY;
			someDataToFill.mySpawnForceRandZ = data.mySpawnForceRandZ;
			someDataToFill.mySpawnForceMirrorX = data.mySpawnForceMirrorX;
			someDataToFill.mySpawnForceMirrorY = data.mySpawnForceMirrorY;
			someDataToFill.mySpawnForceMirrorZ = data.mySpawnForceMirrorZ;
			someDataToFill.myRotateRandomRotationX = data.myRotateRandomRotationX;
			someDataToFill.myRotateRandomRotationY = data.myRotateRandomRotationY;
			someDataToFill.myRotateXAxis = data.myRotateXAxis;
			someDataToFill.myRotateYAxis = data.myRotateYAxis;
			someDataToFill.myRotateZAxis = data.myRotateZAxis;
			someDataToFill.myForwardIsDirection = data.myForwardIsDirection;
			someDataToFill.myUseCurvesEmission = data.myUseCurvesEmission;
			someDataToFill.myAmountOfEmissionPoints = data.myAmountOfEmissionPoints;
			for (int i = 0; i < data.myAmountOfEmissionPoints; i++)
			{
				someDataToFill.myEmissionOverTimePoints[i].x = data.myEmissionOverTimePoints[i * 2];
				someDataToFill.myEmissionOverTimePoints[i].y = data.myEmissionOverTimePoints[i * 2 + 1];
			}
			someDataToFill.myEmissiveCurveStrength = data.myEmissiveCurveStrength;
			someDataToFill.myEmissiveEndStrength = data.myEmissiveEndStrength;
			someDataToFill.myUseCurvesSize = data.myUseCurvesSize;
			someDataToFill.myAmountOfSizePoints = data.myAmountOfSizePoints;
			someDataToFill.mySizeCurveStrength = data.mySizeCurveStrength;
			for (int i = 0; i < data.myAmountOfSizePoints; i++)
			{
				someDataToFill.mySizeOverTimePoints[i].x = data.mySizeOverTimePoints[i * 2];
				someDataToFill.mySizeOverTimePoints[i].y = data.mySizeOverTimePoints[i * 2 + 1];
			}
			someDataToFill.myBurstSpawnDelay = data.myBurstSpawnDelay;
			someDataToFill.myMeshName = data.modelName;
			//char matName[128];
			//strcpy_s(matName, data.myMaterialName);
			aMaterialName = data.myMaterialName;
			aMaterialType = (MaterialTypes)data.myMaterialType;
			aGUIDtoFill = data.mySystemUUID;
		}
		if (particleVersionIndex == 6)
		{
			struct ParticleImportStruct
			{
				unsigned int myNumberOfColors{};
				float myColorBlendTimers[5]{ 0 };
				float myParticleColors[20]{ 0 };
				float myForce[3]{ 0 };
				float myDrag[3]{ 0 };
				float myBoxSize[3]{ 0 };
				float myOffset[3]{ 0 };
				float mySpawnRate{};
				float mySpawnAngle{};
				float myParticleSpeed{};
				float myParticleSpawnSize{};
				float myParticleEndSize{};
				float myParticleEmissiveStrength{};
				float myParticleMinLifeTime{};
				float myParticleMaxLifeTime{};
				float myParticleSpawnMinRotationDirection{};
				float myParticleSpawnMaxRotationDirection{};
				float myParticleRotationSpeed{};
				float myMinRotationSpeed{};
				float myMaxRotationSpeed{};
				unsigned int mySpawnParticleWithRandomRotation{};
				unsigned int myRotateRandomRotation{};

				float myBurstLength{};
				float myBurstSpaceTime{};
				unsigned int myBurstMode{};
				unsigned int myIsContinouslyBursting{};
				float myRadius{};

				unsigned int myEmitterShape{};
				unsigned int myNumberOfCharactersInName{};
				char mySystemName[128]{ 0 };
				unsigned int myNumberOfCharactersInMaterialName{};
				char myMaterialName[128]{ 0 };
				unsigned int myMaterialType{};

				GUID mySystemUUID = NIL_UUID;

				bool myIsBillboard = true;
				bool myIsMeshParticle = false;
				GUID myMeshGUID = NIL_UUID;
				unsigned short mySubmeshID = 0;
				float myParticleSpawnMinRotationDirectionXY[2];
				float myParticleSpawnMaxRotationDirectionXY[2];
				bool mySpawnParticleWithRandomRotationX = false;
				bool mySpawnParticleWithRandomRotationY = false;
				float myParticleSpawnSizeXY[2] = { 1, 1 };
				float myParticleEndSizeXY[2] = { 1.5f, 1.5f };
				bool myIsUniformScale = false;
				float mySpawnForceMin[3] = { -10, -10, -10 };
				float mySpawnForceMax[3] = { 10, 10, 10 };
				bool mySpawnForceRandX = true;
				bool mySpawnForceRandY = true;
				bool mySpawnForceRandZ = true;
				bool mySpawnForceMirrorX = true;
				bool mySpawnForceMirrorY = true;
				bool mySpawnForceMirrorZ = true;
				bool myRotateRandomRotationX = false;
				bool myRotateRandomRotationY = false;
				bool myRotateXAxis = false;
				bool myRotateYAxis = false;
				bool myRotateZAxis = true;
				bool myForwardIsDirection = false;
				bool myUseCurvesEmission = false;
				int myAmountOfEmissionPoints = 3;
				float myEmissionOverTimePoints[40];
				float myEmissiveCurveStrength = 1.0;
				float myEmissiveEndStrength = 1.0;
				bool myUseCurvesSize = false;
				int myAmountOfSizePoints = 3;
				float mySizeCurveStrength = 1.0;
				float mySizeOverTimePoints[40];
				float myBurstSpawnDelay = 0.f;
				char modelName[256];
				bool mySpawnForcesNormalized = false;
			}data;
			unsigned int numberOfSubEmitters = 0;
			iMD.read((char*)&data, sizeof(ParticleImportStruct));
			iMD.close();
			someDataToFill.myNumberOfColors = data.myNumberOfColors;
			for (unsigned int i = 0; i < data.myNumberOfColors; i++)
			{
				someDataToFill.myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
				someDataToFill.myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
				someDataToFill.myColorBlendTimers[i] = data.myColorBlendTimers[i];
			}
			someDataToFill.myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
			someDataToFill.myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
			someDataToFill.myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
			someDataToFill.myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

			someDataToFill.mySpawnRate = data.mySpawnRate;
			//someDataToFill.mySpawnAngle = data.mySpawnAngle;
			someDataToFill.myParticleSpeed = data.myParticleSpeed;
			someDataToFill.myParticleSpawnSizeZ = data.myParticleSpawnSize;
			someDataToFill.myParticleEndSizeZ = data.myParticleEndSize;
			someDataToFill.myParticleEmissiveStrength = data.myParticleEmissiveStrength;
			someDataToFill.myParticleMinLifeTime = data.myParticleMinLifeTime;
			someDataToFill.myParticleMaxLifeTime = data.myParticleMaxLifeTime;
			someDataToFill.myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
			someDataToFill.myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
			someDataToFill.myMinRotationSpeed = data.myMinRotationSpeed;
			someDataToFill.myMaxRotationSpeed = data.myMaxRotationSpeed;
			someDataToFill.mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
			someDataToFill.myRotateRandomRotation = data.myRotateRandomRotation;
			someDataToFill.myBurstLength = data.myBurstLength;
			someDataToFill.myBurstSpaceTime = data.myBurstSpaceTime;
			someDataToFill.myBurstMode = (bool)data.myBurstMode;
			someDataToFill.myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
			someDataToFill.myRadius = data.myRadius;
			someDataToFill.myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;
			someDataToFill.myIsBillboard = data.myIsBillboard;
			someDataToFill.myIsMeshParticle = data.myIsMeshParticle;
			someDataToFill.myMeshGUID = data.myMeshGUID;
			someDataToFill.mySubmeshID = data.mySubmeshID;
			someDataToFill.myParticleSpawnMinRotationDirectionXY.x = data.myParticleSpawnMinRotationDirectionXY[0];
			someDataToFill.myParticleSpawnMinRotationDirectionXY.y = data.myParticleSpawnMinRotationDirectionXY[1];
			someDataToFill.myParticleSpawnMaxRotationDirectionXY.x = data.myParticleSpawnMaxRotationDirectionXY[0];
			someDataToFill.myParticleSpawnMaxRotationDirectionXY.y = data.myParticleSpawnMaxRotationDirectionXY[1];
			someDataToFill.mySpawnParticleWithRandomRotationX = data.mySpawnParticleWithRandomRotationX;
			someDataToFill.mySpawnParticleWithRandomRotationY = data.mySpawnParticleWithRandomRotationY;
			someDataToFill.myParticleSpawnSizeXY.x = data.myParticleSpawnSizeXY[0];
			someDataToFill.myParticleSpawnSizeXY.y = data.myParticleSpawnSizeXY[1];
			someDataToFill.myParticleEndSizeXY.x = data.myParticleEndSizeXY[0];
			someDataToFill.myParticleEndSizeXY.y = data.myParticleEndSizeXY[1];
			someDataToFill.myIsUniformScale = data.myIsUniformScale;
			someDataToFill.mySpawnForceMin.x = data.mySpawnForceMin[0];
			someDataToFill.mySpawnForceMin.y = data.mySpawnForceMin[1];
			someDataToFill.mySpawnForceMin.z = data.mySpawnForceMin[2];
			someDataToFill.mySpawnForceMax.x = data.mySpawnForceMax[0];
			someDataToFill.mySpawnForceMax.y = data.mySpawnForceMax[1];
			someDataToFill.mySpawnForceMax.z = data.mySpawnForceMax[2];
			someDataToFill.mySpawnForceRandX = data.mySpawnForceRandX;
			someDataToFill.mySpawnForceRandY = data.mySpawnForceRandY;
			someDataToFill.mySpawnForceRandZ = data.mySpawnForceRandZ;
			someDataToFill.mySpawnForceMirrorX = data.mySpawnForceMirrorX;
			someDataToFill.mySpawnForceMirrorY = data.mySpawnForceMirrorY;
			someDataToFill.mySpawnForceMirrorZ = data.mySpawnForceMirrorZ;
			someDataToFill.myRotateRandomRotationX = data.myRotateRandomRotationX;
			someDataToFill.myRotateRandomRotationY = data.myRotateRandomRotationY;
			someDataToFill.myRotateXAxis = data.myRotateXAxis;
			someDataToFill.myRotateYAxis = data.myRotateYAxis;
			someDataToFill.myRotateZAxis = data.myRotateZAxis;
			someDataToFill.myForwardIsDirection = data.myForwardIsDirection;
			someDataToFill.myUseCurvesEmission = data.myUseCurvesEmission;
			someDataToFill.myAmountOfEmissionPoints = data.myAmountOfEmissionPoints;
			for (int i = 0; i < data.myAmountOfEmissionPoints; i++)
			{
				someDataToFill.myEmissionOverTimePoints[i].x = data.myEmissionOverTimePoints[i * 2];
				someDataToFill.myEmissionOverTimePoints[i].y = data.myEmissionOverTimePoints[i * 2 + 1];
			}
			someDataToFill.myEmissiveCurveStrength = data.myEmissiveCurveStrength;
			someDataToFill.myEmissiveEndStrength = data.myEmissiveEndStrength;
			someDataToFill.myUseCurvesSize = data.myUseCurvesSize;
			someDataToFill.myAmountOfSizePoints = data.myAmountOfSizePoints;
			someDataToFill.mySizeCurveStrength = data.mySizeCurveStrength;
			for (int i = 0; i < data.myAmountOfSizePoints; i++)
			{
				someDataToFill.mySizeOverTimePoints[i].x = data.mySizeOverTimePoints[i * 2];
				someDataToFill.mySizeOverTimePoints[i].y = data.mySizeOverTimePoints[i * 2 + 1];
			}
			someDataToFill.myBurstSpawnDelay = data.myBurstSpawnDelay;
			someDataToFill.myMeshName = data.modelName;
			someDataToFill.mySpawnForcesNormalized = data.mySpawnForcesNormalized;
			//char matName[128];
			//strcpy_s(matName, data.myMaterialName);
			aMaterialName = data.myMaterialName;
			aMaterialType = (MaterialTypes)data.myMaterialType;
			aGUIDtoFill = data.mySystemUUID;
		}
		return true;
	}
	return false;
}

void Engine::ParticleManager::ReleaseAllResources()
{
	mySystems.ForEach([](GUID&, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&)
		{
			for (unsigned short instance = 0; instance < NUMB_PARTICLES_PERSYSTEM; instance++)
			{
				delete[] cmd.myRenderCommands[instance].myVertices;
				cmd.myRenderCommands[instance].myVertices = nullptr;
				cmd.myRenderCommands[instance].myParticleVertexBuffer = nullptr;
				cmd.myRenderCommands[instance].myMaterial = nullptr;
				cmd.myEmissionData[instance].burstTimer = 0;
				cmd.myEmissionData[instance].lifeTime = 0;
				cmd.myEmissionData[instance].prevBurstTimer = 0;
				cmd.myEmissionData[instance].spawnTimer = 0;
				cmd.myRenderCommands[instance].myAmountOfActiveVertices = 0;
			}

			if (cmd.myParticleEffect.GetData().myIsInited)
			{
				SAFE_RELEASE(cmd.myParticleEffect.GetData().myParticleVertexBuffer);
				cmd.myParticleEffect.GetData().myMaterial = nullptr;
				cmd.myParticleEffect.GetData().myIsInited = false;
				cmd.myParticleEffect.GetData().myNumberOfParticles = 0;
			}
			cmd.myReturnedCommands.Clear();
		});
}

void Engine::ParticleManager::ClearAllParticles()
{
	mySystems.ForEach([](GUID&, ParticleSystem& cmd, CU::Dictionary<GUID, ParticleSystem>&)
		{
			for (unsigned short i = 0; i < cmd.myFetchedCommandCount; i++)
			{
				cmd.myEmissionData[i].burstTimer = 0;
				cmd.myEmissionData[i].lifeTime = 0;
				cmd.myEmissionData[i].prevBurstTimer = 0;
				cmd.myEmissionData[i].spawnTimer = 0;
				cmd.myRenderCommands[i].myAmountOfActiveVertices = 0;
			}
			cmd.myFetchedCommandCount = 0;
			cmd.myReturnedCommands.Clear();
		});
}

void Engine::ParticleManager::ReturnParticle(ParticleEmitterComponent& aComponentToReturn)
{
	if (aComponentToReturn.particleSystem == NIL_UUID || aComponentToReturn.renderIndex == _UI16_MAX)
	{
		return;
	}
	if (mySystems[aComponentToReturn.particleSystem]->myFetchedCommandCount == 0)
	{
		return;
	}
	mySystems[aComponentToReturn.particleSystem]->myReturnedCommands.Enqueue(aComponentToReturn.renderIndex);
	ClearEmission(mySystems[aComponentToReturn.particleSystem]->myEmissionData[aComponentToReturn.renderIndex]);
	mySystems[aComponentToReturn.particleSystem]->myFetchedCommandCount--;
	aComponentToReturn.renderIndex = _UI16_MAX;
}
void Engine::ParticleManager::ReturnParticle(ParticleMultiEmitterComponent& aSystemToReturn, unsigned short aSubSystemIndex)
{
	assert(aSubSystemIndex < MAX_COUNTMULTIEMITTER && "Index out of range for multi emitter component");
	ParticleID* systemID = nullptr;
	uint16_t* renderIndex = nullptr;
	switch (aSubSystemIndex)
	{
	case 0:
		systemID = &aSystemToReturn.particleSystem1;
		renderIndex = &aSystemToReturn.renderIndex1;
		break;
	case 1:
		systemID = &aSystemToReturn.particleSystem2;
		renderIndex = &aSystemToReturn.renderIndex2;
		break;
	case 2:
		systemID = &aSystemToReturn.particleSystem3;
		renderIndex = &aSystemToReturn.renderIndex3;
		break;
	default:
		break;
	}
	if (*renderIndex == _UI16_MAX || *systemID == NIL_UUID){return;	}
	ParticleSystem* system = mySystems[*systemID];
	if (system->myFetchedCommandCount == 0){ return; }
	system->myReturnedCommands.Enqueue(*renderIndex);
	ClearEmission(system->myEmissionData[*renderIndex]);
	system->myFetchedCommandCount--;
	*renderIndex = _UI16_MAX;
}
static unsigned int versionNumb = 6;
bool  Engine::ParticleManager::ExportSystem(ParticleSystem* aSystemToTexport)
{
	struct SystemExportStruct
	{
		SystemExportStruct& operator= (const ParticleEffect::ParticleSettings& aSettingToCopy)
		{
			myNumberOfColors = aSettingToCopy.myNumberOfColors;
			for (unsigned int i = 0; i < 5; i++)
			{
				myColorBlendTimers[i] = aSettingToCopy.myColorBlendTimers[i];
			}
			for (unsigned int i = 0; i < 5; i++)
			{
				v4f color = aSettingToCopy.myColorsToBlendBetween[i].GetRGBANormalized();
				myParticleColors[i * 4] = color[0];
				myParticleColors[1 + i * 4] = color[1];
				myParticleColors[2 + i * 4] = color[2];
				myParticleColors[3 + i * 4] = color[3];
			}
			myForce[0] = aSettingToCopy.myForce.x;
			myForce[1] = aSettingToCopy.myForce.y;
			myForce[2] = aSettingToCopy.myForce.z;
			myDrag[0] = aSettingToCopy.myDrag.x;
			myDrag[1] = aSettingToCopy.myDrag.y;
			myDrag[2] = aSettingToCopy.myDrag.z;
			myBoxSize[0] = aSettingToCopy.myBoxSize.x;
			myBoxSize[1] = aSettingToCopy.myBoxSize.y;
			myBoxSize[2] = aSettingToCopy.myBoxSize.z;
			myOffset[0] = aSettingToCopy.myOffSetAsSubSystem.x;
			myOffset[1] = aSettingToCopy.myOffSetAsSubSystem.y;
			myOffset[2] = aSettingToCopy.myOffSetAsSubSystem.z;

			mySpawnRate = aSettingToCopy.mySpawnRate;
			//mySpawnAngle = aSettingToCopy.mySpawnAngle;
			myParticleSpeed = aSettingToCopy.myParticleSpeed;
			myParticleSpawnSize = aSettingToCopy.myParticleSpawnSizeZ;
			myParticleEndSize = aSettingToCopy.myParticleEndSizeZ;
			myParticleEmissiveStrength = aSettingToCopy.myParticleEmissiveStrength;
			myParticleMinLifeTime = aSettingToCopy.myParticleMinLifeTime;
			myParticleMaxLifeTime = aSettingToCopy.myParticleMaxLifeTime;
			myParticleSpawnMinRotationDirection = aSettingToCopy.myParticleSpawnMinRotationDirectionZ;
			myParticleSpawnMaxRotationDirection = aSettingToCopy.myParticleSpawnMaxRotationDirectionZ;
			myMinRotationSpeed = aSettingToCopy.myMinRotationSpeed;
			myMaxRotationSpeed = aSettingToCopy.myMaxRotationSpeed;
			mySpawnParticleWithRandomRotation = (unsigned int)aSettingToCopy.mySpawnParticleWithRandomRotationZ;
			myRotateRandomRotation = (unsigned int)aSettingToCopy.myRotateRandomRotation;

			myBurstLength = aSettingToCopy.myBurstLength;
			myBurstSpaceTime = aSettingToCopy.myBurstSpaceTime;
			myBurstMode = (unsigned int)aSettingToCopy.myBurstMode;
			myIsContinouslyBursting = (unsigned int)aSettingToCopy.myIsContinouslyBursting;
			myRadius = aSettingToCopy.myRadius;

			myEmitterShape = (unsigned int)aSettingToCopy.myEmitterShape;

			myIsBillboard = aSettingToCopy.myIsBillboard;
			myIsMeshParticle = aSettingToCopy.myIsMeshParticle;
			myMeshGUID = aSettingToCopy.myMeshGUID;
			mySubmeshID = aSettingToCopy.mySubmeshID;
			myParticleSpawnMinRotationDirectionXY[0] = aSettingToCopy.myParticleSpawnMinRotationDirectionXY.x;
			myParticleSpawnMinRotationDirectionXY[1] = aSettingToCopy.myParticleSpawnMinRotationDirectionXY.y;
			myParticleSpawnMaxRotationDirectionXY[0] = aSettingToCopy.myParticleSpawnMaxRotationDirectionXY.x;
			myParticleSpawnMaxRotationDirectionXY[1] = aSettingToCopy.myParticleSpawnMaxRotationDirectionXY.y;
			mySpawnParticleWithRandomRotationX = aSettingToCopy.mySpawnParticleWithRandomRotationX;
			mySpawnParticleWithRandomRotationY = aSettingToCopy.mySpawnParticleWithRandomRotationY;
			myParticleSpawnSizeXY[0] = aSettingToCopy.myParticleSpawnSizeXY.x;
			myParticleSpawnSizeXY[1] = aSettingToCopy.myParticleSpawnSizeXY.y;
			myParticleEndSizeXY[0] = aSettingToCopy.myParticleEndSizeXY.x;
			myParticleEndSizeXY[1] = aSettingToCopy.myParticleEndSizeXY.y;
			myIsUniformScale = aSettingToCopy.myIsUniformScale;
			mySpawnForceMin[0] = aSettingToCopy.mySpawnForceMin.x;
			mySpawnForceMin[1] = aSettingToCopy.mySpawnForceMin.y;
			mySpawnForceMin[2] = aSettingToCopy.mySpawnForceMin.z;
			mySpawnForceMax[0] = aSettingToCopy.mySpawnForceMax.x;
			mySpawnForceMax[1] = aSettingToCopy.mySpawnForceMax.y;
			mySpawnForceMax[2] = aSettingToCopy.mySpawnForceMax.z;
			mySpawnForceRandX = aSettingToCopy.mySpawnForceRandX;
			mySpawnForceRandY = aSettingToCopy.mySpawnForceRandY;
			mySpawnForceRandZ = aSettingToCopy.mySpawnForceRandZ;
			mySpawnForceMirrorX = aSettingToCopy.mySpawnForceMirrorX;
			mySpawnForceMirrorY = aSettingToCopy.mySpawnForceMirrorY;
			mySpawnForceMirrorZ = aSettingToCopy.mySpawnForceMirrorZ;
			myRotateRandomRotationX = aSettingToCopy.myRotateRandomRotationX;
			myRotateRandomRotationY = aSettingToCopy.myRotateRandomRotationY;
			myRotateXAxis = aSettingToCopy.myRotateXAxis;
			myRotateYAxis = aSettingToCopy.myRotateYAxis;
			myRotateZAxis = aSettingToCopy.myRotateZAxis;
			myForwardIsDirection = aSettingToCopy.myForwardIsDirection;
			myUseCurvesEmission = aSettingToCopy.myUseCurvesEmission;
			myAmountOfEmissionPoints = aSettingToCopy.myAmountOfEmissionPoints;
			for (int i = 0; i < myAmountOfEmissionPoints; i++)
			{
				myEmissionOverTimePoints[i * 2] = aSettingToCopy.myEmissionOverTimePoints[i].x;
				myEmissionOverTimePoints[i * 2 + 1] = aSettingToCopy.myEmissionOverTimePoints[i].y;
			}
			myEmissiveCurveStrength = aSettingToCopy.myEmissiveCurveStrength;
			myEmissiveEndStrength = aSettingToCopy.myEmissiveEndStrength;
			myUseCurvesSize = aSettingToCopy.myUseCurvesSize;
			myAmountOfSizePoints = aSettingToCopy.myAmountOfSizePoints;
			for (int i = 0; i < myAmountOfSizePoints; i++)
			{
				mySizeOverTimePoints[i * 2] = aSettingToCopy.mySizeOverTimePoints[i].x;
				mySizeOverTimePoints[i * 2 + 1] = aSettingToCopy.mySizeOverTimePoints[i].y;
			}
			mySizeCurveStrength = aSettingToCopy.mySizeCurveStrength;
			myBurstSpawnDelay = aSettingToCopy.myBurstSpawnDelay;
			strcpy_s(modelname, aSettingToCopy.myMeshName);
			mySpawnForcesNormalized = aSettingToCopy.mySpawnForcesNormalized;
			return(*this);
		}
		unsigned int myNumberOfColors{};
		float myColorBlendTimers[5]{ 0 };
		float myParticleColors[20]{ 0 };
		float myForce[3]{ 0 };
		float myDrag[3]{ 0 };
		float myBoxSize[3]{ 0 };
		float myOffset[3]{ 0 };
		float mySpawnRate{};
		float mySpawnAngle{};
		float myParticleSpeed{};
		float myParticleSpawnSize{};
		float myParticleEndSize{};
		float myParticleEmissiveStrength{};
		float myParticleMinLifeTime{};
		float myParticleMaxLifeTime{};
		float myParticleSpawnMinRotationDirection{};
		float myParticleSpawnMaxRotationDirection{};
		float myParticleRotationSpeed{};
		float myMinRotationSpeed{};
		float myMaxRotationSpeed{};
		unsigned int mySpawnParticleWithRandomRotation{};
		unsigned int myRotateRandomRotation{};

		float myBurstLength{};
		float myBurstSpaceTime{};
		unsigned int myBurstMode{};
		unsigned int myIsContinouslyBursting{};
		float myRadius{};

		unsigned int myEmitterShape{};
		unsigned int myNumberOfCharactersInName{};
		char mySystemName[128]{ 0 };
		unsigned int myNumberOfCharactersInMaterialName{};
		char myMaterialName[128]{ 0 };
		unsigned int myMaterialType{};

		GUID systemID = NIL_UUID;

		bool myIsBillboard = true;
		bool myIsMeshParticle = false;
		GUID myMeshGUID = NIL_UUID;
		unsigned short mySubmeshID = 0;
		float myParticleSpawnMinRotationDirectionXY[2];
		float myParticleSpawnMaxRotationDirectionXY[2];
		bool mySpawnParticleWithRandomRotationX = false;
		bool mySpawnParticleWithRandomRotationY = false;
		float myParticleSpawnSizeXY[2] = { 1, 1 };
		float myParticleEndSizeXY[2] = { 1.5f, 1.5f };
		bool myIsUniformScale = false;
		float mySpawnForceMin[3] = { -10, -10, -10 };
		float mySpawnForceMax[3] = { 10, 10, 10 };
		bool mySpawnForceRandX = true;
		bool mySpawnForceRandY = true;
		bool mySpawnForceRandZ = true;
		bool mySpawnForceMirrorX = true;
		bool mySpawnForceMirrorY = true;
		bool mySpawnForceMirrorZ = true;
		bool myRotateRandomRotationX = false;
		bool myRotateRandomRotationY = false;
		bool myRotateXAxis = false;
		bool myRotateYAxis = false;
		bool myRotateZAxis = true;
		bool myForwardIsDirection = false;
		bool myUseCurvesEmission = false;
		int myAmountOfEmissionPoints = 3;
		float myEmissionOverTimePoints[40];
		float myEmissiveCurveStrength = 1.0;
		float myEmissiveEndStrength = 1.0;
		bool myUseCurvesSize = false;
		int myAmountOfSizePoints = 3;
		float mySizeCurveStrength = 1.0;
		float mySizeOverTimePoints[40];
		float myBurstSpawnDelay = 0.f;
		char modelname[256];
		bool mySpawnForcesNormalized = false;
	} exportStruct;
	ParticleSystem* system = aSystemToTexport;
	ParticleEffect::ParticleSettings emitterSettings = system->myParticleEffect.GetSettings();
	ParticleEffect::ParticleEmitterData emitterData = system->myParticleEffect.GetData();


	exportStruct = SystemExportStruct();
	std::string outpath = "Content/ParticleSystems/";
	std::filesystem::create_directories(outpath);
	std::string name = system->myGratKey.Data();
	std::string systemPath = "Content/ParticleSystems/";
	bool foundSystem = false;
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(systemPath))
	{
		std::string filename = dirEntry.path().filename().replace_extension().string();
		if (dirEntry.is_regular_file() && strcmp(filename.c_str(), system->myGratKey.Data()) == 0)
		{
			outpath = dirEntry.path().relative_path().string().c_str();
			foundSystem = true;
			break;
		}
	}
	if (foundSystem)
	{
		if (std::filesystem::exists(outpath.c_str()))
		{
			std::filesystem::permissions(outpath.c_str(), std::filesystem::perms::all);
			std::filesystem::remove(outpath.c_str());
		}
	}
	else
	{
		outpath = systemPath + name + ".gratsprut";
	}

	std::ofstream outMaterial;
	outMaterial.open(outpath, std::ios::out | std::ios::binary);

	unsigned int materialVersionIndex = versionNumb;
	outMaterial.write((char*)&materialVersionIndex, sizeof(unsigned int));

	exportStruct = emitterSettings;
	exportStruct.systemID = system->mySystemIndex;
	memcpy(&exportStruct.myOffset[0], &emitterSettings.myOffSetAsSubSystem, sizeof(v3f));
	exportStruct.myNumberOfCharactersInName = 0;

	for (unsigned short c = 0; c < 128; c++)
	{
		if (c < strlen(system->myGratKey.Data()))
		{
			exportStruct.myNumberOfCharactersInName += 1;
			exportStruct.mySystemName[c] = system->myGratKey[c];
		}
		else
		{
			exportStruct.mySystemName[c] = 0;
		}
	}
	exportStruct.myNumberOfCharactersInMaterialName = 0;
	for (unsigned short c = 0; c < 128; c++)
	{
		if (c < strlen(emitterData.myMaterial->myMaterialName.Data()))
		{
			exportStruct.myNumberOfCharactersInMaterialName += 1;
			exportStruct.myMaterialName[c] = emitterData.myMaterial->myMaterialName[c];
		}
		else
		{
			exportStruct.myMaterialName[c] = 0;
		}
	}
	exportStruct.myMaterialType = (unsigned int)emitterData.myMaterial->myMaterialType;
	if (exportStruct.systemID == NIL_UUID)
	{
		UuidCreate(&exportStruct.systemID);
	}
	outMaterial.write((char*)&exportStruct, sizeof(SystemExportStruct));
	outMaterial.close();

	return true;
}

void Engine::ParticleManager::ClearEmission(ParticleEmissionData& someDataToClear)
{
	someDataToClear.burstTimer = 0;
	someDataToClear.lifeTime = 0;
	someDataToClear.prevBurstTimer = 0;
	someDataToClear.spawnTimer = 0;
}

void Engine::ParticleManager::InitMeshData(ParticleSystem* aSystem, unsigned short aNumberOfParticles)
{
	ModelManager* mm = EngineInterface::GetModelManager();
	Model* model = mm->GetModel(aSystem->myParticleEffect.GetSettings().myMeshGUID);
	if (model == nullptr)
	{
		FixedString256 name = FixedString256::Format("%s.grat", aSystem->myParticleEffect.GetSettings().myMeshName.Data());
		mm->LoadGratModel(aSystem->myParticleEffect.GetSettings().myMeshName.Data(), true);
		model = mm->GetModel(aSystem->myParticleEffect.GetSettings().myMeshGUID);
	}
	mm->IncrementModelCounter(aSystem->myParticleEffect.GetSettings().myMeshGUID);
	aSystem->myMeshRenderC.model = &model->GetModelData(aSystem->myParticleEffect.GetSettings().mySubmeshID);
	aSystem->myMeshRenderC.modelTransforms = new m4f[aNumberOfParticles * NUMB_PARTICLES_PERSYSTEM];
	aSystem->myMeshRenderC.modelsColliders = new CU::AABB3Df[aNumberOfParticles * NUMB_PARTICLES_PERSYSTEM];
	aSystem->myMeshRenderC.modelsEffectData = new ObjectEffectData[aNumberOfParticles * NUMB_PARTICLES_PERSYSTEM];
	aSystem->myMeshRenderC.numberOfModels = aNumberOfParticles * NUMB_PARTICLES_PERSYSTEM;
	if (aNumberOfParticles * NUMB_PARTICLES_PERSYSTEM > myMaxNumberOfMeshes)
	{
		myMaxNumberOfMeshes = aNumberOfParticles * NUMB_PARTICLES_PERSYSTEM;
		ReInitSRVs(); 
	}
	//aSystem->modelType = model
}

void Engine::ParticleManager::ReInitSRVs()
{
	if (myModelOBToWorldSRV)
	{
		SAFE_RELEASE(myModelOBToWorldSRV);
		SAFE_RELEASE(myModelOBToWorldBuffer);
	}
	if (myModelEffectSRV)
	{
		SAFE_RELEASE(myModelEffectSRV);
		SAFE_RELEASE(myModelEffectBuffer);
	}
	DX::CreateStructuredBuffer(myDevice, &myModelOBToWorldSRV, &myModelOBToWorldBuffer, myMaxNumberOfMeshes, sizeof(m4f));
	DX::CreateStructuredBuffer(myDevice, &myModelEffectSRV, &myModelEffectBuffer, myMaxNumberOfMeshes, sizeof(ObjectEffectData));
}

