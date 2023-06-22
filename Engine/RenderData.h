#pragma once

#include "../Engine/ECS/Systems/RenderCommands.h"
#include "../Engine/GameObjects/Sprite.h"

#include <atomic>

#include "../Engine/RenderConstants.hpp"
#include "..\CommonUtilities\CU\Containers\BitFlagpole.hpp"

#include "ECS\SerializedEnums.hpp"

struct RenderTargetComponent;


namespace Engine
{
	class FullScreenTexture;
	class GBuffer;
	class ModelManager;
	constexpr size_t NUMB_DEBUGSPHERES = 1024U;
#ifndef _DISTRIBUTION
	struct DebugSphereCommand
	{
		CU::Color mySphereColor;
		v3f myPosition;
		float myRadius;
	};
#endif
	enum class ELoadPackageTypes
	{
		LevelLightData,
		AddLevelLightData,
		BakeReflectionProbes,
		ReflectionProbe,
		PlayerPortraits,
		PresetIcon,
		AccessoryIcon,
		COUNT
	};
	enum class ECameraFlags
	{
		none = 0,
		camera1 = PO2(0),
		camera2 = PO2(1),
		camera3 = PO2(2),
		camera4 = PO2(3),
		camera5 = PO2(4),
		camera6 = PO2(5),
		camera7 = PO2(6),
		camera8 = PO2(7)
	};
	struct RenderTarget
	{
		FullScreenTexture** texture = nullptr;
		FullScreenTexture** depthTexture = nullptr;
		FullScreenTexture** intermediateTexture = nullptr; //PostProcessing texture
		GBuffer** gBufferTexture = nullptr;
		Camera camera;
		RenderFlag renderFlag;
	};

	struct RenderData
	{
		ShortString name;
		//The render data for a frame
		PointLightRenderCommand pointLights[MAX_POINT_LIGHTS];
		uint16_t pointLightIDs[MAX_POINT_LIGHTS];
		uint16_t pointlightShadowCaster[MAX_POINT_LIGHTS];
		uint16_t pointLightsSize = 0;
		SpotLightRenderCommand spotLights[MAX_SPOT_LIGHTS];
		uint16_t  spotlightIDs[MAX_SPOT_LIGHTS];
		uint16_t  spotlightsShadowCaster[MAX_SPOT_LIGHTS];
		uint16_t spotLightsSize = 0;
		SpriteCommand sprites[MAX_SPRITES];
		size_t spritesSize = 0;
		WorldSpriteCommand wsprites[MAX_WSPRITES];
		size_t wspritesSize = 0;
		ParticleCommand particles[MAX_PARTICLES];
		size_t particlesSize = 0;
		DecalCommand decals[MAX_DECALS];
		size_t decalsSize = 0;
		SortedModelDataForBuffers sortedMeshes[MAX_SORTED_MESHES];
		size_t sortedMeshesSize = 0;
		MeshBuffererCommand uniqueMeshes[MAX_UNIQUE_MESHES];
		size_t uniqueMeshesSize = 0;
		SortedAnimationDataForBuffers sortedAnimMeshes[MAX_ANIM_SORTED_MESHES];
		size_t sortedAnimMeshesSize = 0;
		AnimatedMeshRenderCommand uniqueAnimatedMeshes[MAX_ANIM_UNIQUE_MESHES];
		size_t uniqueAnimatedMeshesSize = 0;

		Camera camera;
		RenderFlag mainRenderFlag = RenderFlag::RenderFlag_AllPasses;
		DirectionalLight_Data dirLight;
		CU::Color ambience = {0, 0, 0, 0};
		CU::Color ambienceFogColor = { 0, 0, 0, 0 };
		float fogExponent = 1;
		float fogNearDistance = 100;
		float fogFarDistance = 1000;
		float DoF_FocusDistance = 750.f;
		v3f mapMiddle;
		v3f mapHalfSize;
		bool resolutionChanged = false;
		bool renderNormalView = true; // used for excluding the main renderering texture in the modular rendering process
		bool loadedLevel = false;
		bool cursorAdded = false;


		RenderTarget renderTargetCameras[NUMBOF_RENDERTARGETS];
		CU::BitFlagpole8  renderTargetcameraFlags = CU::BitFlagpole8(ECameraFlags::camera8);
		//debug lines and spheres
	#ifndef _DISTRIBUTION
		Vertex_Debug_Line debugLines[50000];
		size_t debugLinesSize = 0;

		DebugSphereCommand debugSpheres[NUMB_DEBUGSPHERES];
		size_t debugSpheresSize = 0;
	#endif
		//data packages to be able to send data over the threads safely
		void* dataPackages[NUMBOF_MAXAMOUNTOFRRENDERDATAPACKAGES] = { nullptr };
		size_t numberOfDataPackages = 0;
		ELoadPackageTypes dataPackagesType[NUMBOF_MAXAMOUNTOFRRENDERDATAPACKAGES];
	};

	struct GlobalRenderData
	{
		/* General */
		std::atomic_flag renderThreadIsCopying = ATOMIC_FLAG_INIT;

		/* Gameplay Thread */
		std::atomic_uint64_t step = 0U;
		enum GameplayFlags_
		{
			GameplayFlags_None = 1 >> 1,
			GameplayFlags_ModelBuffersAreAdjusted = 1 << 0,
		};
		std::atomic_int32_t gameplayFlags = GameplayFlags_ModelBuffersAreAdjusted;
		std::atomic<double> frameTimer = 0.0;

		/* Rendering Thread */
		std::atomic_uint64_t frame = 0U;
		std::atomic_uint64_t currentRenderStep = 0U;

		size_t bufferLastReadIndex = 0;
		size_t bufferReadIndex = 1;
		size_t bufferWriteIndex = 2;

		RenderData renderBuffersData[3];

	};

	extern GlobalRenderData globalRenderData;

	RenderData* GetWriteGameplayBuffer();
	RenderData* GetReadBuffer();
	RenderData* GetLastReadBuffer();

	void RenderMesh(MeshBuffererCommand cmd, ModelManager* aModelManager, bool isEnv = false, RenderTargetComponent* aRenderTargetComponent = nullptr);
	void RenderAnimatedModel(AnimatedMeshRenderCommand aCommand);
	void RenderPointLight(PointLightRenderCommand aLightCommand, const uint16_t aLightRenderState, uint16_t aID);
	void RenderSpotLight(SpotLightRenderCommand aLightCommand, const uint16_t aLightRenderState, uint16_t aID);
	void RenderSprite(SpriteCommand aSpriteToRender);
	void RenderWorldSprite(WorldSpriteCommandID aWorldSpriteToRender);
	void RenderParticle(ParticleCommand aParticleCommand);
	void RenderDecal(DecalCommand aDecalToRender);
	void UpdateRenderTargetCameras();
	void AddDataPackageToRenderer(ELoadPackageTypes aType, void* data);

	void SortMeshBuffer();
	bool SwapAndClearGameplayBuffers();
}