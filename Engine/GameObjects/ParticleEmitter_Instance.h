#pragma once
#include "ParticleEffectData.h"
#include "..\CommonUtilities\CU\Math\Matrix4x4.hpp"
#include "Vertex.h"
#include "../ECS/Systems/RenderCommands.h"
struct ID3D11Device;
namespace Engine
{
	class TextureManager;
	class ParticleManager;
}



class ParticleEmitter_Instance
{
//public:
//	ParticleEmitter_Instance() = default;
//	bool Init(ParticleEmitter* aEmitter, ID3D11Device* aDevice);
//	bool Init(ParticleEmitter* aEmitter, ID3D11Device* aDevice, unsigned int aNumberOfVertices);
//
//	inline ParticleEmitter* GetEmitterInstance() { return myEmitter; }
//	inline const Vertex_Particle* GetVertices() const { return myRenderCommand.myVertices; }
//	inline const unsigned int GetAmountOfActiveVertices() const { return myRenderCommand.myAmountOfActiveVertices; }
//	inline const unsigned int GetSubEmittersCount() const { return myNumberOfSubEmitters; }
//	const v3f GetOffset() { return myOffset; }
//	void ResetSystem();
//	void SetBoxSize(v3f aBoxSize);
//	void SetEmissionRadius(float aRadius);
//	void Update(const float aDeltaTime, const m4f& aComponentTransform);
//	void UpdateDepthFromCamera(const v3f aCameraPosition);
//	void SetTransform(const m4f& aTransform);
//	void SetOffset(const v3f& anOffset) { myOffset = anOffset; }
//	inline const m4f& GetMatrix() { return myRenderCommand.myTransform; }
//	const bool GetIsPlaying() const { return myIsPlaying; }
//	void SetIsPlaying(const bool aIsPlaying) { myIsPlaying = aIsPlaying; }
//	void PlayBurst();
//	const float GetLifeTime() { return myLifeTime; }
//	void AddSubEmitter(ParticleCommand aEmitterToAdd);
//	void RemoveSubEmitter(unsigned int aIndex);
//	void RemoveSubEmitters();
//	const ParticleCommand GetSubEmitters(const unsigned int aIndex);
//	void ReleaseAllResources();
//	const MaterialTypes GetMaterialType() const { return myMaterialType; }
//	void SetMaterialType(MaterialTypes aType)  {  myMaterialType = aType; }
//	ParticleRenderCommand& GetRenderCommand() { myRenderCommand.myMaterial = myEmitter->GetData().myMaterial; return myRenderCommand; }
//private:
//	void SpawnParticle(const m4f& aComponentTransform);
//	friend class Engine::ParticleManager;
//	ParticleEmitter* myEmitter = nullptr;
//
//	unsigned int myNumberOfSubEmitters = 0;
//	ParticleCommand mySubEmitters[4];
//
//	v3f myBoxSize = {100, 100, 100};
//	v3f myOffset;
//	float mySphereRadius = 100;
//	float myLifeTime{};
//	float mySpawnTimer{};
//
//	float myBurstTimer{};
//	float myPrevBurstTimer{};
//	
//	bool myIsPlaying = true;
//	bool myIsInited = false;
//	bool myShouldSpawn = true;
//	MaterialTypes myMaterialType;
//	ParticleRenderCommand myRenderCommand;
};

