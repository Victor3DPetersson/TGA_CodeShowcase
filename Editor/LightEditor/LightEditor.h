#pragma once
namespace Windows
{
	namespace LightEditor
	{
		bool DrawLightVolumesGizmosInEditor();
		bool DrawReflectionProbesInEditor();

		void LoadLevelData(void* someLightData);
		void ResetData();

		int GetGridEntity(unsigned int aGridIndex);
		int GetRProbeEntity(unsigned int aReflectionProbeIndex);
		float GetGridSpacing(unsigned int aGridIndex);
		void SetActiveEditedGrid(size_t aGridIndex);
		void SetActiveEditedProbe(size_t aReflectionProbeIndex);
		void* GetSceneGrids(); // Will return data of the type Editor_ProbeGrid
		void* GetSceneRProbes(); // Will return data of the type Editor_ReflectionProbe
		void* GetLevelLightData();
	}
	void LightEditorInit();
	bool LightEditorUpdate(float aDT, void*);

}