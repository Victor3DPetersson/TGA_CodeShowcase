#pragma once
#include "../Resources\CubeTexture.h"
#include "../../../GameObjects/Camera.h"
#include <atomic>
#include "../../../ECS\Systems\RenderCommands.h"

struct ID3D11DeviceContext;
struct ID3D11Device;
struct ID3D11ComputeShader;
struct MeshesToRender;
struct SortedModelDataForBuffers;
struct SortedModelDataForRendering;
namespace Engine
{
	class ConstantBufferManager;
	class FullScreenTexture;
	class FullScreenTextureArray;
	struct RenderTarget;
	struct RenderData;
	class FullScreenRenderer;
	class DeferredRenderer;
	class ShadowRenderer;
	class GBuffer;
	struct RenderStates;

	struct SHBakeData
	{
		unsigned int bakeW = 0;
		unsigned int bakeH = 0;
		unsigned int bakeD = 0;
		unsigned int linearProgress = 0;
		unsigned int numberOfProbes = 0;
		float progressCounter = 0;
		float bakeSpacing = 0;
		float bakeAmount = 0;
		v3f bakeMiddle;
		v3f bakeSize;
		m4f bakeRotation;
		float gridBakeProgress = 0;
	};

	struct CubeRendererEditorData
	{
		ID3D11PixelShader* spherePS = nullptr;
		ID3D11PixelShader* gridPS = nullptr;
		MeshRenderCommand debugSphere;
		SortedModelDataForRendering* meshBuffer = nullptr;
		unsigned int reflectionProbeToDebug = INVALID_ENTITY;
		SHBakeData gridsBakeData[NUMBOF_SHGRIDS];
		ID3D11Buffer* SHGridBuffer = nullptr;


		bool isDebugRenderingGrid = false;
		bool isDebugRenderingSHAmbientLight = true;

		bool gridsToDebugRender[NUMBOF_SHGRIDS];
		unsigned int amountOfBakeGridsInLevel = 0;
		unsigned int amountOfDebugGridsInLevel = 0;
		unsigned int SHOffsetCounter = 0;
		unsigned int totalNumberOfBakedSHProbes = 0;
		unsigned int currentOffset = 0;
		bool isRendering = false;
		bool isRenderingAllGrids = false;

		ID3D11Buffer* reflectionProbeDebugBuffer = nullptr;
		unsigned int gridToRender = 0;

	};
	struct CubeRenderData
	{
		CU::Matrix4x4f cubeRotationMatrixes[6];
		v3f cubeMapPos;
		CubeTexture cubeMap;

		FullScreenTextureArray* cubeDownSampledTargets_Lods[8];
		FullScreenTexture* cubeRenderTargets[6];
		FullScreenTexture* cubeDepth = nullptr;
		FullScreenTexture* SSAOtarget = nullptr;
		FullScreenTexture* SSAOBlurredtarget = nullptr;
		GBuffer* GBufferTarget = nullptr;
		Camera renderCam;
		ID3D11Buffer* cubemapRenderSizeBuffer = nullptr;
		SH SHToFill;

		ID3D11ComputeShader* preBlurShader = nullptr;
		CubeTexture filteredCubeMap;
		CubeTextureArray levelReflectionProbes;
		ID3D11Buffer* levelReflectionProbeBuffer = nullptr;
		ID3D11ShaderResourceView* levelReflectionProbeBufferSRV = nullptr;
		v2ui cubeMapTextureSize;
	};

	class CubemapRenderer
	{
	public:
		bool Init(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext, ConstantBufferManager* aConstantBufferManager, FullScreenRenderer* aFullscreenRenderer, DeferredRenderer* aDeferredRenderer, ShadowRenderer* aShadowRenderer, unsigned int& aNumberOfMips);
		bool InitGPUAndRenderData();
		void EditorInit();
		void DeInitGPUData();

