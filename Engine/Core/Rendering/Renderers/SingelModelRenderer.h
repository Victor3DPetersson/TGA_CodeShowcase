#pragma once

class Model;
class ModelAnimated;
class Model_Particle;
class Camera;
struct ParticleRenderCommand;
struct MeshesToRender;
namespace Engine
{
	class ModelManager;
	void SortMesh(MeshesToRender& meshesToFill, Model* aModelToSort, Camera& aCamera, ModelManager* aModelManager);
	void SortMeshAnimated(MeshesToRender& meshesToFill, ModelAnimated** aModelToSort, Camera& aCamera, m4f* aSkeleton, unsigned int aNumberOfModels, bool aRenderWithFloor = true, CU::Color aMeshColor = {255, 255, 255, 255});
	void SortMeshParticle(MeshesToRender& meshesToFill, size_t aNumbOfCommands, ParticleRenderCommand* aParticle, Camera& aCamera, bool aRenderGround);
}