#include "LightEditor.h"
#include "Cmn.h"
#include "../Engine/RenderData.h"
#include "../Externals/imgui/imgui.h"
#include <Misc\IconsFontAwesome5.h>
#include <FileBrowser\FileBrowser.h>

#include "LevelState\LevelState.h"
#include "..\Engine\Core\Rendering/Resources\FullScreenTexture_Factory.h"
#include "..\Engine\Core\DirectXFramework.h"
#include "Misc\ViewerCamControls.h"
#include "..\Engine\Core\Rendering/Renderer.h"
#include "..\Engine\Core\Rendering/Renderers\CubemapRenderer.h"
#include "..\Engine\RenderData.h"

namespace f = std::filesystem;

namespace Windows
{
	namespace LightEditor
	{
		void UpdateECS();
		void UpdateReflectionProbes();
		void UpdateSHGrids();
		void UpdatePostProcessing();
		void SetSHRendererGridData(unsigned int aIndex);
	}
	struct LightEditorData
	{
		EditorReflectionProbe rProbes[NUMBOF_REFLECTIONPROBES];
		bool renderReflectProbesGizmos = false;
		bool renderDebugReflectProbes = false;
		bool renderSHGridGizmos = false;
		bool renderDebugCube = false;
		bool renderDebugGrid = false;
		bool renderingAllGrids = false;
		bool renderWithSHAmbientLighting = false;
		bool renderOnlyIrradiantLight = false;
		EditorLightProbeGrid probeGrids[NUMBOF_SHGRIDS];
		size_t activeRProbeIndex = INVALID_ENTITY;
		size_t activeGridIndex = INVALID_ENTITY;
		size_t numberOfGridsInMap = 0;
		size_t numberOfProbesInMap = 0;
		Engine::CubemapRenderer* cubeRenderer;
		PostProcessingData levelPostProcessData;
	} gLightEdData;

	void LightEditorInit()
	{
		gLightEdData.cubeRenderer = EngineInterface::GetRenderer()->GetCubeMapRenderer();
		EngineInterface::GetRenderer()->SetRenderWithHarmonics(gLightEdData.renderWithSHAmbientLighting);
		gLightEdData.cubeRenderer->EditorInit();
	}

	bool LightEditorUpdate(float aDT, void*)
	{
		bool result = true;

		ImGui::Begin(ICON_FA_LIGHTBULB " Light Editor Editor###LightEditor", &result, ImGuiWindowFlags_MenuBar);
	
		if (ImGui::Checkbox("Render With Baked Game Lighting", &gLightEdData.renderWithSHAmbientLighting))
		{
			EngineInterface::GetRenderer()->SetRenderWithHarmonics(gLightEdData.renderWithSHAmbientLighting);
			if (gLightEdData.renderWithSHAmbientLighting)
			{
				levelState.renderFlag = RenderFlag_AllPasses;
				levelState.fogFarDistance = 10000;
				levelState.fogNearDistance = 9000;
				levelState.ambienceFogColor = levelState.ambientLight;
			}
			else
			{
				levelState.renderFlag = RenderFlag_NoUiOrPost;
			}
		}
		if (ImGui::Checkbox("Render Baked Ambiance Only", &gLightEdData.renderOnlyIrradiantLight))
		{
			EngineInterface::GetRenderer()->SetRenderIrradiantLightOnly(gLightEdData.renderOnlyIrradiantLight);
		}
		LightEditor::UpdateReflectionProbes();
		LightEditor::UpdateSHGrids();
		LightEditor::UpdatePostProcessing();
		ImGui::End();
		return result;
	}
	namespace LightEditor
	{
		bool DrawLightVolumesGizmosInEditor()
		{
			return gLightEdData.renderSHGridGizmos;
		}
		bool DrawReflectionProbesInEditor()
		{
			return gLightEdData.renderReflectProbesGizmos;
		}

