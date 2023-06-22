#pragma once
#include "../../CommonUtilities/CU/Math/Matrix4x4.hpp"
#include "../../CommonUtilities/CU/Math/Transform.hpp"
#include "../../CommonUtilities/CU\Containers\Queue.hpp"
#include "../../CommonUtilities/CU\Containers\MinHeap.hpp"
#include "../../CommonUtilities/CU\Containers\MaxHeap.hpp"
#include "../../CommonUtilities/CU/Math/Vector3.hpp"
#include "../../CommonUtilities/CU/Math/Color.hpp"
#include "../../CommonUtilities/CU/Collision/AABB3D.hpp"
#include "../../CommonUtilities/CU/Collision/AABB2D.h"
#include "../../CommonUtilities/CU/Collision/OOBB3D.h"
#include "../../CommonUtilities/CU/Collision/Sphere.hpp"
#include "../../CommonUtilities/CU/Utility/ShortString.h"
#include <functional>
#include "../ECS/ECS.h"
#include "../GameObjects/Lights.h"
#include "../GameObjects/Model.h"
#include "../Engine/ECS/Systems/RenderCommands.h"
#include "../Engine/Managers/TextureManager.h"
#include "../../Engine/EngineInterface.h"
#include "../Externals/physx/PxActor.h"
#include "../Engine/RenderConstants.hpp"

#include "../Engine/ECS/Level/StructExporter.h"
#include "../Engine/BehaviorTree.h"
#include "../Engine/SteeringData.hpp"

#include "../Engine/ECS/SerializedEnums.hpp"

constexpr size_t MAX_COMPONENTS = 64U;

SERIALIZE_STRUCT(TransformComponent,
	struct TransformComponent
{
	CU::Transform transform;
	CU::AABB2D boundingBox;

	NO_SERIALIZE_IN_EDITOR;
	m4f matrix;
	v3f rotation;
};
);

SERIALIZE_STRUCT(MeshComponent,
	struct MeshComponent
{
	ModelID model;
	bool dynamic = false;
	bool renderUnique = false;
	bool walkable = false;
	PrimitiveType primitiveType = PrimitiveType_None;

	NO_SERIALIZE_IN_EDITOR;
	// Object Effect Data
	CU::Color effectColor = { 0, 0, 0, 255 };
	CU::Color outlineColor = { 0, 0, 0, 255 };
	float effectTValue = 0;
	unsigned int psEffectIndex = 0;

	int modelType = EModelType_STATIC;
	bool engineRenderTarget = false;
};
);

SERIALIZE_STRUCT(AnimatedMeshComponent,
	struct AnimatedMeshComponent
{
	ModelAnimatedID model;
	bool renderUnique = false;

	NO_SERIALIZE_IN_EDITOR;
	ShortString activeAnimation1;
	ShortString activeAnimation2;
	bool shouldAnimate = true;
	bool drawSkeleton = false;
	bool isLooping = true;
	int modelType = EAnimatedModelType_NORMAL;
	float currentActiveTime1 = 0;
	float currentActiveTime2 = 0;
	float blendFactor = 0;

	// Object Effect Data
	CU::Color effectColor = { 0, 0, 0, 255 };
	CU::Color outlineColor = { 0, 0, 0, 255 };
	float effectTValue = 0;
	unsigned int psEffectIndex = 0;
};
);

SERIALIZE_STRUCT(PointLightComponent,
	struct PointLightComponent
{
	v3f offset;
	CU::Color color;
	float range = 5.0f;
	float baseIntensity = 1.0f;
	float lightSize = 1.0f;
	LightFlags shadowType = LightFlags_NoShadow;
};
);

SERIALIZE_STRUCT(SpotLightComponent,
	struct SpotLightComponent
{
	v3f offset;
	CU::Color color;
	float range = 5.0f;
	float angle = 70.0f;
	float baseIntensity = 1.0f;
	float lightSize = 1.0f;
	LightFlags shadowType = LightFlags_NoShadow;
};
);

SERIALIZE_STRUCT(RenderTargetComponent,
	struct RenderTargetComponent
{
	RenderCamera CameraIndex = RenderCamera_None;
	RenderFlag CameraRenderFlag = RenderFlag_NoUI;
	RenderRes Resolution = RenderRes_x16y9quarter;
	NO_SERIALIZE_IN_EDITOR;
};
);

SERIALIZE_STRUCT(LevelSelectComponent,
	struct LevelSelectComponent
{
	uint8_t connectedLevel = 0;
	NO_SERIALIZE_IN_EDITOR;
	ShortString levelName = "Level_T";
	bool open = false;
};
);

struct DecayingPointLightComponent
{
	float baseIntensity;
	float endIntensity;
	float time;
	float timer;
};
struct DecayingSpotLightComponent
{
	float baseIntensity;
	float endIntensity;
	float time;
	float timer;
};

