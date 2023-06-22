#pragma once
#include "../Engine/RenderConstants.hpp"
#include "../Engine/Includes.h"
#include "../CommonUtilities/CU/Math/Color.hpp"
#include "../CommonUtilities/CU/Math/Vector3.hpp"
#include "../../GameObjects/Lights.h"
#include "../../GameObjects/ModelData.h"
#include "../CommonUtilities/CU/Containers/VectorOnStack.h"
#include "../CommonUtilities/CU/Containers/GrowingArray.hpp"

class Model;
class ModelAnimated;
class Model_Particle;

struct ObjectEffectData
{
	CU::Color effectColor = {0, 0, 0, 255};
	CU::Color outlineColor = {0, 0, 0, 255};
	unsigned int gBufferPSEffectIndex = 0;
	unsigned int gBufferVSEffectIndex = 0;
	unsigned int modelIndex = 0;
	float tValue = 0;
};

namespace Engine
{
	class ParticleManager;
}
namespace MV
{
	class MV_ParticleEditor;
}
struct MeshBuffererCommand
{
	int modelType = EModelType_STATIC;
	ModelID model = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	MaterialID customMat = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	CU::Matrix4x4f matrix;
	CU::Transform transform;
	CU::AABB3Df myCollider;
	bool renderUnique = false;
	float padding;
	ObjectEffectData effectData;
};

struct MeshRenderCommand
{
	int modelType = EModelType_STATIC;
	Model* model = nullptr;
	CU::Matrix4x4f matrix;
	CU::AABB3Df myCollider;
	ObjectEffectData effectData;
};

struct MeshParticleRenderCommand
{
	Model_Particle* model = nullptr;
	CU::Matrix4x4f matrix;
	CU::Matrix4x4f MVP_matrix;
};

struct AnimatedMeshRenderCommand
{
	m4f boneTransformsFinal[MAX_BONECOUNT];
	CU::Matrix4x4f matrix;
	CU::Transform transform;
	CU::AABB3Df collider;
	ObjectEffectData effectData;
	ModelAnimated *model = nullptr;
	int modelType = EAnimatedModelType_NORMAL;
	unsigned short numberOfBones;
	bool renderUnique = false;
};


struct ParticleRenderCommand
{
	m4f myTransform;
	unsigned int myNumberOfParticles = 0;
	unsigned int myAmountOfActiveVertices = 0;
	unsigned int myStride = 0;
	unsigned int myOffset = 0;
	Material* myMaterial = nullptr;
	ID3D11Buffer* myParticleVertexBuffer = nullptr;
	Vertex_Particle* myVertices = nullptr;
	CU::AABB3Df myParticlesBound;
	bool myIsMeshParticle = false;
	GUID mySystemIndex;
};

struct ID3D11ShaderResourceView;
struct ID3D11Buffer;

struct ParticleMeshRenderCommand
{
	int modelType = 0;
	unsigned short numberOfModels = 0;
	ModelData* model = nullptr;
	m4f* modelTransforms;
	CU::AABB3Df* modelsColliders;
	ObjectEffectData* modelsEffectData;

	ID3D11ShaderResourceView* modelOBToWorldSRV = nullptr;
	ID3D11Buffer* modelOBToWorldBuffer = nullptr;
	ID3D11ShaderResourceView* modelEffectSRV = nullptr;
	ID3D11Buffer* modelEffectBuffer = nullptr;
};



struct SortedAnimationDataForBuffers
{
	SortedAnimationDataForBuffers()
	{
		//boneTransforms = new m4f[NUMB_ANIMMODELSPERTYPE * 128];
		model = nullptr;
		modelType = EAnimatedModelType_NORMAL;
		numberOfBones = 0;
	}
	~SortedAnimationDataForBuffers()
	{
		//delete[]boneTransforms;
	}

	int modelType;
	unsigned short numberOfModels = 0;
	AnimatedModelData* model;
	m4f transforms[NUMB_ANIMMODELSPERTYPE];
	CU::Transform gameTransform[NUMB_MODELSPERTYPE];
	CU::AABB3Df colliders[NUMB_ANIMMODELSPERTYPE];
	ObjectEffectData effectData[NUMB_ANIMMODELSPERTYPE];

	unsigned int numberOfBones;
	m4f boneTransforms[NUMB_ANIMMODELSPERTYPE * MAX_BONECOUNT];// = nullptr;
};

struct SortedModelDataForBuffers
{
	ModelID model = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	int modelType;
	unsigned short numberOfModels = 0;
	unsigned short submodelIndex = 0;
	m4f transforms[NUMB_MODELSPERTYPE];
	CU::AABB3Df colliders[NUMB_MODELSPERTYPE];
	CU::Transform gameTransform[NUMB_MODELSPERTYPE];
	ObjectEffectData effectData[NUMB_MODELSPERTYPE];
};