		void LoadLevelData(void* someLightData)
		{
			ResetData();
			LevelLightExportData* lightData = (LevelLightExportData*)someLightData;
			gLightEdData.numberOfGridsInMap = lightData->light.numberOfGridsInLevel;
			for (size_t grid = 0; grid < lightData->light.numberOfGridsInLevel; grid++)
			{
				gLightEdData.probeGrids[grid] = lightData->light.editorGridData[grid];
				if (gLightEdData.renderSHGridGizmos)
				{
					gLightEdData.probeGrids[grid].ent = levelState.ecs.GetEntity();
					levelState.transformComponents.AddComponent(gLightEdData.probeGrids[grid].ent).transform = gLightEdData.probeGrids[grid].transform;
					gLightEdData.probeGrids[grid].renderDebugGrid = true;
					SetSHRendererGridData(grid);
					gLightEdData.cubeRenderer->Debug_SetDrawSpecificGrid(gLightEdData.probeGrids[grid].renderDebugGrid, grid);
				}
			}
			gLightEdData.numberOfProbesInMap = lightData->light.rProbes.numberOfRProbes;
			for (size_t i = 0; i < gLightEdData.numberOfProbesInMap; i++)
			{
				gLightEdData.rProbes[i] = lightData->light.rProbes.probes[i];
				if (gLightEdData.renderReflectProbesGizmos)
				{
					gLightEdData.rProbes[i].ent = levelState.ecs.GetEntity();
					levelState.transformComponents.AddComponent(gLightEdData.rProbes[i].ent).transform = gLightEdData.rProbes[i].transform;
				}
			}
			gLightEdData.levelPostProcessData = lightData->post;
		}
		void ResetData()
		{
			for (size_t grid = 0; grid < NUMBOF_SHGRIDS; grid++)
			{
				gLightEdData.probeGrids[grid] = EditorLightProbeGrid();
				gLightEdData.probeGrids[grid].transform.SetScale(v3f(1000, 1000, 1000));
			}
			for (size_t i = 0; i < NUMBOF_REFLECTIONPROBES; i++)
			{
				gLightEdData.rProbes[i] = EditorReflectionProbe();
			}
			gLightEdData.activeRProbeIndex = INVALID_ENTITY;
			gLightEdData.activeGridIndex = INVALID_ENTITY;
			gLightEdData.numberOfProbesInMap = 0;
			gLightEdData.numberOfGridsInMap = 0;
			gLightEdData.levelPostProcessData = PostProcessingData();
		}
		int GetGridEntity(unsigned int aGridIndex)
		{
			return gLightEdData.probeGrids[aGridIndex].ent;
		}
		int GetRProbeEntity(unsigned int aProbeIndex)
		{
			return gLightEdData.rProbes[aProbeIndex].ent;
		}
		float GetGridSpacing(unsigned int aGridIndex)
		{
			return gLightEdData.probeGrids[aGridIndex].gridDensity;
		}
		void SetActiveEditedProbe(size_t aReflectionProbeIndex)
		{
			gLightEdData.activeRProbeIndex = aReflectionProbeIndex;
		}
		void SetActiveEditedGrid(size_t aGridIndex)
		{
			gLightEdData.activeGridIndex = aGridIndex;
		}
		void* GetSceneGrids()
		{
			return &gLightEdData.probeGrids;
		}
		void* GetSceneRProbes()
		{
			return gLightEdData.rProbes;
		}