struct AnimatedMultiMeshComponent
{
	ModelAnimatedID model;
	ModelAnimatedID modelAccessory;
	bool renderUnique = false;

	ShortString activeAnimation1;
	ShortString activeAnimation2;
	bool shouldAnimate = true;
	bool drawSkeleton = false;
	bool isLooping = true;
	int modelType = EAnimatedModelType_NORMAL;
	float currentActiveTime1 = 0;
	float currentActiveTime2 = 0;
	float blendFactor = 0;

	// Object Effect Data
	CU::Color effectColor = { 0, 0, 0, 255 };
	CU::Color outlineColor = { 0, 0, 0, 255 };
	float effectTValue = 0;
	unsigned int psEffectIndex = 0;
};

SERIALIZE_STRUCT(ParticleEmitterComponent,
	struct ParticleEmitterComponent
{
	ParticleID particleSystem;
	v3f offset;
	bool isPlaying = true;
	bool shouldSpawn = true;
	NO_SERIALIZE_IN_EDITOR;
	//Do not touch this Index as it is handled by the Manager
	uint16_t renderIndex = _UI16_MAX;
};
);

SERIALIZE_STRUCT(ParticleMultiEmitterComponent,
	struct ParticleMultiEmitterComponent
{
	ParticleMID1 particleSystem1;
	ParticleMID2 particleSystem2;
	ParticleMID3 particleSystem3;
	v3f offset1;
	v3f offset2;
	v3f offset3;
	bool isPlaying1 = true;
	bool isPlaying2 = true;
	bool isPlaying3 = true;
	bool shouldSpawn1 = true;
	bool shouldSpawn2 = true;
	bool shouldSpawn3 = true;
	NO_SERIALIZE_IN_EDITOR;
	//Do not touch these Indices as they are handled by the Manager
	uint16_t renderIndex1 = _UI16_MAX;
	uint16_t renderIndex2 = _UI16_MAX;
	uint16_t renderIndex3 = _UI16_MAX;
};
);


enum class BlendFlag
{
	LERP,
	QUADRATIC,
	QUADRATIC_INV,
	SMOOTHSTEP,
	LOGARITHMIC,
};

struct AnimationControllerComponent
{
	BlendFlag blendFlag = BlendFlag(0);
	float* blendReference = nullptr;

	float transitionDuration = 0;
	float animDuration = 0;
	float t = 0;

	bool isTransitioning = false;
	bool isAnimationOver = false;
	bool isActingOnValue = false;

	bool isLooping = true;
	bool shouldActOnRef = false;
	bool isFinal = false;

	ShortString defaultAnim;
};


struct MovementComponent
{
	float maxVelocity = 5000.f; // CONST How fast the actor can move
	float acceleration = 10000.0f; // CONST How much linear acceleration the actor has when grounded
	float drag = -1000.f; // CONST The friction constant that applies to velocity when the actor is grounded

	float rotationalVelocity = 15.0f; // CONST How fast the actor can rotate
	float angularAcceleration = 2.f; // CONST How fast the actor accelerates their rotation
	float angularSlowdown = 25.0f; // CONST How fast the rotation stops without additional input
	float dashForce = 10000.0f;

	bool isGrounded = true; // If the actor is touching the ground.
	bool isDashing = false;

	v2f angularVelocity; // 2D rotation input based on acceleration
	v3f lastFramePos; // How much the position changed since last frame
	v3f velocity; // How much the position should change this frame

	v3f pitchYawRoll; // The actor's current orientation
	v3f movementInput; // What data the actor should act on
};

struct ClientComponent
{
	uint32_t clientID = 0;
};

struct BulletComponent
{
	v3f vel;
	v3f gravity;
	v3f knockBack;
	float lifeTimer = 0.0f;
	float damage = 0.0f;
	Entity owner;
};

struct MissileComponent
{
	v3f vel;
	v3f acc;
	v3f dir;
	float knockBack = 0;
	float lifeTimer = 0.0f;
	float damage = 0.0f;
	float radius = 0.f;
	Entity owner;
	unsigned int ownerID = 0;
};

struct MeleeComponent
{
	v3f knockBack;
	float lifeTimer = 0.0f;
	float damage = 0.0f;
	Entity owner;
};

struct MineComponent
{
	float knockBack;
	float lifeTimer = 0.0f;
	float damage = 0.0f;
	float radius = 0.0f;
	Entity owner;
	bool triggered = false;
};

struct ExplosiveComponent
{
	float knockBack;
	float damage = 0.0f;
	float radius = 0.f;
	int frameCounter = 0;
	Entity owner;
	bool damageOwner = true;
};

struct MagnetComponent
{
	float pullForce;
	float radius;
	float lifeTimer = 0.0f;
	Entity owner;
};

struct FireTrailComponent
{
	float spawnTime;
	float spawnTimer;
	float fireTime;
	float damage;
	float timer;
	Entity owner;
};