struct SortedModelDataForRendering
{
	int modelType;
	unsigned short numberOfModels = 0;
	ModelData* model = nullptr;
	m4f transforms[NUMB_MODELSPERTYPE];
	CU::AABB3Df colliders[NUMB_MODELSPERTYPE];
	CU::Transform gameTransform[NUMB_MODELSPERTYPE];
	ObjectEffectData effectData[NUMB_MODELSPERTYPE];
};

struct ParticleBuffer
{
	ParticleRenderCommand* buffer = nullptr;
	unsigned int myNumberOfSystems = 0;
};
struct ParticleCommand
{
	CU::Matrix4x4f myMatrix;
	CU::Transform myTransform;
	GUID mySystemIndex;
	unsigned short myEmitterIndex;
	bool shouldSpawn = true;
};
struct ParticleEmissionData
{
	float lifeTime = 0;
	float spawnTimer = 0;
	float burstTimer = 0;
	float prevBurstTimer = 0;
};

struct DecalCommand
{
	m4f matrix;
	CU::Transform gameTransform;
	Material* material = nullptr;
};

struct SH
{
	float bands[9][4];
};
struct SHGridData
{
	m4f gridRotation;
	v3f gridHalfSize;
	float brightness = 1.0f;
	float gridSpacing = 100.f;
	unsigned int numberOfHarmonics = 0;
	unsigned int globalOffset = 0;
	unsigned int padding = 0;
};

struct EditorRenderCamera
{
	CU::Transform transform;
	float fov = 90;
	bool activated = false;
	bool perspectiveProjection = true;
	float nearClipDistance = 20;
	float farClipDistance = 2500;
	Entity ent = INVALID_ENTITY;
	FrustumDrawData frustumData;
	v2ui renderResolution;
};

struct EditorLightProbeGrid
{
	CU::Transform transform;
	float gridDensity = 400.0f;
	float brightness = 1.0f;
	Entity ent = INVALID_ENTITY;
	bool renderDebugGrid = false;
};

struct EditorReflectionProbe
{
	CU::Transform transform;
	float outerRadius = 1000.f;
	float innerRadius = 200.f;
	float brightness = 1.0f;
	Entity ent = INVALID_ENTITY;
};

struct ReflectionProbesData
{
	unsigned int  numberOfRProbes = 0;
	EditorReflectionProbe probes[NUMBOF_REFLECTIONPROBES];
};
constexpr unsigned int  LIGHTDATA_VERSIONNUMB = 3;
struct LevelLightData
{
	size_t versionNumb = LIGHTDATA_VERSIONNUMB;
	unsigned int numberOfGridsInLevel = 0;
	EditorLightProbeGrid editorGridData[NUMBOF_SHGRIDS];
	SHGridData gridData[NUMBOF_SHGRIDS];
	unsigned int totalNumberOfHarmonics = 0;
	ReflectionProbesData rProbes;
	SH* SHData = nullptr;
};
struct ReflectionProbe
{
	v3f position;
	float probePadding1 = 0;
	float outerRadius = 1000.f;
	float innerRadius = 200.f;
	float brightness = 1.0f;
	float probePadding2 = 0;
	SH irradianceLight;
};

struct PlayerPortraitPackage
{
	size_t numberOfMeshes = 0;
	GUID meshes[2];
	unsigned char playerID;
	CU::Color playerColor;
};

struct AccessoryPackage
{
	GUID mesh;
	unsigned char id;
};
struct PresetPackage
{
	GUID mesh;
	unsigned char id;
};

struct PostProcessingData
{
	float	SSAO_scale = 0.5f;
	float	SSAO_bias = 1.0f;
	float	VIGNETTE_strength = 10.f;
	float	VIGNETTE_extent = 1.0f;

	float	CA_Strength = 1.0f;
	float	BLOOM_strength = 2.0f;
	float	FOG_nearDistance = 1000.0f;
	float	FOG_farDistance = 5000.0f;

	v3f		FOG_color = { 0.18f, 0.175f, 0.285f };
	float	FOG_exponent = 0.5f;
	v3f		colorFilter = { 1, 1, 1 };
	float	postExposure = 1.0f;
	v3f		splitToneShadowTint = { 0.5f, 0.5f, 0.5f };
	float	contrast = 0;
	v3f		splitToneHighLightTint = { 0.5f, 0.5f, 0.5f };
	float	saturation = 0;
	v3f		channelMixerR = { 1, 0, 0 };
	float	splitToningBalance = 0;
	v3f		channelMixerG = { 0, 1, 0 };
	float	hueShift = 0;
	v3f		channelMixerB = { 0, 0, 1 };
	float	SMH_shadowStart = 0;
	v3f		SMH_shadows = { 1, 1, 1 };
	float	SMH_shadowEnd = 0.3f;
	v3f		SMH_midtones = { 1, 1, 1 };
	float	SMH_highlightStart = 0.55f;
	v3f		SMH_highlights = { 1, 1, 1 };
	float	SMH_highlightEnd = 1.f;
};
struct LevelLightExportData
{
	LevelLightData light;
	PostProcessingData post;
};