		void* GetLevelLightData()
		{
			LevelLightExportData* levelData = new LevelLightExportData();
			levelData->light.numberOfGridsInLevel = gLightEdData.numberOfGridsInMap;
			for (unsigned int grid = 0; grid < gLightEdData.numberOfGridsInMap; grid++)
			{
				levelData->light.gridData[grid] = gLightEdData.cubeRenderer->GetSHLevelBakeData(grid);
				levelData->light.gridData[grid].brightness = gLightEdData.probeGrids[grid].brightness;
				levelData->light.totalNumberOfHarmonics += levelData->light.gridData[grid].numberOfHarmonics;
				levelData->light.editorGridData[grid].gridDensity = gLightEdData.probeGrids[grid].gridDensity;
				levelData->light.editorGridData[grid].transform = gLightEdData.probeGrids[grid].transform;
				levelData->light.editorGridData[grid].brightness = gLightEdData.probeGrids[grid].brightness;
			}
			levelData->light.SHData = gLightEdData.cubeRenderer->GetSHLevelData();
			levelData->light.rProbes.numberOfRProbes = gLightEdData.numberOfProbesInMap;
			for (size_t i = 0; i < levelData->light.rProbes.numberOfRProbes; i++)
			{
				levelData->light.rProbes.probes[i] = gLightEdData.rProbes[i];
			}
			levelData->post = gLightEdData.levelPostProcessData;
			return levelData;
		}

