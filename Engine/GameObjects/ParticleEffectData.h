#pragma once
#include "../../CommonUtilities/CU/Utility/FixedString.hpp"
#include "../../CommonUtilities/CU/Math/Vector.h"
#include "..\Core\Rendering\Resources\DX_Includes.h"
#include "Material.h"

class ParticleEffect
{
public:
	enum EmitterShape
	{
		ePOINT,
		eBOX,
		eSPHERE,
		eCOUNT
	};
	struct ParticleSettings
	{
		unsigned int myNumberOfColors = 2;
		float myColorBlendTimers[5] = { 0, 0.25f, 0.5f, 0.75f, 1.0f };

		CU::Color myColorsToBlendBetween[5];
		v3f myForce = { 0, 0.1f, 0 };
		v3f myDrag = { -0.1f, 0.0f, 0 };
		v3f myOffSetAsSubSystem = { 0, 0, 0 };
		float mySpawnRate = 50.0f;
		float myParticleSpeed = 50.5f;
		float myParticleSpawnSizeZ = 1.0f;
		float myParticleEndSizeZ = 1.50f;
		float myParticleEmissiveStrength = 1.0;
		float myParticleMinLifeTime = 2.50f;
		float myParticleMaxLifeTime = 7.50f;

		float myParticleSpawnMinRotationDirectionZ = 0;
		float myParticleSpawnMaxRotationDirectionZ = 0;
		float myMinRotationSpeed = -1;
		float myMaxRotationSpeed = 0;


		bool mySpawnParticleWithRandomRotationZ = false;
		bool myRotateRandomRotation = false;

		float myBurstLength = 1.0f;
		float myBurstSpaceTime = 2.0f;

		bool myBurstMode = false;
		bool myIsContinouslyBursting = false;

		EmitterShape myEmitterShape = EmitterShape::ePOINT;
		v3f myBoxSize{100.0f, 100.0f, 100.0f};
		float myRadius = 100.0f;


		//New stuff

		bool myIsBillboard = true;
		bool myIsMeshParticle = false;

		GUID myMeshGUID = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
		FixedString256 myMeshName;
		unsigned short mySubmeshID = 0;

		v2f myParticleSpawnMinRotationDirectionXY;
		v2f myParticleSpawnMaxRotationDirectionXY;
		bool mySpawnParticleWithRandomRotationX = false;
		bool mySpawnParticleWithRandomRotationY = false;

		v2f myParticleSpawnSizeXY = { 1, 1 };
		v2f myParticleEndSizeXY = { 1.5f, 1.5f };
		bool myIsUniformScale = false;

		v3f mySpawnForceMin={-10, -10, -10};
		v3f mySpawnForceMax = { 10, 10, 10 };
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
		v2f myEmissionOverTimePoints[20];
		float myEmissiveCurveStrength = 1.0;
		float myEmissiveEndStrength = 1.0;

		bool myUseCurvesSize = false;
		int myAmountOfSizePoints = 3;
		float mySizeCurveStrength = 1.0;
		v2f mySizeOverTimePoints[20];

		float myBurstSpawnDelay = 0.f;

		bool mySpawnForcesNormalized = false;
	};
	struct ParticleEmitterData
	{
		unsigned int myNumberOfParticles = 0;
		ID3D11Buffer* myParticleVertexBuffer = nullptr;
		Material* myMaterial;
		bool myIsInited = false;
	};
	ParticleSettings& GetSettings() { return mySettings; }
	ParticleEmitterData& GetData() { return myData; }
private:
	ParticleSettings mySettings;
	ParticleEmitterData myData;
};