		void RenderCubeMapAndSH(RenderData* aRenderBuffer, v3f aCubeMapPosition, MeshesToRender& meshesToFill, RenderStates* someRenderStates, std::atomic<unsigned int>& aDrawcallCounter, bool aGeneratePrefiltered, float aOuterRadius);
		void RenderReflectionProbes(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, RenderStates* someRenderStates, std::atomic<unsigned int>& aDrawcallCounter);
		void RenderLightProbeGrid(RenderData* aRenderBuffer, MeshesToRender& meshesToFill, RenderStates* someRenderStates, std::atomic<unsigned int>& aDrawcallCounter);

		void StartRenderOfAllGrids(unsigned int aNumberOfGrids);
		void StartRenderOfGrid(unsigned int aNumberOfGrids, unsigned int aGridToRender);

		const bool GetIsRenderingGrid();
		const bool GetIsBakingReflectionProbes();
		SHGridData* GetSHGridData(unsigned int aIndex);
		const bool IsEditorInited();
		const unsigned int GetTotalNumberLightProbesOnGPU() { return myBakedTotalLightProbeCount; }
		const unsigned int GetNumberLightProbeGridsOnGPU() { return myLoadedGridAmount; }
		SH* GetSHLevelData();
		void SetNumberOfLightProbesOnGPU(const unsigned int aLightProbeAmount) { myBakedTotalLightProbeCount = aLightProbeAmount; }

		void UpdateGridsGPUDataWithEdtiorData();
		void UpdateGridsGPUData(unsigned short aGridAmount);
		void UpdateSHLightProbeData(unsigned int aLightProbeAmount, SH* aSHSet);
		void BakeLevelReflectionProbes(unsigned int aReflectionProbeAmount, EditorReflectionProbe* someLoadedProbesToRender, bool aAddReflectionProbes);

		//Editor Functions
		void Debug_DrawGrid(Camera& aRenderCamera);
		void Debug_RenderReflectionProbe(Camera& aRenderCamera);
		void Debug_SetDrawSpecificGrid(bool aDrawState, unsigned int aIndex);
		void Debug_SetAmbientSHLight(bool aDrawState);
		void Debug_SetDrawGrid(bool aDrawState);
		bool Debug_DrawGrid();
		void Debug_SetNumberOfGrids(unsigned int aNumberOfGrids);
		void Debug_SetNumberOfBakeGrids(unsigned int aNumberOfGrids);
		void Debug_SetReflectionProbeToRender(unsigned int aReflectionProbeToRender);
		bool Debug_DrawReflectionProbe();

		const SHGridData GetSHLevelBakeData(unsigned int aIndex);
		float GetProgressOfGridBake(unsigned int aIndex);
		float GetTotalProgressOfGridBakes();
		void SetBakeDataWithGridData(SH* aHarmonicSet);

	private:
		void SetDebugGridDataForGPU(unsigned int aIndex);
		void GenerateSphericalHormonicsSet();
		void PrefilterCubemapResource();
		void CopyCubeTextureResource(bool aGeneratePrefiltered);

		ID3D11Device* myDevice = nullptr;
		ID3D11DeviceContext* myContext = nullptr;
		ConstantBufferManager* myCBufferManager = nullptr;
		FullScreenRenderer* myFullscreenRenderer = nullptr;
		DeferredRenderer* myDeferredRenderer = nullptr;
		ShadowRenderer* myShadowRenderer = nullptr;

		CubeRendererEditorData* myEditorData = nullptr;
		CubeRenderData* myRenderData = nullptr;

		SH* mySHSet = nullptr;
		SHGridData myLoadedGridGPUData[MAX_NUMBOF_SHGRIDS];
		unsigned int myLoadedGridAmount = 0;
		unsigned int myTotalLightProbeCount = 0;
		unsigned int myBakedTotalLightProbeCount = 0;

		ReflectionProbe myLoadedLevelReflectionProbesData[MAX_NUMBOF_REFLECTIONPROBES];
		unsigned int myLoadedReflectionProbeAmount = 0;

		bool myBakeReflectionProbes = false;
		ModelManager* myModelManager = nullptr;
		unsigned int myMemoryNewCounter = 0;

	};
}