		void UpdateECS()
		{
			//Returning Entitys if it should not have one
			if (gLightEdData.renderSHGridGizmos == false)
			{
				gLightEdData.renderDebugGrid = false;
				gLightEdData.cubeRenderer->Debug_SetDrawGrid(gLightEdData.renderDebugGrid);
				gLightEdData.activeGridIndex = INVALID_ENTITY;
				for (unsigned int grid = 0; grid < gLightEdData.numberOfGridsInMap; grid++)
				{
					if (gLightEdData.probeGrids[grid].ent != INVALID_ENTITY)
					{
						levelState.transformComponents.RemoveComponent(gLightEdData.probeGrids[grid].ent);
						levelState.ecs.ReturnEntity(gLightEdData.probeGrids[grid].ent);
						gLightEdData.probeGrids[grid].ent = INVALID_ENTITY;
					}
				}
			}
			else
			{
				for (unsigned int grid = 0; grid < gLightEdData.numberOfGridsInMap; grid++)
				{
					if (gLightEdData.probeGrids[grid].ent == INVALID_ENTITY)
					{
						gLightEdData.probeGrids[grid].ent = levelState.ecs.GetEntity();
						TransformComponent& tc = levelState.transformComponents.AddComponent(gLightEdData.probeGrids[grid].ent);
						tc.transform = gLightEdData.probeGrids[grid].transform;
					}
				}
			}
		}
		void UpdateReflectionProbes()
		{
			if (ImGui::TreeNode("Reflection Probes"))
			{
				if (ImGui::Checkbox("Visualize Reflection Gizmos", &gLightEdData.renderReflectProbesGizmos))
				{
					if (gLightEdData.renderReflectProbesGizmos)
					{
						for (size_t i = 0; i < gLightEdData.numberOfProbesInMap; i++)
						{
							if (gLightEdData.renderReflectProbesGizmos)
							{
								gLightEdData.rProbes[i].ent = levelState.ecs.GetEntity();
								levelState.transformComponents.AddComponent(gLightEdData.rProbes[i].ent).transform = gLightEdData.rProbes[i].transform;
							}
						}
					}
					else
					{
						for (size_t i = 0; i < gLightEdData.numberOfProbesInMap; i++)
						{
							levelState.transformComponents.RemoveComponent(gLightEdData.rProbes[i].ent);
							levelState.ecs.ReturnEntity(gLightEdData.rProbes[i].ent);
							gLightEdData.rProbes[i].ent = INVALID_ENTITY;
						}
						gLightEdData.activeRProbeIndex = INVALID_ENTITY;
					}
				}
				if (gLightEdData.renderDebugReflectProbes == false)
				{
					gLightEdData.cubeRenderer->Debug_SetReflectionProbeToRender(INVALID_ENTITY);
				}
				if (ImGui::Checkbox("Visualize Reflection Probe", &gLightEdData.renderDebugReflectProbes));

				if (ImGui::Button("Build Level Reflection Probes"))
				{
					//Todo fill this shit
					if (gLightEdData.numberOfProbesInMap > 0)
					{
						ReflectionProbesData* reflectionProbeData = new ReflectionProbesData();
						reflectionProbeData->numberOfRProbes = gLightEdData.numberOfProbesInMap;
						memcpy(reflectionProbeData->probes, gLightEdData.rProbes, sizeof(EditorReflectionProbe) * gLightEdData.numberOfProbesInMap);
						Engine::AddDataPackageToRenderer(Engine::ELoadPackageTypes::BakeReflectionProbes, reflectionProbeData);
					}

				}
				ImGui::Text("Number of Reflection Captures in scene %i", gLightEdData.numberOfProbesInMap);
				size_t& activeProbe = gLightEdData.activeRProbeIndex;
				if (gLightEdData.numberOfProbesInMap < NUMBOF_REFLECTIONPROBES && ImGui::Button("Create Reflection Probe"))
				{
					gLightEdData.rProbes[gLightEdData.numberOfProbesInMap] = EditorReflectionProbe();
					if (gLightEdData.renderReflectProbesGizmos)
					{
						gLightEdData.rProbes[gLightEdData.numberOfProbesInMap].ent = levelState.ecs.GetEntity();
						levelState.transformComponents.AddComponent(gLightEdData.rProbes[gLightEdData.numberOfProbesInMap].ent).transform = gLightEdData.rProbes[gLightEdData.numberOfProbesInMap].transform;
					}
					gLightEdData.numberOfProbesInMap++;
				}
				if (activeProbe != INVALID_ENTITY)
				{
					CU::BitArray<MAX_ENTITIES> occupied = levelState.ecs.GetOccupiedEntities();
					if (occupied.Test(gLightEdData.rProbes[activeProbe].ent) == false)
					{
						gLightEdData.rProbes[activeProbe].ent = INVALID_ENTITY;
						activeProbe = INVALID_ENTITY;
						gLightEdData.numberOfProbesInMap--;
						return;
					}
					if (ImGui::Button("Delete Probe"))
					{
						levelState.transformComponents.RemoveComponent(gLightEdData.rProbes[activeProbe].ent);
						levelState.ecs.ReturnEntity(gLightEdData.rProbes[activeProbe].ent);
						gLightEdData.rProbes[activeProbe].ent = INVALID_ENTITY;
						gLightEdData.rProbes[activeProbe] = gLightEdData.rProbes[gLightEdData.numberOfProbesInMap - 1];
						gLightEdData.numberOfProbesInMap--;
						if (activeProbe == gLightEdData.numberOfProbesInMap)
						{
							activeProbe--;
							if (activeProbe < 0)
							{
								activeProbe = 0;
								return;
							}
						}
					}
					if (gLightEdData.renderDebugReflectProbes)
					{
						gLightEdData.cubeRenderer->Debug_SetReflectionProbeToRender(activeProbe);
					}
					if (gLightEdData.rProbes[activeProbe].ent != INVALID_ENTITY)
					{
						if (levelState.transformComponents.HasComponent(gLightEdData.rProbes[activeProbe].ent))
						{
							gLightEdData.rProbes[activeProbe].transform = levelState.transformComponents.GetComponent(gLightEdData.rProbes[activeProbe].ent).transform;
						}
						ImGui::DragFloat("Reflection Probe Outer Radius", &gLightEdData.rProbes[activeProbe].outerRadius, 10.0f, gLightEdData.rProbes[0].innerRadius, 2500.f, "%.1f", 1.f);
						ImGui::DragFloat("Reflection Probe Inner Radius", &gLightEdData.rProbes[activeProbe].innerRadius, 10.0f, 10, gLightEdData.rProbes[0].outerRadius, "%.1f", 1.f);
						ImGui::DragFloat3("Reflection Probe Position", &gLightEdData.rProbes[activeProbe].transform.GetPosition()[0], 10.1f, -10000.f, 10000.f, "%.3f", 1.f);
						ImGui::DragFloat("Reflection Probe Brightness", &gLightEdData.rProbes[activeProbe].brightness, 0.01f, 0.0f, 10.f, "%.3f", 1.f);
						if (levelState.transformComponents.HasComponent(gLightEdData.rProbes[activeProbe].ent))
						{
							levelState.transformComponents.GetComponent(gLightEdData.rProbes[activeProbe].ent).transform = gLightEdData.rProbes[activeProbe].transform;
						}
						if (ImGui::Button("Build Reflection Probe"))
						{
							Engine::AddDataPackageToRenderer(Engine::ELoadPackageTypes::ReflectionProbe, &gLightEdData.rProbes[activeProbe]);
						}
					}
				}
				ImGui::TreePop();
			}
		}

