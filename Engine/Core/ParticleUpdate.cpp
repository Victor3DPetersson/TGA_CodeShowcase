#include "stdafx.h"
#include "ParticleUpdate.h"

#include "Managers\ParticleManager.h"
#include "ECS\Systems\RenderCommands.h"
#include "ECS\Components.h"
#include "../CommonUtilities/CU/Math/BezierSolver.h"

namespace Engine
{
	//Declaring internal function
	void SpawnParticle(ParticleRenderCommand* aCommandToFill, ParticleCommand* aPC, ParticleEffect& aEffect);
	CU::Transform rotationTransform;
	void UpdateParticleComponent(ParticleEffect& someSettings, ParticleEmissionData& someEmissionData, ParticleRenderCommand* aCommandToFill, ParticleCommand* aPC, const float aDT)
	{
		const ParticleEffect::ParticleSettings& settings = someSettings.GetSettings();
		const float EPSILON = 0.1f * aDT + 0.055f;
	
		ParticleEmissionData& emissionData = someEmissionData;
		emissionData.lifeTime += aDT;
		if (emissionData.lifeTime > settings.myBurstSpawnDelay)
		{
			emissionData.burstTimer += aDT;
			emissionData.spawnTimer += aDT;
		}
		//spawn Logic
		const float spawnTime = 1 / settings.mySpawnRate;
		if (settings.myBurstMode)
		{
			if (emissionData.burstTimer < settings.myBurstLength && emissionData.spawnTimer > spawnTime)
			{
				const float burstDelta = emissionData.burstTimer - emissionData.prevBurstTimer;
				const unsigned int particlesToSpawn = unsigned int(burstDelta * settings.mySpawnRate);
				emissionData.spawnTimer = 0;
				emissionData.prevBurstTimer = emissionData.burstTimer;

				for (unsigned int i = 0; i < particlesToSpawn; ++i)
				{
					SpawnParticle(aCommandToFill, aPC, someSettings);
				}
			}
			if (settings.myIsContinouslyBursting && emissionData.burstTimer > settings.myBurstSpaceTime)
			{
				emissionData.burstTimer = 0;
				emissionData.prevBurstTimer = 0;
			}
		}
		else
		{
			if (emissionData.spawnTimer > spawnTime)
			{
				emissionData.spawnTimer = 0;
				SpawnParticle(aCommandToFill, aPC, someSettings);
			}
		}
		ParticleRenderCommand& commandToFill = *aCommandToFill;
		///Logic for Updating the Particles
		for (unsigned int i = 0; i < commandToFill.myAmountOfActiveVertices; i++)
		{
			if (i < commandToFill.myAmountOfActiveVertices)
			{
				Vertex_Particle& vertex = commandToFill.myVertices[i];
				if (vertex.myLifetime < vertex.myEndTime)
				{
					vertex.myLifetime += aDT;
					float t = vertex.myLifetime / vertex.myEndTime;

					vertex.myVelocity += (settings.myDrag * CU::Matrix3x3f(aPC->myMatrix)) * aDT;
					vertex.myPosition += vertex.myVelocity * aDT;
					float sizeT = t;
					if (settings.myUseCurvesSize)
					{
						sizeT = (CU::CalculateBezierSet(settings.myAmountOfSizePoints, &settings.mySizeOverTimePoints[0], t, settings.mySizeCurveStrength));

					}
					if (settings.myIsBillboard == false)
					{
				
						vertex.mySize.x = CU::Lerp(settings.myParticleSpawnSizeXY.x, settings.myParticleEndSizeXY.x, sizeT);
						vertex.mySize.y = CU::Lerp(settings.myParticleSpawnSizeXY.y, settings.myParticleEndSizeXY.y, sizeT);
						vertex.mySizeZ = CU::Lerp(settings.myParticleSpawnSizeZ, settings.myParticleEndSizeZ, sizeT);
					}
					else
					{
						vertex.mySizeZ = 1;
						vertex.mySize = CU::Lerp(settings.myParticleSpawnSizeXY, settings.myParticleEndSizeXY, sizeT);
					}
				
					if (settings.myForwardIsDirection)
					{						
						rotationTransform.LookAt(vertex.myVelocity.GetNormalized() + vertex.myPosition, vertex.myPosition);
						v3f radians = rotationTransform.GetRotationQ().GetEulerAnglesRadians();
						vertex.myRotationX = radians.x;
						vertex.myRotationY = radians.y;
						vertex.myRotation = radians.z;
					}
					else
					{
						if (settings.myRotateZAxis) { vertex.myRotation += vertex.myRotationDir * aDT; }
						if (settings.myIsBillboard == false)
						{
							if (settings.myRotateXAxis) { vertex.myRotationX += vertex.myRotationDir * aDT; }
							if (settings.myRotateYAxis) { vertex.myRotationY += vertex.myRotationDir * aDT; }
						}
					}
					if (settings.myUseCurvesEmission)
					{
						vertex.myEmissiveStrength = CU::Lerp(settings.myParticleEmissiveStrength, settings.myEmissiveEndStrength, CU::CalculateBezierSet(settings.myAmountOfEmissionPoints, &settings.myEmissionOverTimePoints[0], t, settings.myEmissiveCurveStrength));
					}
					if (vertex.myCurrentColor < settings.myNumberOfColors - 1)
					{
						unsigned int currentColor = vertex.myCurrentColor;
						float blendPosition0 = settings.myColorBlendTimers[currentColor];
						float blendPosition1 = settings.myColorBlendTimers[currentColor + 1];
						float dif = blendPosition1 - blendPosition0;
						if (t > blendPosition0 && t < blendPosition1)
						{
							t -= blendPosition0;
							t /= dif;
							t = CU::Clamp(0.0f, 1.0f, t);
							vertex.myColor.LerpColor(settings.myColorsToBlendBetween[currentColor], settings.myColorsToBlendBetween[currentColor + 1], t);
							if (t >= 1.0f - EPSILON)
							{
								vertex.myCurrentColor = vertex.myCurrentColor + 1;
							}
						}
					}
				}
				else
				{
					commandToFill.myAmountOfActiveVertices--;
					vertex = commandToFill.myVertices[commandToFill.myAmountOfActiveVertices];
					commandToFill.myVertices[commandToFill.myAmountOfActiveVertices].myDistanceToCamera = FLT_MAX;
					commandToFill.myVertices[commandToFill.myAmountOfActiveVertices].myLifetime = FLT_MAX;
					i--;
				}
			}
		}
		
	}

