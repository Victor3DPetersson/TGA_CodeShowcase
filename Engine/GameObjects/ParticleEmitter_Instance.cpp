#include "stdafx.h"
#include "ParticleEmitter_Instance.h"
#include "ParticleEffectData.h"

#include <d3d11.h>

#include <fstream>

//bool ParticleEmitter_Instance::Init(ParticleEffect* aEmitter, ID3D11Device* aDevice)
//{
//	assert(!myIsInited && "Can not init an Emitter more than once");
//	myEmitter = aEmitter;
//	myLifeTime = 0;
//
//	if (myEmitter->GetData().myIsInited)
//	{
//		myRenderCommand.myVertices = new Vertex_Particle[myEmitter->GetData().myNumberOfParticles];
//	}
//	else
//	{
//		HRESULT result;
//
//		unsigned int myAmountOfParticles = (unsigned int)(myEmitter->GetSettings().myParticleMaxLifeTime * myEmitter->GetSettings().mySpawnRate) + 1;
//
//		myRenderCommand.myVertices = new Vertex_Particle[myAmountOfParticles];
//		D3D11_BUFFER_DESC particleVertexBufferDesc = { 0 };
//		particleVertexBufferDesc.ByteWidth = sizeof(Vertex_Particle) * myAmountOfParticles;
//		particleVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
//		particleVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//		particleVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//		D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
//		subresourceData.pSysMem = myRenderCommand.myVertices;
//		ID3D11Buffer* vertexBuffer = nullptr;
//		result = aDevice->CreateBuffer(&particleVertexBufferDesc, &subresourceData, &vertexBuffer);
//		if (FAILED(result))
//		{
//			assert(false && "Failed to create Vertex Buffer");
//		}
//		myEmitter->GetData().myParticleVertexBuffer = vertexBuffer;
//		myEmitter->GetData().myIsInited = true;
//		myEmitter->GetData().myNumberOfParticles = myAmountOfParticles;
//	}
//
//	myRenderCommand.myStride = sizeof(Vertex_Particle);
//	myRenderCommand.myParticleVertexBuffer = myEmitter->GetData().myParticleVertexBuffer;
//	myRenderCommand.myMaterial = myEmitter->GetData().myMaterial;
//	myBoxSize = myEmitter->GetSettings().myBoxSize;
//	mySphereRadius = myEmitter->GetSettings().myRadius;
//	myMaterialType = myEmitter->GetData().myMaterial->myMaterialType;
//	myIsInited = true;
//
//	return true;
//}
//
////Should ever only be used by an editor, as it is unoptimized to specify the amount of vertices the system has
//bool ParticleEmitter_Instance::Init(ParticleEmitter* aEmitter, ID3D11Device* aDevice, unsigned int aNumberOfVertices)
//{
//	assert(!myIsInited && "Can not init an Emitter more than once");
//	myEmitter = aEmitter;
//	myLifeTime = 0;
//	if (myEmitter->GetData().myIsInited)
//	{
//		myRenderCommand.myVertices = new Vertex_Particle[aNumberOfVertices];
//	}
//	else
//	{
//		HRESULT result;
//
//		unsigned int myAmountOfParticles = aNumberOfVertices;
//		myRenderCommand.myVertices = new Vertex_Particle[aNumberOfVertices];
//		D3D11_BUFFER_DESC particleVertexBufferDesc = { 0 };
//		particleVertexBufferDesc.ByteWidth = sizeof(Vertex_Particle) * myAmountOfParticles;
//		particleVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
//		particleVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//		particleVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//		D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
//		subresourceData.pSysMem = myRenderCommand.myVertices;
//
//		ID3D11Buffer* vertexBuffer = nullptr;
//		result = aDevice->CreateBuffer(&particleVertexBufferDesc, &subresourceData, &vertexBuffer);
//		if (FAILED(result))
//		{
//			assert(false && "Failed to create Vertex Buffer");
//		}
//		myEmitter->GetData().myParticleVertexBuffer = vertexBuffer;
//		myEmitter->GetData().myIsInited = true;
//		myEmitter->GetData().myNumberOfParticles = myAmountOfParticles;
//	}
//	myRenderCommand.myStride = sizeof(Vertex_Particle);
//	myBoxSize = myEmitter->GetSettings().myBoxSize;
//	mySphereRadius = myEmitter->GetSettings().myRadius;
//	myRenderCommand.myParticleVertexBuffer = myEmitter->GetData().myParticleVertexBuffer;
//	myRenderCommand.myMaterial = myEmitter->GetData().myMaterial;
//	myIsInited = true;
//		switch (myEmitter->GetSettings().myEmitterShape)
//		{
//		case ParticleEmitter::EmitterShape::eBOX:
//			myRenderCommand.myParticlesBound.InitWithMinAndMax(myEmitter->GetSettings().myBoxSize * -0.5f, myEmitter->GetSettings().myBoxSize * 0.5f);
//			break;
//		case ParticleEmitter::EmitterShape::eSPHERE:
//		{
//			float radM = myEmitter->GetSettings().myRadius * -1.0f;
//			float radP = myEmitter->GetSettings().myRadius;
//			myRenderCommand.myParticlesBound.InitWithMinAndMax(v3f(radM, radM, radM), v3f(radP, radP, radP));
//		}
//			break;
//		case ParticleEmitter::EmitterShape::ePOINT:
//			myRenderCommand.myParticlesBound.InitWithMinAndMax(v3f(100, 100, 100), v3f(100, 100, 100));
//			break;
//		default:
//			break;
//		}
//	return true;
//}
//
//void ParticleEmitter_Instance::ResetSystem()
//{
//	Vertex_Particle vertex;
//	for (unsigned int i = 0; i < myEmitter->GetData().myNumberOfParticles; i++)
//	{
//		myRenderCommand.myVertices[i] = vertex;
//		myRenderCommand.myVertices[i].myDistanceToCamera = FLT_MAX;
//	}
//	myRenderCommand.myAmountOfActiveVertices = 0;
//	myLifeTime = 0;
//	myBurstTimer = 0;
//	myPrevBurstTimer = 0;
//	mySpawnTimer = 0;
//
//	myShouldSpawn = true;
//	
//}
//
//void ParticleEmitter_Instance::SetBoxSize(v3f aBoxSize)
//{
//	myBoxSize = aBoxSize;
//}
//
//void ParticleEmitter_Instance::SetEmissionRadius(float aRadius)
//{
//	mySphereRadius = aRadius;
//}
//
//void ParticleEmitter_Instance::Update(const float aDeltaTime, const m4f& aComponentTransform )
//{
//	 const ParticleEmitter::ParticleSettings& settings = myEmitter->GetSettings();
//	 const float EPSILON = 0.1f * aDeltaTime + 0.055f;
//	 if (myIsPlaying)
//	 {
//		 myLifeTime += aDeltaTime;
//		 myBurstTimer += aDeltaTime;
//		 mySpawnTimer += aDeltaTime;
//		 const float spawnTime = 1 / settings.mySpawnRate;
//		 if (settings.myBurstMode)
//		 {
//			 if (myBurstTimer < settings.myBurstLength && mySpawnTimer > spawnTime)
//			 {
//				 const float burstDelta = myBurstTimer - myPrevBurstTimer;
//				 const unsigned int particlesToSpawn = unsigned int(burstDelta * settings.mySpawnRate);
//				 mySpawnTimer = 0;
//				 myPrevBurstTimer = myBurstTimer;
//
//				 for (unsigned int i = 0; i < particlesToSpawn; ++i)
//				 {
//					 SpawnParticle(aComponentTransform);
//				 }
//			 }
//			 if (settings.myIsContinouslyBursting && myBurstTimer > settings.myBurstSpaceTime)
//			 {
//				 myBurstTimer = 0;
//				 myPrevBurstTimer = 0;
//			 }
//		 }
//		 else
//		 {
//			 if (mySpawnTimer > spawnTime)
//			 {
//				 mySpawnTimer = 0;
//				 SpawnParticle(aComponentTransform);
//			 }
//		 }
//		
//		 ///Logic for Updating the Particles
//		 for (unsigned int i = 0; i < myRenderCommand.myAmountOfActiveVertices; i++)
//		 {
//			 if (i < myRenderCommand.myAmountOfActiveVertices)
//			 {
//				 Vertex_Particle& vertex = myRenderCommand.myVertices[i];
//				 if (vertex.myLifetime < vertex.myEndTime)
//				 {
//					 vertex.myLifetime += aDeltaTime;
//					 float t = vertex.myLifetime / vertex.myEndTime;
//
//					 vertex.myVelocity += (settings.myDrag * CU::Matrix3x3f(aComponentTransform)) * aDeltaTime;
//					 vertex.myPosition += vertex.myVelocity * aDeltaTime;
//					 vertex.mySize.x = CU::Lerp(settings.myParticleSpawnSize, settings.myParticleEndSize, t);
//					 vertex.mySize.y = vertex.mySize.x;
//					 vertex.myRotation += vertex.myRotationDir * aDeltaTime;
//					 if (vertex.myCurrentColor < settings.myNumberOfColors - 1)
//					 {
//						 unsigned int currentColor = vertex.myCurrentColor;
//						 float blendPosition0 = settings.myColorBlendTimers[currentColor];
//						 float blendPosition1 = settings.myColorBlendTimers[currentColor + 1];
//						 float dif = blendPosition1 - blendPosition0;
//						 if (t > blendPosition0 && t < blendPosition1)
//						 {
//							 t -= blendPosition0;
//							 t /= dif;
//							 t = CU::Clamp(0.0f, 1.0f, t);
//							 vertex.myColor.LerpColor(settings.myColorsToBlendBetween[currentColor], settings.myColorsToBlendBetween[currentColor + 1], t);
//							 if (t >= 1.0f - EPSILON)
//							 {
//								 vertex.myCurrentColor = vertex.myCurrentColor + 1;
//							 }
//						 }
//					 }
//				 }
//				 else
//				 {
//					 myRenderCommand.myAmountOfActiveVertices--;
//					 vertex = myRenderCommand.myVertices[myRenderCommand.myAmountOfActiveVertices];
//					 myRenderCommand.myVertices[myRenderCommand.myAmountOfActiveVertices].myDistanceToCamera = FLT_MAX;
//					 myRenderCommand.myVertices[myRenderCommand.myAmountOfActiveVertices].myLifetime = FLT_MAX;
//					 i--;
//				 }
//			 }
//		 }
//	 }
//}
//
//void ParticleEmitter_Instance::UpdateDepthFromCamera(const v3f aCameraPosition)
//{
//	if (myRenderCommand.myAmountOfActiveVertices > 0 && myEmitter->GetData().myMaterial->myMaterialType == MaterialTypes::EParticle_Default)
//	{
//		for (unsigned int i = 0; i < myRenderCommand.myAmountOfActiveVertices; i++)
//		{
//			Vertex_Particle& vertex = myRenderCommand.myVertices[i];
//			vertex.myDistanceToCamera = (vertex.myPosition - aCameraPosition).LengthSqr();
//		}
//		//std::sort(myVertices, myVertices + myAmountOfActiveVertices);
//
//		std::sort(&myRenderCommand.myVertices[0], &myRenderCommand.myVertices[myRenderCommand.myAmountOfActiveVertices - 1], [](Vertex_Particle& aV1, Vertex_Particle& av2)
//			{
//				return aV1.myDistanceToCamera > av2.myDistanceToCamera;
//			});
//	}
//}
//
//void ParticleEmitter_Instance::SetTransform(const m4f& aTransform)
//{
//	memcpy(&myRenderCommand.myTransform, &aTransform, sizeof(m4f));
//}
//
//void ParticleEmitter_Instance::PlayBurst()
//{
//	if (myEmitter->GetSettings().myBurstMode)
//	{
//		myIsPlaying = true;
//		myBurstTimer = 0;
//		myPrevBurstTimer = 0;
//		myLifeTime = 0;
//	}
//	else
//	{
//		SetIsPlaying(true);
//	}
//}
//
//void ParticleEmitter_Instance::AddSubEmitter(ParticleCommand aEmitterToAdd)
//{
//	mySubEmitters[myNumberOfSubEmitters] = aEmitterToAdd;
//	myNumberOfSubEmitters++;
//}
//
//void ParticleEmitter_Instance::RemoveSubEmitter(unsigned int aIndex)
//{
//	mySubEmitters[aIndex] = mySubEmitters[myNumberOfSubEmitters - 1];
//	myNumberOfSubEmitters--;
//}
//
//void ParticleEmitter_Instance::RemoveSubEmitters()
//{
//	myNumberOfSubEmitters = 0;
//}
//
//const ParticleCommand ParticleEmitter_Instance::GetSubEmitters(const unsigned int aIndex)
//{
//	return mySubEmitters[aIndex];
//}
//
//void ParticleEmitter_Instance::ReleaseAllResources()
//{
//	delete [] myRenderCommand.myVertices;
//	myEmitter = nullptr;
//}
//
//void ParticleEmitter_Instance::SpawnParticle(const m4f& aComponentTransform)
//{
//	if (!myShouldSpawn)
//	{
//		return;
//	}
//
//	assert(myEmitter->GetData().myNumberOfParticles > myRenderCommand.myAmountOfActiveVertices && "Victor did very wrong BAD VICTOR!");
//	const ParticleEmitter::ParticleSettings& settings = myEmitter->GetSettings();
//	Vertex_Particle vertex;
//	vertex.myColor = settings.myColorsToBlendBetween[0];
//	vertex.myCurrentColor = 0;
//	vertex.myEmissiveStrength = settings.myParticleEmissiveStrength;
//	vertex.myEndTime = Random.RandNumbInRange(settings.myParticleMinLifeTime, settings.myParticleMaxLifeTime);
//	vertex.myLifetime = 0;
//	v3f spawnPos;
//	float radius = 0;
//	switch (settings.myEmitterShape)
//	{
//	case ParticleEmitter::EmitterShape::eBOX:
//		spawnPos.x = Random.RandNumbInRange(spawnPos.x + (-myBoxSize.x * 0.5f), spawnPos.x + (myBoxSize.x * 0.5f));
//		spawnPos.y = Random.RandNumbInRange(spawnPos.y + (-myBoxSize.y * 0.5f), spawnPos.y + (myBoxSize.y * 0.5f));
//		spawnPos.z = Random.RandNumbInRange(spawnPos.z + (-myBoxSize.z * 0.5f), spawnPos.z + (myBoxSize.z * 0.5f));
//		break;
//	case ParticleEmitter::EmitterShape::eSPHERE:
//		spawnPos.x = Random.RandNumbInRange(-1, 1);
//		spawnPos.y = Random.RandNumbInRange(-1, 1);
//		spawnPos.z = Random.RandNumbInRange(-1, 1);
//		spawnPos.Normalize();
//		radius = Random.RandNumbInRange(0, mySphereRadius);
//		spawnPos *= radius;
//		break;
//	default:
//		break;
//	}
//	vertex.myPosition = spawnPos + myOffset + aComponentTransform.GetTranslationVector();
//	vertex.mySize.x = settings.myParticleSpawnSize;
//	vertex.mySize.y = settings.myParticleSpawnSize;
//
//	v3f rotationVector = aComponentTransform.GetUpVector();
//	CU::Matrix3x3f rotationMatrixX = CU::Matrix3x3f(aComponentTransform);
//	CU::Matrix3x3f rotationMatrixY = CU::Matrix3x3f(aComponentTransform);
//	CU::Matrix3x3f rotationMatrixZ = CU::Matrix3x3f(aComponentTransform);
//	CU::Matrix3x3f rotationMatrix;
//
//	rotationMatrixZ = rotationMatrix.CreateRotationAroundZ(Random.RandNumbInRange(0, CU::AngleToRadian(settings.mySpawnAngle)));
//	rotationMatrixY = rotationMatrix.CreateRotationAroundY(Random.RandNumbInRange(0, CU::AngleToRadian(settings.mySpawnAngle)));
//	rotationMatrixX = rotationMatrix.CreateRotationAroundX(Random.RandNumbInRange(0, CU::AngleToRadian(settings.mySpawnAngle)));
//
//	rotationMatrix = rotationMatrixZ * rotationMatrixY * rotationMatrixX;
//	rotationVector = rotationVector * rotationMatrix;
//	v3f forcevec = settings.myForce * CU::Matrix3x3f(aComponentTransform);
//
//	vertex.myUVPanningSpeed = Random.RandNumbInRange(-1, 1);
//
//	if (settings.mySpawnParticleWithRandomRotation)
//	{
//		vertex.myRotation = Random.RandNumbInRange(settings.myParticleSpawnMinRotationDirection, settings.myParticleSpawnMaxRotationDirection);
//	}
//	else
//	{
//		vertex.myRotation = settings.myParticleSpawnMaxRotationDirection;
//	}
//	if (settings.myRotateRandomRotation)
//	{
//		vertex.myRotationDir = Random.RandNumbInRange(settings.myMinRotationSpeed, settings.myMaxRotationSpeed);
//	}
//	else
//	{
//		vertex.myRotationDir = settings.myParticleRotationSpeed;
//	}
//	vertex.myVelocity = rotationVector * settings.myParticleSpeed + forcevec;
//	myRenderCommand.myVertices[myRenderCommand.myAmountOfActiveVertices] = vertex;
//	myRenderCommand.myAmountOfActiveVertices++;
//}