struct FireComponent
{
	float timer;
	float baseDamage;
	float damage;
	float flicker;
	Entity owner;
};

namespace AI
{
	enum AIType : uint16_t
	{
		AIType_Dummy,
		AIType_Drone
	};
}

struct AIControllerComponent
{
	std::function<void(Entity, const float)> handleInput;
	std::function<void(Entity, const float)> handleUpdate;

	Data data;
};

SERIALIZE_STRUCT(BoxColliderComponent,
	struct BoxColliderComponent
{
	v3f boxSize = { 100.0f, 100.0f, 100.0f };
	v3f offset = { 0.0f, 0.0f, 0.0f };
	PhysicsMaterial material;
};
);

SERIALIZE_STRUCT(SphereColliderComponent,
	struct SphereColliderComponent
{
	float radius = 50.0f;
	v3f offset = { 0.0f, 0.0f, 0.0f };
	PhysicsMaterial material;
};
);

SERIALIZE_STRUCT(PlaneColliderComponent,
	struct PlaneColliderComponent
{
	v2f size = { 500.0f, 500.0f };
	v3f offset = { 0.0f, 0.0f, 0.0f };
	PhysicsMaterial material;
};
);

struct NavVertexComponent
{
	unsigned short index = 0;
};

SERIALIZE_STRUCT(LoaComponent,
	struct LoaComponent
{
	LoaID id;
	NO_SERIALIZE_IN_EDITOR;
	int instanceID;
	bool triggerEnter = false;
};
);
SERIALIZE_STRUCT(DecalComponent,
	struct DecalComponent
{
	MaterialID material;
	NO_SERIALIZE_IN_EDITOR;
};
);

// Adding this component deletes the entity at the end of the frame
struct EliaComponent
{
	float destroyTimer = 0.0f;
};

struct HealthComponent
{
	v3f knockbackVelocity = { 0.0f, 0.0f, 0.0f };
	float damagePercentage = 1.0f;
	float maxDamagePercentage = FLT_MAX;
	float deflectionTimer = 0.0f;
	float invincibilityTimer = 0.0f;
	bool deflectionActive = false;
	bool dead = false;
	bool isStunned = false;
};

struct RespawnComponent
{
	float respawnTime = 1.0f;
	float respawnTimer = 1.0f;
	float respawnDelay = 1.0f;
	v3f offset = { 0.0f, 300.0f, 0.0f };
	bool effectsSpawned = false;
};

SERIALIZE_STRUCT(SpawnComponent,
	struct SpawnComponent
{
	int spawnID = 0;
};
);

SERIALIZE_STRUCT(TileComponent,
	struct TileComponent
{
	TileType tiletype;
};
);

struct SpikeComponent
{
	float activationTimer = 0.0f;
	float activationTime = 1.0f;
	float damage = 0.2f;
	bool activated = false;
};

struct LaserComponent
{
	float baseKnockBack;
	float knockBack;
	float activeTimer = 3.5f;
	float baseDamage = 3.0f;
	float damage = 0.0f;
	Entity owner;
};

SERIALIZE_STRUCT(PickupComponent,
	struct PickupComponent
{
	float cooldownTime = 10.0f;
	PickupType type = PickupType_Health;
	NO_SERIALIZE_IN_EDITOR;
	float cooldownTimer = 0.0f;
	Entity pickupMesh = MAX_ENTITIES;
	Entity pickedUpBy = INVALID_ENTITY;
	float activationTime = 1.5f;
	bool activated = false;
};
);

#include "../Engine/ECS/UIComponents.h"

COMPONENT_LIST(
	LIST_COMPONENT(TransformComponent),
	LIST_COMPONENT(MeshComponent),
	LIST_COMPONENT(AnimatedMeshComponent),
	LIST_COMPONENT(SpotLightComponent),
	LIST_COMPONENT(PointLightComponent),
	LIST_COMPONENT(BoxColliderComponent),
	LIST_COMPONENT(SphereColliderComponent),
	LIST_COMPONENT(PlaneColliderComponent),
	LIST_COMPONENT(RenderTargetComponent),
	LIST_COMPONENT(LoaComponent),
	LIST_COMPONENT(DecalComponent),
	LIST_COMPONENT(ParticleEmitterComponent),
	LIST_COMPONENT(ParticleMultiEmitterComponent),
	LIST_COMPONENT(SpawnComponent),
	LIST_COMPONENT(UITransformComponent),
	LIST_COMPONENT(TileComponent),
	LIST_COMPONENT(LevelSelectComponent),
	LIST_COMPONENT(UIInteractiveComponent),
	LIST_COMPONENT(UISliderComponent),
	LIST_COMPONENT(UICheckboxComponent),
	LIST_COMPONENT(UIBtnComponent),
	LIST_COMPONENT(UISpriteComponent),
	LIST_COMPONENT(PickupComponent),
	LIST_COMPONENT(UISliderBtnComponent),
);