	void SpawnParticle(ParticleRenderCommand* aCommandToFill, ParticleCommand* aPC, ParticleEffect& aEffect)
	{
		if (!aPC->shouldSpawn)
		{
			return;
		}

		assert(aEffect.GetData().myNumberOfParticles >= aCommandToFill->myAmountOfActiveVertices && "Victor did very wrong BAD VICTOR!");
		ParticleEffect::ParticleSettings& settings = aEffect.GetSettings();
		Vertex_Particle vertex;
		vertex.myColor = settings.myColorsToBlendBetween[0];
		vertex.myCurrentColor = 0;
		vertex.myEmissiveStrength = settings.myParticleEmissiveStrength;
		vertex.myEndTime = Random.RandNumbInRange(settings.myParticleMinLifeTime, settings.myParticleMaxLifeTime);
		vertex.myLifetime = 0;
		v3f spawnPos;
		float radius = 0;
		switch (settings.myEmitterShape)
		{
		case ParticleEffect::EmitterShape::eBOX:
			spawnPos.x = Random.RandNumbInRange(spawnPos.x + (-settings.myBoxSize.x * 0.5f), spawnPos.x + (settings.myBoxSize.x * 0.5f));
			spawnPos.y = Random.RandNumbInRange(spawnPos.y + (-settings.myBoxSize.y * 0.5f), spawnPos.y + (settings.myBoxSize.y * 0.5f));
			spawnPos.z = Random.RandNumbInRange(spawnPos.z + (-settings.myBoxSize.z * 0.5f), spawnPos.z + (settings.myBoxSize.z * 0.5f));
			break;
		case ParticleEffect::EmitterShape::eSPHERE:
			spawnPos.x = Random.RandNumbInRange(-1, 1);
			spawnPos.y = Random.RandNumbInRange(-1, 1);
			spawnPos.z = Random.RandNumbInRange(-1, 1);
			spawnPos.Normalize();
			radius = Random.RandNumbInRange(0, settings.myRadius);
			spawnPos *= radius;
			break;
		default:
			break;
		}
		vertex.myPosition = spawnPos + aPC->myMatrix.GetTranslationVector();
		vertex.mySize = settings.myParticleSpawnSizeXY;
		vertex.mySizeZ = settings.myParticleSpawnSizeZ;
		//v3f rotationVector = aPC->myMatrix.GetUpVector();
		//CU::Matrix3x3f rotationMatrixX = CU::Matrix3x3f(aPC->myMatrix);
		//CU::Matrix3x3f rotationMatrixY = CU::Matrix3x3f(aPC->myMatrix);
		//CU::Matrix3x3f rotationMatrixZ = CU::Matrix3x3f(aPC->myMatrix);
		//CU::Matrix3x3f rotationMatrix;

		//rotationMatrixZ = rotationMatrix.CreateRotationAroundZ(Random.RandNumbInRange(0, CU::AngleToRadian(settings.mySpawnAngle)));
		//rotationMatrixY = rotationMatrix.CreateRotationAroundY(Random.RandNumbInRange(0, CU::AngleToRadian(settings.mySpawnAngle)));
		//rotationMatrixX = rotationMatrix.CreateRotationAroundX(Random.RandNumbInRange(0, CU::AngleToRadian(settings.mySpawnAngle)));

		//rotationMatrix = rotationMatrixZ * rotationMatrixY * rotationMatrixX;
		//rotationVector = rotationVector * rotationMatrix;
		v3f forcevec;
		//v3f precalc = (settings.myForce );
		if (settings.mySpawnForceRandX)
		{
			if (settings.mySpawnForceMirrorX)
			{
				float force = settings.mySpawnForceMin.x;
				forcevec.x = Random.RandNumbInRange(CU::Min(force, force * -1), CU::Max(force, force * -1));
			}
			else
			{
				forcevec.x = Random.RandNumbInRange(settings.mySpawnForceMin.x, settings.mySpawnForceMax.x);
			}
		}
		else
		{
			forcevec.x = settings.myForce.x;
		}
		if (settings.mySpawnForceRandY)
		{
			if (settings.mySpawnForceMirrorY)
			{
				float force = settings.mySpawnForceMin.y;
				forcevec.y = Random.RandNumbInRange(CU::Min(force, force * - 1), CU::Max(force, force * -1));
			}
			else
			{
				forcevec.y = Random.RandNumbInRange(settings.mySpawnForceMin.y, settings.mySpawnForceMax.y);
			}
		}
		else
		{
			forcevec.y = settings.myForce.y;
		}
		if (settings.mySpawnForceRandZ)
		{
			if (settings.mySpawnForceMirrorZ)
			{
				float force = settings.mySpawnForceMin.z;
				forcevec.z = Random.RandNumbInRange(CU::Min(force, force * -1), CU::Max(force, force * -1));
			}
			else
			{
				forcevec.z = Random.RandNumbInRange(settings.mySpawnForceMin.z, settings.mySpawnForceMax.z);
			}
		}
		else
		{
			forcevec.z = settings.myForce.z;
		}
		if (settings.mySpawnForcesNormalized)
		{
			forcevec.Normalize();
		}
		vertex.myUVPanningSpeed = Random.RandNumbInRange(-1, 1);

		if (settings.myIsBillboard == false)
		{
			if (settings.mySpawnParticleWithRandomRotationX)
			{
				vertex.myRotationX = CU::AngleToRadian(Random.RandNumbInRange(settings.myParticleSpawnMinRotationDirectionXY.x, settings.myParticleSpawnMaxRotationDirectionXY.x));
			}
			else
			{
				vertex.myRotationX = CU::AngleToRadian(settings.myParticleSpawnMaxRotationDirectionXY.x);
			}
			if (settings.mySpawnParticleWithRandomRotationY)
			{
				vertex.myRotationY = CU::AngleToRadian(Random.RandNumbInRange(settings.myParticleSpawnMinRotationDirectionXY.y, settings.myParticleSpawnMaxRotationDirectionXY.y));
			}
			else
			{
				vertex.myRotationY = CU::AngleToRadian(settings.myParticleSpawnMaxRotationDirectionXY.y);
			}
			vertex.myBillBoardState = 1;
		}

		if (settings.mySpawnParticleWithRandomRotationZ)
		{
			vertex.myRotation = CU::AngleToRadian(Random.RandNumbInRange(settings.myParticleSpawnMinRotationDirectionZ, settings.myParticleSpawnMaxRotationDirectionZ));
		}
		else
		{
			vertex.myRotation = CU::AngleToRadian(settings.myParticleSpawnMaxRotationDirectionZ);
		}
		if (settings.myRotateRandomRotation)
		{
			vertex.myRotationDir = Random.RandNumbInRange(settings.myMinRotationSpeed, settings.myMaxRotationSpeed);
		}
		else
		{
			vertex.myRotationDir = settings.myMaxRotationSpeed;
		}
		vertex.myVelocity = forcevec * settings.myParticleSpeed * CU::Matrix3x3f(aPC->myMatrix);
		if (settings.myForwardIsDirection)
		{
			rotationTransform.LookAt(vertex.myVelocity.GetNormalized() + vertex.myPosition, vertex.myPosition);
			v3f radians = rotationTransform.GetRotationQ().GetEulerAnglesRadians();
			vertex.myRotationX = radians.x;
			vertex.myRotationY = radians.y;
			vertex.myRotation = radians.z;
		}
		else
		{
		}
		aCommandToFill->myVertices[aCommandToFill->myAmountOfActiveVertices] = vertex;
		aCommandToFill->myAmountOfActiveVertices++;
	}

	void UpdateDepthFromCamera(ParticleRenderCommand* aCommandToSort, const v3f aCameraPosition)
	{
		if (aCommandToSort->myAmountOfActiveVertices > 0)
		{
			for (unsigned int i = 0; i < aCommandToSort->myAmountOfActiveVertices; i++)
			{
				Vertex_Particle& vertex = aCommandToSort->myVertices[i];
				vertex.myDistanceToCamera = (vertex.myPosition - aCameraPosition).LengthSqr();
			}
			//std::sort(myVertices, myVertices + myAmountOfActiveVertices);

			std::sort(&aCommandToSort->myVertices[0], &aCommandToSort->myVertices[aCommandToSort->myAmountOfActiveVertices - 1], [](Vertex_Particle& aV1, Vertex_Particle& av2)
				{
					return aV1.myDistanceToCamera > av2.myDistanceToCamera;
				});
		}
	}
}