		void UpdateSHGrids()
		{
			size_t& activeGrid = gLightEdData.activeGridIndex;
			size_t& numberOfGrids = gLightEdData.numberOfGridsInMap;
			if (ImGui::TreeNode("Baked Light Grids"))
			{
				ImGui::Checkbox("Visualize Ambient Gizmos", &gLightEdData.renderSHGridGizmos);
				UpdateECS();
				gLightEdData.cubeRenderer->Debug_SetNumberOfGrids(gLightEdData.numberOfGridsInMap);
				if (gLightEdData.renderSHGridGizmos && ImGui::Checkbox("Draw Debug Grid", &gLightEdData.renderDebugGrid))
				{
					if (gLightEdData.renderSHGridGizmos == false)
					{
						gLightEdData.renderDebugGrid = false;
					}
					gLightEdData.cubeRenderer->Debug_SetDrawGrid(gLightEdData.renderDebugGrid);
					for (unsigned int grid = 0; grid < gLightEdData.numberOfGridsInMap; grid++)
					{
						gLightEdData.probeGrids[grid].renderDebugGrid = gLightEdData.renderDebugGrid;
						SetSHRendererGridData(grid);
					}
				}
				if (ImGui::Button("Render All Grids"))
				{
					for (unsigned int grid = 0; grid < gLightEdData.numberOfGridsInMap; grid++)
					{
						SetSHRendererGridData(grid);
					}
					gLightEdData.cubeRenderer->StartRenderOfAllGrids(gLightEdData.numberOfGridsInMap);
					gLightEdData.renderingAllGrids = true;
				}
				ImGui::Text("Number of Light Grids in scene %i", gLightEdData.numberOfGridsInMap);
				if (numberOfGrids < NUMBOF_SHGRIDS && ImGui::Button("Create Light Probe Grid"))
				{
					EditorLightProbeGrid& grid = gLightEdData.probeGrids[numberOfGrids];
					grid = EditorLightProbeGrid();
					grid.transform.SetScale(v3f(1000.f, 1000.f, 1000.f));
					if (gLightEdData.renderSHGridGizmos)
					{
						grid.ent = levelState.ecs.GetEntity();
						levelState.transformComponents.AddComponent(grid.ent).transform = grid.transform;
					}
					numberOfGrids++;
				}
				if (gLightEdData.cubeRenderer->GetIsRenderingGrid() && gLightEdData.renderingAllGrids)
				{
					ImGui::Text("Current Grid bake progress: %f", gLightEdData.cubeRenderer->GetProgressOfGridBake(0));
					ImGui::Text("Total bake Progress: %f", gLightEdData.cubeRenderer->GetTotalProgressOfGridBakes());
				}

				if (activeGrid != INVALID_ENTITY)
				{
					CU::BitArray<MAX_ENTITIES> occupied = levelState.ecs.GetOccupiedEntities();
					if (occupied.Test(gLightEdData.probeGrids[activeGrid].ent) == false)
					{
						gLightEdData.probeGrids[activeGrid].ent = INVALID_ENTITY;
						activeGrid = INVALID_ENTITY;
						numberOfGrids--;
						return;
					}
					if (ImGui::Button("Delete Grid"))
					{
						levelState.transformComponents.RemoveComponent(gLightEdData.probeGrids[activeGrid].ent);
						levelState.ecs.ReturnEntity(gLightEdData.probeGrids[activeGrid].ent);
						gLightEdData.probeGrids[activeGrid].ent = INVALID_ENTITY;
						gLightEdData.probeGrids[activeGrid] = gLightEdData.probeGrids[numberOfGrids - 1];
						numberOfGrids--;
						if (activeGrid == numberOfGrids)
						{
							activeGrid--;
							if (activeGrid < 0)
							{
								activeGrid = 0;
								return;
							}
						}
					}
					if (gLightEdData.probeGrids[activeGrid].ent != INVALID_ENTITY)
					{
						EditorLightProbeGrid& grid = gLightEdData.probeGrids[activeGrid];
						if (grid.ent != INVALID_ENTITY && levelState.transformComponents.HasComponent(grid.ent))
						{
							grid.transform = levelState.transformComponents.GetComponent(grid.ent).transform;
						}
						float rotation = grid.transform.GetRotation().y;
						ImGui::DragFloat("Grid Spacing", &grid.gridDensity, 10.0f, 100, 2500.f, "%.1f", 1.f);
						ImGui::DragFloat3("Grid Position", &grid.transform.GetPosition()[0], 10.f, -10000.f, 10000.f, "%.3f", 1.f);
						ImGui::DragFloat3("Grid Scale", &grid.transform.GetScale()[0], 10.f, 0.f, 10000.f, "%.3f", 1.f);
						ImGui::DragFloat("Grid Rotation", &rotation, 0.1f, -180.f, 180.f, "%.3f", 1.f);
						ImGui::DragFloat("Grid Brightness", &grid.brightness, 0.01f, 0.0f, 10.f, "%.3f", 1.f);
						grid.transform.SetRotation(v3f(0, rotation, 0));
						SetSHRendererGridData(activeGrid);
						if (ImGui::Checkbox(FixedString256::Format("Debug Draw Grid %i", activeGrid), &grid.renderDebugGrid))
						{
							gLightEdData.cubeRenderer->Debug_SetDrawSpecificGrid(grid.renderDebugGrid, activeGrid);
							if (grid.renderDebugGrid)
							{
								gLightEdData.renderDebugGrid = grid.renderDebugGrid;
							}
						}
						if (ImGui::Button("Render Selected Grid"))
						{
							gLightEdData.cubeRenderer->StartRenderOfGrid(gLightEdData.numberOfGridsInMap, activeGrid);
						}
						if (gLightEdData.cubeRenderer->GetIsRenderingGrid() && gLightEdData.renderingAllGrids == false)
						{
							ImGui::Text("Grid bake progress: %f", gLightEdData.cubeRenderer->GetProgressOfGridBake(activeGrid));
						}
					}
				}
				gLightEdData.cubeRenderer->UpdateGridsGPUDataWithEdtiorData();
				ImGui::TreePop();
			}
		}
		void UpdatePostProcessing()
		{
			if (ImGui::TreeNode("Post Processing"))
			{
				PostProcessingData& ppData = gLightEdData.levelPostProcessData;

				if (ImGui::TreeNode("SSAO"))
				{
					ImGui::SliderFloat("Scale", &ppData.SSAO_scale, 0, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Bias", &ppData.SSAO_bias, 0, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Vignette"))
				{
					ImGui::SliderFloat("Strength", &ppData.VIGNETTE_strength, 0, 250.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Extent", &ppData.VIGNETTE_extent, 0, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Bloom Fog ChromaticA"))
				{
					ImGui::SliderFloat("Chromatic Strength", &ppData.CA_Strength, 0, 250.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Bloom Strength", &ppData.BLOOM_strength, 0, 5.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Fog Strength", &ppData.FOG_exponent, 0, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Fog Near", &ppData.FOG_nearDistance, 0, ppData.FOG_farDistance - 0.1f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Fog Far", &ppData.FOG_farDistance, ppData.FOG_nearDistance + 0.1f, 25000.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::ColorEdit3("Fog Color", &ppData.FOG_color[0]);
					if (ImGui::Button("Reset Bloom Fog CA"))
					{
						ppData.CA_Strength = 1.0f;
						ppData.BLOOM_strength = 2.f;
						ppData.FOG_exponent = 0.5f;
						ppData.FOG_nearDistance = 1000.0f;
						ppData.FOG_farDistance = 5000.0f;
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Color Adjustments"))
				{
					ImGui::SliderFloat("Exposure", &ppData.postExposure, 0, 5.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Contrast", &ppData.contrast, -100, 250.f, "%.1f");
					ImGui::ColorEdit3("Color Filter", &ppData.colorFilter[0]);
					ImGui::SliderFloat("Hue Shift", &ppData.hueShift, -180.f, 180.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Saturation", &ppData.saturation, -100.f, 100.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					if (ImGui::Button("Reset Adjustments"))
					{
						ppData.postExposure = 1.0f;
						ppData.colorFilter = { 1, 1, 1 };
						ppData.contrast = 0;
						ppData.saturation = 0;
						ppData.hueShift = 0;
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Split Toning"))
				{
					ImGui::ColorEdit3("Shadows", &ppData.splitToneShadowTint[0]);
					ImGui::ColorEdit3("Highlights", &ppData.splitToneHighLightTint[0]);
					ImGui::SliderFloat("Balance", &ppData.splitToningBalance, 0, 100.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
					if (ImGui::Button("Reset Split"))
					{
						ppData.splitToneShadowTint = { 0.5f, 0.5f, 0.5f };
						ppData.splitToneHighLightTint = { 0.5f, 0.5f, 0.5f };
						ppData.splitToningBalance = 0;
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Channel Mixer"))
				{
					ImGui::ColorEdit3("Red", &ppData.channelMixerR[0]);
					ImGui::ColorEdit3("Green", &ppData.channelMixerG[0]);
					ImGui::ColorEdit3("Blue", &ppData.channelMixerB[0]);
					if (ImGui::Button("Reset Channels"))
					{
						ppData.channelMixerR = { 1.f, 0, 0 };
						ppData.channelMixerG = { 0, 1.f, 0 };
						ppData.channelMixerB = { 0, 0, 1.f };
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Shadows Midtones Highlights"))
				{
					ImGui::ColorEdit3("Shadows", &ppData.SMH_shadows[0]);
					ImGui::ColorEdit3("Midtones", &ppData.SMH_midtones[0]);
					ImGui::ColorEdit3("Highlights", &ppData.SMH_highlights[0]);
					ImGui::SliderFloat("Shadows Start", &ppData.SMH_shadowStart , 0, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Shadows End", &ppData.SMH_shadowEnd, 0, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Highlights Start", &ppData.SMH_highlightStart, 0, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::SliderFloat("Highlights End", &ppData.SMH_highlightEnd, 0, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					if (ImGui::Button("Reset SMH"))
					{
						ppData.SMH_shadows = { 1.f, 1.f, 1.f };
						ppData.SMH_midtones = { 1.f, 1.f, 1.f };
						ppData.SMH_highlights = { 1.f, 1.f, 1.f };
						ppData.SMH_shadowStart = 0;
						ppData.SMH_shadowEnd = 0.3f;
						ppData.SMH_highlightStart = 0.55f;
						ppData.SMH_highlightEnd = 1.f;
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
				EngineInterface::GetRenderer()->SetPostProcessingData(ppData);
			}
		}
		void SetSHRendererGridData(unsigned int aIndex)
		{
			SHGridData& shData = *gLightEdData.cubeRenderer->GetSHGridData(aIndex);
			EditorLightProbeGrid& grid = gLightEdData.probeGrids[aIndex];
			shData.gridHalfSize = grid.transform.GetScale() * 0.5f;
			shData.gridSpacing = grid.gridDensity;
			shData.gridRotation = grid.transform.GetRotationQ().GetRotationMatrix44();
			shData.gridRotation.SetTranslation(grid.transform.GetPosition());
			shData.brightness = grid.brightness;
			if (grid.ent != INVALID_ENTITY && levelState.transformComponents.HasComponent(grid.ent))
			{
				levelState.transformComponents.GetComponent(grid.ent).transform = grid.transform;
			}
		}

	}
}