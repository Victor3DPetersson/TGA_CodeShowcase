#include "ParticleEditor.h"

#include "../Externals/imgui/imgui.h"
#include "../Externals/imgui/imGui_BezierWidget.h"
#include "Misc\IconsFontAwesome5.h"
#include <FileBrowser\FileBrowser.h>

#include "LevelState\LevelState.h"
#include "..\Engine\Core\Rendering/Resources\FullScreenTexture_Factory.h"
#include "..\Engine\Core\DirectXFramework.h"
#include "..\Engine\Core\Rendering/Renderer.h"

#include <../Engine/GameObjects/ParticleEffectData.h>
#include <../Engine/GameObjects/ParticleEmitter_Instance.h>

#include <../Engine/Managers/MaterialManager.h>
#include <../Engine/Managers/Managers.h>
#include <../Engine/Managers/ParticleManager.h>
#include <../Engine/ECS/ECS.h>
#include <../Engine/Core\ParticleUpdate.h>
#include "../Engine/Core/Rendering/DX_Functions/DX_RenderFunctions.h"

#include "../CommonUtilities/CU/Math/BezierSolver.h"

#include "Misc\ViewerCamControls.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <filesystem>
#include "Cmn.h"

namespace f = std::filesystem;
namespace Windows
{

	struct ParticleSettings
	{
		int imGuiShape = 0;
		bool showShape = false;
		float spawnedPerSecond = 50;
		float minLifeTime = 2.5f;
		float maxLifeTime = 5.f;
		FixedString256 myMaterialName;
		MaterialTypes myMaterialType = MaterialTypes::EParticle_Default;
		bool choosingBlendSystem = false;
		ParticleEffect::ParticleSettings blendSettings;
		ParticleEffect::ParticleSettings blendedFromSettings;
		GUID systemGUID = NIL_UUID;
	};

	struct SystemData
	{
		FixedString256 systemName;
		bool isUsed = true;
		bool showSystem = true;
		bool failedExport = false;
		bool render = true;
		bool isPlaying = true;

		ParticleSettings particleData;
		ParticleRenderCommand renderCommand;
		ParticleEffect pEffect;
		ParticleCommand command;
		ParticleEmissionData emission;
		ParticleMeshRenderCommand meshCommand;
		FixedString256 meshName;
		Model* model = nullptr;
	};

	struct ParticleEditorData
	{
		//Render Textures
		Engine::RenderTarget renderTarget;
		Engine::FullScreenTexture* texture = 0;
		Engine::FullScreenTexture* depth = 0;
		Engine::FullScreenTexture* intermediate = 0;
		Engine::GBuffer* gbuf = 0;

		CU::GrowingArray<SystemData> systems;
		ParticleRenderCommand* renderCommands;
		ParticleMeshRenderCommand* meshRenderCommands;
		unsigned short currentSystem = 0;
		bool importingSystem = false;
		bool blendingSystem = false;
		float blendFactor = 0;
		CamControlsSettings cameraSettings;
		v3f systemsPos;
		float rotationValue = 0;
		bool moveSystems = false;
		bool showGround = true;
		FixedString256 inputSystemName;

		ID3D11ShaderResourceView* myModelOBToWorldSRV = nullptr;
		ID3D11Buffer* myModelOBToWorldBuffer = nullptr;
		ID3D11ShaderResourceView* myModelEffectSRV = nullptr;
		ID3D11Buffer* myModelEffectBuffer = nullptr;

		CU::Transform lightRotateDirection;
	} gPaEd;

	void ParticleEditorInit()
	{
		ID3D11Device* const dev = EngineInterface::GetDXFramework()->GetDevice();
		ID3D11DeviceContext* const cont = EngineInterface::GetDXFramework()->GetDeviceContext();

		gPaEd.texture =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		gPaEd.depth =
			Engine::CreateDepthTexture({ 1920U, 1080U }, DXGI_FORMAT_D32_FLOAT, dev, cont, Engine::EDepthStencilSRV::CREATE, Engine::EDepthStencilFlag::BOTH);
		gPaEd.intermediate =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		gPaEd.gbuf =
			Engine::CreateGBuffer({ 1920U, 1080U }, dev, cont);

		gPaEd.renderTarget.texture = &gPaEd.texture;
		gPaEd.renderTarget.depthTexture = &gPaEd.depth;
		gPaEd.renderTarget.intermediateTexture = &gPaEd.intermediate;
		gPaEd.renderTarget.gBufferTexture = &gPaEd.gbuf;
		gPaEd.renderTarget.camera.RecalculateProjectionMatrix(90, { 1920.0f, 1080.0f }, true, 100.0f, 50000.0f);
		gPaEd.renderTarget.renderFlag = RenderFlag_NoUiOrPost;

		gPaEd.systems.Init(4);
		gPaEd.renderTarget.camera.GetTransform().SetPosition(v3f(0, 150, -1000));
		gPaEd.renderCommands = new ParticleRenderCommand[16];
		gPaEd.meshRenderCommands = new ParticleMeshRenderCommand[16];
		Engine::DX::CreateStructuredBuffer(dev, &gPaEd.myModelOBToWorldSRV, &gPaEd.myModelOBToWorldBuffer, 999999U, sizeof(m4f));
		Engine::DX::CreateStructuredBuffer(dev, &gPaEd.myModelEffectSRV, &gPaEd.myModelEffectBuffer, 999999U, sizeof(ObjectEffectData));
		gPaEd.lightRotateDirection.LookAt(v3f(), v3f(150, -250, 250));
	}

	bool ParticleEditor(float aDT, void*)
	{
		bool result = true;

		ImGui::SetNextWindowSize({ 0, 0 }, ImGuiCond_FirstUseEver);
		ImGui::Begin(ICON_FA_MAGIC " Particle Editor###particle editor window", &result, ImGuiWindowFlags_MenuBar);

		ImGui::BeginMenuBar();
		if (ImGui::Button(" Create New System "))
		{
			SystemData settings;
			settings.systemName = FixedString256::Format("Default : %i", gPaEd.systems.Size() + 1);
			gPaEd.systems.Add(settings);
			ParticleEditorInternal::CreateSystem(gPaEd.systems.Size() - 1);
			gPaEd.currentSystem = gPaEd.systems.Size() - 1;
			gPaEd.systems.GetLast().command.shouldSpawn = true;
		}
		if (ImGui::Button(" Import Existing System"))
		{
			SystemData settings;
			gPaEd.systems.Add(settings);
			Filebrowser::PushModal("Find System", 7569 << 16 | gPaEd.systems.Size() - 1);
			gPaEd.currentSystem = gPaEd.systems.Size() - 1;
			ParticleEditorInternal::CreateSystem(gPaEd.currentSystem);
			gPaEd.importingSystem = true;
		}
		if (gPaEd.currentSystem == gPaEd.systems.Size() - 1)
		{
			Filebrowser::FBResult res =
				Filebrowser::UpdateModal("Find System", 7569 << 16 | gPaEd.currentSystem, L"Content/ParticleSystems/", { L".gratsprut" });

			if (res.doubleClicked[0])
			{
				FixedString256 key = f::path(res.doubleClicked.Data()).filename().replace_extension().string();
				ParticleEditorInternal::ImportSystemInternal(key, false, false);
				gPaEd.importingSystem = false;
			}
		}

		ImGui::EndMenuBar();

		ImGui::Columns(2, 0, true);

		/* Viewport and Rendering */
		{
			(*gPaEd.renderTarget.texture)->ClearTexture({0, 0, 0, 255});
			(*gPaEd.renderTarget.depthTexture)->ClearDepth();
			
			if (gPaEd.systems.Size() > 0)
			{
				CameraControls(aDT, gPaEd.renderTarget.camera, gPaEd.cameraSettings, gPaEd.systemsPos);

				unsigned short numbPartsToRender = 0;
				unsigned short numbMeshPartsToRender = 0;
				for (size_t i = 0; i < gPaEd.systems.Size(); i++)
				{
					Windows::SystemData& sysData = gPaEd.systems[i];
					if (sysData.isPlaying)
					{
						Engine::UpdateParticleComponent(sysData.pEffect, sysData.emission, &sysData.renderCommand, &sysData.command, aDT);
						Engine::UpdateDepthFromCamera(&sysData.renderCommand, gPaEd.renderTarget.camera.GetMatrix().GetTranslationVector());
					}
					if (sysData.showSystem)
					{
						sysData.renderCommand.myTransform.SetTranslation(gPaEd.systemsPos);
						if (sysData.renderCommand.myIsMeshParticle && sysData.meshCommand.model)
						{
							m4f matrix;
							v4f min, max;
							CU::AABB3Df collider;
							sysData.meshCommand.numberOfModels = sysData.renderCommand.myAmountOfActiveVertices;
							for (unsigned int i = 0; i < sysData.renderCommand.myAmountOfActiveVertices; i++)
							{
								matrix = m4f::Identity;
								const Vertex_Particle& vert = sysData.renderCommand.myVertices[i];
								matrix.SetScale(v3f(vert.mySize.x, vert.mySize.y, vert.mySizeZ), false);
								matrix.SetRotation(v3f(vert.myRotationX, vert.myRotationY, vert.myRotation));
								matrix.SetTranslation(vert.myPosition);
								sysData.meshCommand.modelTransforms[i] = matrix;
								collider = sysData.meshCommand.model->myBoundingVolume;
								min = { collider.myMin, 1 };
								min = min * matrix;
								max = { collider.myMax, 1 };
								max = max * matrix;
								collider.myMin = { min.x, min.y, min.z };
								collider.myMax = { max.x, max.y, max.z };
								sysData.meshCommand.modelsColliders[i] = collider;
								sysData.meshCommand.modelsEffectData[i].tValue = vert.myLifetime / vert.myEndTime;
								sysData.meshCommand.modelsEffectData[i].gBufferPSEffectIndex = 140;
								sysData.meshCommand.modelsEffectData[i].effectColor = vert.myColor;
								sysData.meshCommand.modelsEffectData[i].effectColor.A_Normalized() *= vert.myEmissiveStrength;
							}
							gPaEd.meshRenderCommands[numbMeshPartsToRender++] = sysData.meshCommand;
						}
						else
						{
							gPaEd.renderCommands[numbPartsToRender++] = sysData.renderCommand;
						}
					}
				}
				EngineInterface::GetRenderer()->RenderModelParticleToResource(gPaEd.renderCommands, numbPartsToRender, gPaEd.meshRenderCommands, numbMeshPartsToRender, gPaEd.renderTarget.texture, gPaEd.renderTarget, gPaEd.lightRotateDirection.GetForward(), gPaEd.showGround);
			}
			else
			{
				//EngineInterface::GetRenderer()->RenderModelParticleToResource(&gPaEd.defaultCommand, 1, gPaEd.renderTarget.texture, gPaEd.renderTarget);
			}
			ImGui::Image(gPaEd.texture->GetSRV(), ImGui::GetContentRegionAvail());
		}

		ImGui::NextColumn();

		/* Properties */
		{
			v3f rot = gPaEd.lightRotateDirection.GetRotation();
			ImGui::DragFloat("Light Rotation", &rot.y, 1.f, -180, 180);
			gPaEd.lightRotateDirection.SetRotation(rot);

			ImGui::Checkbox("Move Systems", &gPaEd.moveSystems);
			ImGui::SameLine();
			ImGui::Checkbox("Show Ground", &gPaEd.showGround);
			if (ImGui::BeginTabBar("Particle Systems"))
			{
				for (size_t i = 0; i < gPaEd.systems.Size(); i++)
				{
					if (ImGui::BeginTabItem(gPaEd.systems[i].systemName, &gPaEd.systems[i].isUsed))
					{
						if (gPaEd.importingSystem == false)
						{
							gPaEd.currentSystem = i;
						}
						ParticleEditorInternal::ImGuiUpdate(aDT, i);
						ImGui::EndTabItem();
					}
					gPaEd.systems[i].command.myMatrix.SetTranslation(gPaEd.systemsPos);
				}
				ImGui::EndTabBar();
			}
			if (gPaEd.systems.Size() > 0)
			{
				for (size_t i = gPaEd.systems.Size(); i > 0; i--)
				{
					if (gPaEd.systems[i - 1].isUsed == false)
					{
						ParticleEditorInternal::DestroySystem(i - 1);
						gPaEd.systems.RemoveCyclicAtIndex(i - 1);
						if (gPaEd.currentSystem == gPaEd.systems.Size())
						{
							gPaEd.currentSystem = i - 1;
						}
					}
				}
			}
		}
		ParticleEditorInternal::SystemUpdate(aDT);

		ImGui::End();

		return result;
	}


	namespace ParticleEditorInternal
	{
		void ImGuiUpdate(float aDeltaTime, unsigned short aSystemIndex)
		{
			SystemData& system = gPaEd.systems[aSystemIndex];

			//if (ImGui::BeginPopupModal("Import System"))
			//{

			//	ImGui::Text("Use Existing : ");
			//	ImGui::SameLine();
			//	if (ImGui::Button("Import SubSystem"))
			//	{
			//		Filebrowser::PushModal("Find System", 7569 << 16 | aSystemIndex);
			//	}

			//	Filebrowser::FBResult res =
			//		Filebrowser::UpdateModal("Find System", 7569 << 16 | aSystemIndex, L"Content/ParticleSystems/", { L".gratsprut" });

			//	if (res.doubleClicked[0])
			//	{
			//		CreateSystem(gPaEd.currentSystem);
			//		FixedString256 key = f::path(res.doubleClicked.Data()).filename().replace_extension().string();
			//		ParticleEditorInternal::ImportSystemInternal(key, true, false, system.numbSubSystems - 1);
			//		gPaEd.importingSystem = false;
			//		system.loadingSubSystem = false;
			//	}
			//}
			//else
			//{
			if (gPaEd.blendingSystem)
			{
				BlendSystems();
			}
			else
			{
				ImGui::Checkbox("Show System", &system.showSystem);
				ImGui::SameLine();
				if (strlen(system.systemName.Data()) > 0)
				{
					if (ImGui::Button("Export System"))
					{
						if (!ExportSystem(aSystemIndex))
						{
							system.failedExport = true;
						}
					}
					ImGui::SameLine();
				}
				if (system.failedExport)
				{
					ImGui::OpenPopup("Export Error");
					if (ImGui::BeginPopupModal("Export Error"))
					{
						ImGui::Text("Failed To Export");
						if (ImGui::Button("Exit"))
						{
							system.failedExport = false;
						}
						ImGui::EndPopup();
					}
				}
				if (ImGui::Button("Clear System"))
				{
					ResetSystem(aSystemIndex, false);
				}

				ParticleCommand& instance = system.command;
				if (ImGui::Button("Restart System"))
				{
					system.emission.lifeTime = 0;
					system.emission.burstTimer = 0;
					system.emission.prevBurstTimer = 0;
					system.renderCommand.myAmountOfActiveVertices = 0;
				}
				if (system.isPlaying)
				{
					ImGui::SameLine();
					if (ImGui::Button("Pause System"))
					{
						system.isPlaying = false;
					}
				}
				else
				{
					ImGui::SameLine();
					if (ImGui::Button("Resume System"))
					{
						system.isPlaying = true;
					}
				}

				//UpdateSystem(aSystemIndex);
				SetImGuiOptions(aSystemIndex);
			}
			//}
		}
		void SystemUpdate(float aDeltaTime)
		{
			if (gPaEd.systems.Size() > 0)
			{
				if (gPaEd.moveSystems)
				{
					gPaEd.rotationValue += aDeltaTime;
					float x = cos(gPaEd.rotationValue) * 400.0f;
					float z = sin(gPaEd.rotationValue) * 400.0f;

					gPaEd.systemsPos = { x, 0, z };
				}
				else
				{
					gPaEd.systemsPos = { 0, 0, 0 };
				}
			}
		}
		void SetImGuiOptions(unsigned short aSystemIndex)
		{
			SystemData& system = gPaEd.systems[aSystemIndex];
			ParticleEffect::ParticleSettings& subSystem = system.pEffect.GetSettings();
			if (ImGui::TreeNode("Particle Settings"))
			{
				if (ImGui::InputTextWithHint("SubSystem Name", "Add the name of the final GratSprut here", gPaEd.inputSystemName, 256, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					system.systemName = gPaEd.inputSystemName;
					system.particleData.systemGUID = NIL_UUID;
				}
				if (ImGui::Button("Import Settings From File"))
				{
					Filebrowser::PushModal("Find System", 7569 << 16 | aSystemIndex);
					gPaEd.currentSystem = aSystemIndex;
					ImGui::TreePop();
					return;
				}
				if (gPaEd.currentSystem == aSystemIndex)
				{
					Filebrowser::FBResult res =
						Filebrowser::UpdateModal("Find System", 7569 << 16 | aSystemIndex, L"Content/ParticleSystems/", { L".gratsprut" });
					if (res.doubleClicked[0])
					{
						FixedString256 key = f::path(res.doubleClicked.Data()).filename().replace_extension().string();
						ParticleEditorInternal::ImportSystemInternal(key, true, true);
					}
				}

				if (ImGui::Checkbox("Mesh Particle", &subSystem.myIsMeshParticle))
				{
					if (subSystem.myIsMeshParticle)
					{
						subSystem.myIsBillboard = false;
					}
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("Particle Billboard", &subSystem.myIsBillboard))
				{
					if (subSystem.myIsBillboard)
					{
						subSystem.myIsMeshParticle = false;
					}
				}
				system.renderCommand.myIsMeshParticle = subSystem.myIsMeshParticle;
				if (subSystem.myIsMeshParticle)
				{
					if (ImGui::TreeNode("Mesh Editing"))
					{
						if (ImGui::Button("Chose Mesh"))
						{
							Filebrowser::PushModal("Find Mesh", 7117 << 16 | aSystemIndex);
							gPaEd.currentSystem = aSystemIndex;
						}
						if (aSystemIndex == gPaEd.currentSystem)
						{
							Filebrowser::FBResult res =
								Filebrowser::UpdateModal("Find Mesh", 7117 << 16 | aSystemIndex, L"Content/Models/Static/", { L".grat" });

							if (res.doubleClicked[0])
							{
								FixedString256 pathstr =
									FixedString256::Format("%S", f::path(res.doubleClicked.Data()).filename().replace_extension().c_str());
								UUID mdl = EngineInterface::GetModelManager()
									->GetModel(pathstr.Data(), true);

								if (mdl != NIL_UUID)
								{
									subSystem.myMeshGUID = mdl;
									system.meshName = pathstr;
									subSystem.myMeshName = system.meshName;
									subSystem.mySubmeshID = 0;
									system.model = EngineInterface::GetModelManager()->GetModel(subSystem.myMeshGUID);
									system.meshCommand.model = &system.model->GetModelData();
								}
							}
						}
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODELID"))
							{
								FixedWString256* str = (FixedWString256*)payload->Data;
								UUID mdl = EngineInterface::GetModelManager()
									->GetModel(FixedString256::Format("%S", *str).Data(), true);
								if (mdl != NIL_UUID)
								{
									subSystem.myMeshGUID = mdl;
									system.meshName = FixedString256::Format("%S", *str).Data();
									subSystem.myMeshName = system.meshName;
									subSystem.mySubmeshID = 0;
									system.model = EngineInterface::GetModelManager()->GetModel(subSystem.myMeshGUID);
									system.meshCommand.model = &system.model->GetModelData();
								}
							}
							ImGui::EndDragDropTarget();
						}
						if (subSystem.myMeshGUID != NIL_UUID)
						{
							ImGui::Text("Mesh: %s", system.meshName);

							ImGui::Text("Submesh: %s", system.model->GetModelData(subSystem.mySubmeshID).mySubModelName);
							if (system.model->GetAmountOfSubModels() > subSystem.mySubmeshID + 1)
							{
								ImGui::SameLine();
								if (ImGui::ArrowButton("Submesh Up", ImGuiDir_Up))
								{
									system.meshCommand.model = &system.model->GetModelData(++subSystem.mySubmeshID);
								}
							}
							if (subSystem.mySubmeshID > 0)
							{
								ImGui::SameLine();
								if (ImGui::ArrowButton("Submesh Down", ImGuiDir_Down))
								{
									system.meshCommand.model = &system.model->GetModelData(--subSystem.mySubmeshID);
								}
							}
						}

						ImGui::TreePop();
					}
				}

				ImGui::Checkbox("Burst Mode", &subSystem.myBurstMode);
				if (subSystem.myBurstMode)
				{
					ImGui::SameLine();
					if (ImGui::Button("Spawn Burst"))
					{
						system.emission.burstTimer = 0;
						system.emission.prevBurstTimer = 0;
						system.emission.lifeTime = 0;
					}
				}
				if (subSystem.myBurstMode)
				{
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("Burst Length", &subSystem.myBurstLength, 0.0f, 5.0f, "%.2f");
					ImGui::Checkbox("Continous Bursts", &subSystem.myIsContinouslyBursting);
					if (subSystem.myIsContinouslyBursting)
					{
						ImGui::SliderFloat("Time Between Bursts", &subSystem.myBurstSpaceTime, 0.0f, 25.0f, "%.2f");
					}
					ImGui::SliderFloat("Burst SpawnDelay Length", &subSystem.myBurstSpawnDelay, 0.0f, 10.0f, "%.2f");

				}
		
				//Colors

				if (ImGui::TreeNode("Colors"))
				{
					//Amount of Colors
					if (subSystem.myNumberOfColors < 5)
					{
						if (ImGui::Button("Add Color"))
						{
							subSystem.myColorBlendTimers[subSystem.myNumberOfColors] = subSystem.myColorBlendTimers[subSystem.myNumberOfColors - 1] + 0.05f;
							subSystem.myNumberOfColors++;
						}
					}
					if (subSystem.myNumberOfColors > 2)
					{
						if (ImGui::Button("Remove Color"))
						{
							subSystem.myNumberOfColors--;
							subSystem.myColorBlendTimers[subSystem.myNumberOfColors] = subSystem.myColorBlendTimers[subSystem.myNumberOfColors + 1] - 0.05f;
						}
					}

					for (unsigned int i = 0; i < subSystem.myNumberOfColors; i++)
					{
						std::string colorName = "Particle Color ";
						colorName += std::to_string(i);
						ImGui::ColorEdit4(colorName.c_str(), &subSystem.myColorsToBlendBetween[i][0]);
						std::string colorBLendName = "Blend Position ";
						colorBLendName += std::to_string(i);
						if (i == 0)
						{
							ImGui::SliderFloat(colorBLendName.c_str(), &subSystem.myColorBlendTimers[i], 0, subSystem.myColorBlendTimers[i + 1], "%.2f");
						}
						if (i != 0 && i < subSystem.myNumberOfColors - 1)
						{
							ImGui::SliderFloat(colorBLendName.c_str(), &subSystem.myColorBlendTimers[i], subSystem.myColorBlendTimers[i - 1], subSystem.myColorBlendTimers[i + 1], "%.2f");
						}
						if (i == subSystem.myNumberOfColors - 1)
						{
							ImGui::SliderFloat(colorBLendName.c_str(), &subSystem.myColorBlendTimers[i], subSystem.myColorBlendTimers[i - 1], 1.0f, "%.2f");
						}
						ImGui::Spacing();
					}
					ImGui::Checkbox("Change Emission Over Time", &subSystem.myUseCurvesEmission);
					if (subSystem.myUseCurvesEmission)
					{
						ImGui::SetNextItemWidth(100);
						ImGui::SliderFloat("Emissive Start Strength", &subSystem.myParticleEmissiveStrength, 0, 10.0f, "%.1f");
						ImGui::SetNextItemWidth(100);
						ImGui::SliderFloat("Emissive End Strength", &subSystem.myEmissiveEndStrength, 0, 10.0f, "%.1f");
						ImGui::Curve("Emission Over Time", { 400, 150 }, subSystem.myAmountOfEmissionPoints, &subSystem.myEmissionOverTimePoints[0], subSystem.myEmissiveCurveStrength);
					}
					else
					{
						ImGui::SetNextItemWidth(100);
						ImGui::SliderFloat("Emissive Strength", &subSystem.myParticleEmissiveStrength, 0, 10.0f, "%.1f");
					}

					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Speed and Forces"))
				{
					ImGui::SliderFloat("Particle Speed", &subSystem.myParticleSpeed, -250.0f, 250.0f, "%.2f");
					ImGui::Text("Particle Spawn Force : ");
					if (subSystem.mySpawnForceRandX)
					{
						ImGui::Checkbox("Mirror Force X", &subSystem.mySpawnForceMirrorX);
						ImGui::SameLine();
						if (subSystem.mySpawnForceMirrorX)
						{
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("X min / max", &subSystem.mySpawnForceMin.x, 0, 250.f, "%.2f");
						}
						else
						{
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("X min", &subSystem.mySpawnForceMin.x, -250.0f, 250.0f, "%.2f");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("X max", &subSystem.mySpawnForceMax.x, -250.0f, 250.0f, "%.2");
						}
						subSystem.mySpawnForceMax.x = CU::Max(subSystem.mySpawnForceMax.x, subSystem.mySpawnForceMin.x);
						subSystem.mySpawnForceMin.x = CU::Min(subSystem.mySpawnForceMax.x, subSystem.mySpawnForceMin.x);
					}
					else
					{
						ImGui::SliderFloat("X", &subSystem.myForce.x, -250.0f, 250.0f, "%.2f");
					}
					if (subSystem.mySpawnForceRandY)
					{
						ImGui::Checkbox("Mirror Force Y", &subSystem.mySpawnForceMirrorY);
						ImGui::SameLine();
						if (subSystem.mySpawnForceMirrorY)
						{
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("Y min / max", &subSystem.mySpawnForceMin.y, -250.0f, 250.f, "%.2f");
						}
						else
						{
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("Y min", &subSystem.mySpawnForceMin.y, -250.0f, 250.0f, "%.2f");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("Y max", &subSystem.mySpawnForceMax.y, -250.0f, 250.0f, "%.2f");
						}
						subSystem.mySpawnForceMax.y = CU::Max(subSystem.mySpawnForceMax.y, subSystem.mySpawnForceMin.y);
						subSystem.mySpawnForceMin.y = CU::Min(subSystem.mySpawnForceMax.y, subSystem.mySpawnForceMin.y);
					}
					else
					{
						ImGui::SliderFloat("Y", &subSystem.myForce.y, -250.0f, 250.0f, "%.2f");
					}
					if (subSystem.mySpawnForceRandZ)
					{
						ImGui::Checkbox("Mirror Force Z", &subSystem.mySpawnForceMirrorZ);
						ImGui::SameLine();
						if (subSystem.mySpawnForceMirrorZ)
						{
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("Z min / max", &subSystem.mySpawnForceMin.z, -250.0f, 250.f, "%.2f");
						}
						else
						{
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("Z min", &subSystem.mySpawnForceMin.z, -250.0f, 250.0f, "%.2f");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(150);
							ImGui::SliderFloat("Z max", &subSystem.mySpawnForceMax.z, -250.0f, 250.0f, "%.2f");
						}
						subSystem.mySpawnForceMax.z = CU::Max(subSystem.mySpawnForceMax.z, subSystem.mySpawnForceMin.z);
						subSystem.mySpawnForceMin.z = CU::Min(subSystem.mySpawnForceMax.z, subSystem.mySpawnForceMin.z);
					}
					else
					{
						ImGui::SliderFloat("Z", &subSystem.myForce.z, -250.0f, 250.0f, "%.2f");
					}



					ImGui::Checkbox("Normalize Forces", &subSystem.mySpawnForcesNormalized);
					ImGui::Text("Particle Drag : ");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("DragX", &subSystem.myDrag.x, -250.0f, 250.0f, "%.2f");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("DragY", &subSystem.myDrag.y, -250.0f, 250.0f, "%.2f");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("DragZ", &subSystem.myDrag.z, -250.0f, 250.0f, "%.2f");
					//const ImVec2 curveSize{ 200, 200 };
					//static ImVec2 curveTest[5] = { {0,0}, {0.25,0.25}, {0.5,0.5}, {0.75,0.75}, {1,1} };
					//static v2f curveTest2[5] = { {0,0}, {0.15,0.55}, {0.35,0.1}, {0.65,0.95}, {1,0.5} };
					//static int keyCounter = 2;
					//static float tTest = 1.0f;
					//ImGui::Curve("easeLinear", curveSize, keyCounter, curveTest2, tTest); }
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("System Offset"))
				{
					ImGui::Text("Offset: ");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("Offset X", &system.pEffect.GetSettings().myOffSetAsSubSystem.x, -250.0f, 250.0f, "%.1f");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("Offset Y", &system.pEffect.GetSettings().myOffSetAsSubSystem.y, -250.0f, 250.0f, "%.1f");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("Offset Z", &system.pEffect.GetSettings().myOffSetAsSubSystem.z, -250.0f, 250.0f, "%.1f");
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("LifeTime"))
				{
					ImGui::SetNextItemWidth(350);
					if (ImGui::SliderFloat("Particles Spawned / Second", &system.particleData.spawnedPerSecond, 0.0f, 500.0f, "%.0f"))
					{
						subSystem.mySpawnRate = system.particleData.spawnedPerSecond;
						ResetSystemEmission(aSystemIndex);
					}
					ImGui::SetNextItemWidth(350);
					if (ImGui::SliderFloat("Particle Life Min Time", &system.particleData.minLifeTime, 0.0f, subSystem.myParticleMaxLifeTime, "%.2f"))
					{
						if (system.particleData.minLifeTime >= subSystem.myParticleMaxLifeTime)
						{
							system.particleData.minLifeTime = subSystem.myParticleMaxLifeTime - 0.01f;
						}
						subSystem.myParticleMinLifeTime = system.particleData.minLifeTime;
						ResetSystemEmission(aSystemIndex);
					}
					ImGui::SetNextItemWidth(350);
					if (ImGui::SliderFloat("Particle Life Max Time", &system.particleData.maxLifeTime, subSystem.myParticleMinLifeTime, 60.0f, "%.2f"))
					{
						if (system.particleData.maxLifeTime <= subSystem.myParticleMinLifeTime)
						{
							system.particleData.maxLifeTime = subSystem.myParticleMinLifeTime + 0.01f;
						}
						subSystem.myParticleMaxLifeTime = system.particleData.maxLifeTime;
						ResetSystemEmission(aSystemIndex);
					}
					ImGui::TreePop();
				}
				//Rotation
				if (ImGui::TreeNode("Rotation"))
				{
					ImGui::Checkbox("Rotate Around Direction", &subSystem.myForwardIsDirection);
					if (subSystem.myForwardIsDirection == false)
					{
						if (subSystem.myIsBillboard == false)
						{
							ImGuiParticleRotationSettings("X", subSystem.myParticleSpawnMinRotationDirectionXY.x, subSystem.myParticleSpawnMaxRotationDirectionXY.x, subSystem.mySpawnParticleWithRandomRotationX, subSystem.myRotateXAxis);
							ImGuiParticleRotationSettings("Y", subSystem.myParticleSpawnMinRotationDirectionXY.y, subSystem.myParticleSpawnMaxRotationDirectionXY.y, subSystem.mySpawnParticleWithRandomRotationY, subSystem.myRotateYAxis);
						}
						ImGuiParticleRotationSettings("Z", subSystem.myParticleSpawnMinRotationDirectionZ, subSystem.myParticleSpawnMaxRotationDirectionZ, subSystem.mySpawnParticleWithRandomRotationZ, subSystem.myRotateZAxis);
						if (subSystem.myRotateXAxis || subSystem.myRotateYAxis || subSystem.myRotateZAxis)
						{
							ImGui::Checkbox("Particle Rotate RandomDirection", &subSystem.myRotateRandomRotation);
							if (subSystem.myRotateRandomRotation)
							{
								ImGui::SetNextItemWidth(75);
								ImGui::SliderFloat(("Particle Min Rotation"), &subSystem.myMinRotationSpeed, -10, subSystem.myMaxRotationSpeed, "%.2f");
								ImGui::SetNextItemWidth(75);
								ImGui::SliderFloat(("Particle Max Rotation"), &subSystem.myMaxRotationSpeed, subSystem.myMinRotationSpeed, 10, "%.2f");
							}
							else
							{
								ImGui::SetNextItemWidth(75);
								ImGui::SliderFloat(("Particle Rotation Direction"), &subSystem.myMaxRotationSpeed, -10, 10.0f, "%.2f");
							}
						}
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Scale"))
				{
					ImGui::Checkbox("Use Uniform Scale", &subSystem.myIsUniformScale);
					if (subSystem.myIsBillboard == false)
					{
						if (subSystem.myIsUniformScale == false)
						{
							ImGuiParticleScaleSettings("X", subSystem.myParticleSpawnSizeXY.x, subSystem.myParticleEndSizeXY.x);
							ImGuiParticleScaleSettings("Y", subSystem.myParticleSpawnSizeXY.y, subSystem.myParticleEndSizeXY.y);
							ImGuiParticleScaleSettings("Z", subSystem.myParticleSpawnSizeZ, subSystem.myParticleEndSizeZ);
						}
						else
						{
							ImGuiParticleScaleSettings("XYZ", subSystem.myParticleSpawnSizeZ, subSystem.myParticleEndSizeZ);
							subSystem.myParticleSpawnSizeXY = v2f(subSystem.myParticleSpawnSizeZ, subSystem.myParticleSpawnSizeZ);
							subSystem.myParticleEndSizeXY = v2f(subSystem.myParticleEndSizeZ, subSystem.myParticleEndSizeZ);
						}
					}
					else
					{
						if (subSystem.myIsUniformScale == false)
						{
							ImGuiParticleScaleSettings("X", subSystem.myParticleSpawnSizeXY.x, subSystem.myParticleEndSizeXY.x);
							ImGuiParticleScaleSettings("Y", subSystem.myParticleSpawnSizeXY.y, subSystem.myParticleEndSizeXY.y);
							subSystem.myParticleSpawnSizeZ = 1;
							subSystem.myParticleEndSizeZ = 1;
						}
						else
						{
							ImGuiParticleScaleSettings("XY", subSystem.myParticleSpawnSizeZ, subSystem.myParticleEndSizeZ);
							subSystem.myParticleSpawnSizeXY = v2f(subSystem.myParticleSpawnSizeZ, subSystem.myParticleSpawnSizeZ);
							subSystem.myParticleEndSizeXY = v2f(subSystem.myParticleEndSizeZ, subSystem.myParticleEndSizeZ);
						}
					}
					ImGui::Checkbox("Size Curve Over Time", &subSystem.myUseCurvesSize);
					if (subSystem.myUseCurvesSize)
					{
						ImGui::Curve("Size Over Time", { 400, 150 }, subSystem.myAmountOfSizePoints, & subSystem.mySizeOverTimePoints[0], subSystem.mySizeCurveStrength);
					}

					ImGui::TreePop();
				}




				const char* items[] = { "Point", "Box", "Sphere" };
				ImGui::Combo("Spawn Shape", &system.particleData.imGuiShape, items, IM_ARRAYSIZE(items));
				subSystem.myEmitterShape = (ParticleEffect::EmitterShape)system.particleData.imGuiShape;
				float x = subSystem.myBoxSize.x * 0.01f;
				float y = subSystem.myBoxSize.y * 0.01f;
				float z = subSystem.myBoxSize.z * 0.01f;
				float rad = subSystem.myRadius * 0.01f;
				switch (system.particleData.imGuiShape)
				{
				case 1:
					ImGui::Text("Box Bounds : ");

					ImGui::SameLine();
					ImGui::Checkbox("Show Bounds", &system.particleData.showShape);

					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("X-Bound", &x, 0.0f, 10.0f, "%.1f");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("Y-Bound", &y, 0.0f, 10.0f, "%.1f");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("Z-Bound", &z, 0.0f, 10.0f, "%.1f");
					subSystem.myBoxSize = { x * 100, y * 100, z * 100 };
					if (system.particleData.showShape)
					{
						EngineInterface::DrawBox(gPaEd.systemsPos + system.pEffect.GetSettings().myOffSetAsSubSystem, subSystem.myBoxSize, 1.5f);
					}
					break;
				case 2:
					ImGui::Text("Sphere Bounds : ");

					ImGui::SameLine();
					ImGui::Checkbox("Show Bounds", &system.particleData.showShape);

					ImGui::SetNextItemWidth(80);
					ImGui::SliderFloat("Radius", &rad, 0.0f, 10.0f, "%.1f");
					subSystem.myRadius = rad * 100;
					if (system.particleData.showShape)
					{
						EngineInterface::DrawSphere(gPaEd.systemsPos + system.pEffect.GetSettings().myOffSetAsSubSystem, subSystem.myRadius);
					}
					break;
				default:
					break;
				}

				ImGui::TreePop();
			}


			if(subSystem.myIsMeshParticle == false)
			{
				if (ImGui::TreeNode("Material Editing"))
				{
					if (ImGui::Button("Chose Material"))
					{
						Filebrowser::PushModal("Find material", 9001 << 16 | aSystemIndex);
						gPaEd.currentSystem = aSystemIndex;
					}
					if (aSystemIndex == gPaEd.currentSystem)
					{
						Filebrowser::FBResult res =
							Filebrowser::UpdateModal("Find material", 9001 << 16 | aSystemIndex, L"Content/Materials/", { L".gratplat" });

						if (res.doubleClicked[0])
						{
							FixedString256 type = f::path(res.doubleClicked.Data()).parent_path().parent_path().stem().string();

							if (strcmp(type, "Particle_Default") == 0)
							{
								system.particleData.myMaterialType = MaterialTypes::EParticle_Default;
							}
							if (strcmp(type, "Particle_Glow") == 0)
							{
								system.particleData.myMaterialType = MaterialTypes::EParticle_Glow;
							}
							system.particleData.myMaterialName = f::path(res.doubleClicked.Data()).filename().replace_extension().string();
							Engine::MaterialManager* matMan = EngineInterface::GetMaterialManager();
							system.pEffect.GetData().myMaterial = matMan->GetGratPlat(system.particleData.myMaterialName.Data(), system.particleData.myMaterialType);
							system.renderCommand.myMaterial = matMan->GetGratPlat(system.particleData.myMaterialName.Data(), system.particleData.myMaterialType);
						}
					}
					std::string materialTypeName = "Material : ";
					if (system.particleData.myMaterialType == MaterialTypes::EParticle_Default)
					{
						materialTypeName.append("Particle_Default");
					}
					if (system.particleData.myMaterialType == MaterialTypes::EParticle_Glow)
					{
						materialTypeName.append("Particle_Glow");
					}

					ImGui::SameLine();
					ImGui::Text(materialTypeName.c_str());
					ImGui::SameLine();
					ImGui::Text(system.particleData.myMaterialName);
					ImGui::TreePop();
				}
			}
		}

		void CreateSystem(unsigned short aSystemIndex)
		{
			SystemData& system = gPaEd.systems[aSystemIndex];
			HRESULT result;

			unsigned int amountOfParticles = 999999U;
			system.renderCommand.myVertices = new Vertex_Particle[amountOfParticles];
			D3D11_BUFFER_DESC particleVertexBufferDesc = { 0 };
			particleVertexBufferDesc.ByteWidth = sizeof(Vertex_Particle) * amountOfParticles;
			particleVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			particleVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			particleVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
			subresourceData.pSysMem = system.renderCommand.myVertices;
			ID3D11Buffer* vertexBuffer = nullptr;
			result = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&particleVertexBufferDesc, &subresourceData, &vertexBuffer);
			if (FAILED(result))
			{
				assert(false && "Failed to create Vertex Buffer");
			}
			system.pEffect.GetData().myParticleVertexBuffer = vertexBuffer;
			system.pEffect.GetData().myIsInited = true;
			system.pEffect.GetData().myNumberOfParticles = amountOfParticles;
			system.renderCommand.myStride = sizeof(Vertex_Particle);
			system.renderCommand.myParticleVertexBuffer = vertexBuffer;
			system.renderCommand.myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat("", MaterialTypes::EParticle_Default);
			system.pEffect.GetData().myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat("", MaterialTypes::EParticle_Default);
			system.renderCommand.myNumberOfParticles = amountOfParticles;
			system.isUsed = true;
			system.particleData.systemGUID = NIL_UUID;

			system.meshCommand.modelTransforms = new m4f[amountOfParticles];
			system.meshCommand.modelsColliders = new CU::AABB3Df[amountOfParticles];
			system.meshCommand.modelsEffectData = new ObjectEffectData[amountOfParticles];
			system.meshCommand.numberOfModels = amountOfParticles;

			system.meshCommand.modelEffectBuffer = gPaEd.myModelEffectBuffer;
			system.meshCommand.modelEffectSRV = gPaEd.myModelEffectSRV;
			system.meshCommand.modelOBToWorldBuffer = gPaEd.myModelOBToWorldBuffer;
			system.meshCommand.modelOBToWorldSRV = gPaEd.myModelOBToWorldSRV;

			system.pEffect.GetSettings().myEmissionOverTimePoints[1] = { 0.5f, 0.5f };
			system.pEffect.GetSettings().mySizeOverTimePoints[1] = { 0.5f, 0.5f };
			system.pEffect.GetSettings().myEmissionOverTimePoints[2] = { 1.f, 1.f };
			system.pEffect.GetSettings().mySizeOverTimePoints[2] = { 1.f, 1.f };
		}

		void ResetSystem(unsigned short aSystemIndex, bool aResetName)
		{
			SystemData& system = gPaEd.systems[aSystemIndex];
			system.pEffect.GetSettings() = ParticleEffect::ParticleSettings();
			system.pEffect.GetData().myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat("", MaterialTypes::EParticle_Default);
			system.renderCommand.myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat("", MaterialTypes::EParticle_Default);
			system.command = ParticleCommand();
			system.renderCommand.myAmountOfActiveVertices = 0;
			ResetSystemEmission(aSystemIndex);
			system.particleData.imGuiShape = 0;
			system.particleData.myMaterialType = MaterialTypes::EParticle_Default;
			system.particleData.myMaterialName = "";
			system.particleData.systemGUID = NIL_UUID;
			if (aResetName)
			{
				system.systemName = FixedString256::Format("Default : %i", aSystemIndex + 1);;
			}
		}

		static unsigned int versionNumb = 6;
		bool ExportSystem(unsigned short aSystemIndex)
		{
			struct SystemExportStruct
			{
				SystemExportStruct& operator= (const ParticleEffect::ParticleSettings& aSettingToCopy)
				{
					myNumberOfColors = aSettingToCopy.myNumberOfColors;
					for (unsigned int i = 0; i < 5; i++)
					{
						myColorBlendTimers[i] = aSettingToCopy.myColorBlendTimers[i];
					}
					for (unsigned int i = 0; i < 5; i++)
					{
						myParticleColors[i * 4] = aSettingToCopy.myColorsToBlendBetween[i].Get_R_Normalized();
						myParticleColors[1 + i * 4] = aSettingToCopy.myColorsToBlendBetween[i].Get_G_Normalized();
						myParticleColors[2 + i * 4] = aSettingToCopy.myColorsToBlendBetween[i].Get_B_Normalized();
						myParticleColors[3 + i * 4] = aSettingToCopy.myColorsToBlendBetween[i].Get_A_Normalized();
					}
					myForce[0] = aSettingToCopy.myForce.x;
					myForce[1] = aSettingToCopy.myForce.y;
					myForce[2] = aSettingToCopy.myForce.z;
					myDrag[0] = aSettingToCopy.myDrag.x;
					myDrag[1] = aSettingToCopy.myDrag.y;
					myDrag[2] = aSettingToCopy.myDrag.z;
					myBoxSize[0] = aSettingToCopy.myBoxSize.x;
					myBoxSize[1] = aSettingToCopy.myBoxSize.y;
					myBoxSize[2] = aSettingToCopy.myBoxSize.z;

					mySpawnRate = aSettingToCopy.mySpawnRate;
					myParticleSpeed = aSettingToCopy.myParticleSpeed;
					myParticleSpawnSize = aSettingToCopy.myParticleSpawnSizeZ;
					myParticleEndSize = aSettingToCopy.myParticleEndSizeZ;
					myParticleEmissiveStrength = aSettingToCopy.myParticleEmissiveStrength;
					myParticleMinLifeTime = aSettingToCopy.myParticleMinLifeTime;
					myParticleMaxLifeTime = aSettingToCopy.myParticleMaxLifeTime;
					myParticleSpawnMinRotationDirection = aSettingToCopy.myParticleSpawnMinRotationDirectionZ;
					myParticleSpawnMaxRotationDirection = aSettingToCopy.myParticleSpawnMaxRotationDirectionZ;
					myMinRotationSpeed = aSettingToCopy.myMinRotationSpeed;
					myMaxRotationSpeed = aSettingToCopy.myMaxRotationSpeed;
					mySpawnParticleWithRandomRotation = (unsigned int)aSettingToCopy.mySpawnParticleWithRandomRotationZ;
					myRotateRandomRotation = (unsigned int)aSettingToCopy.myRotateRandomRotation;
					myBurstLength = aSettingToCopy.myBurstLength;
					myBurstSpaceTime = aSettingToCopy.myBurstSpaceTime;
					myBurstMode = (unsigned int)aSettingToCopy.myBurstMode;
					myIsContinouslyBursting = (unsigned int)aSettingToCopy.myIsContinouslyBursting;
					myRadius = aSettingToCopy.myRadius;
					myEmitterShape = (unsigned int)aSettingToCopy.myEmitterShape;

					myIsBillboard = aSettingToCopy.myIsBillboard;
					myIsMeshParticle = aSettingToCopy.myIsMeshParticle;
					myMeshGUID = aSettingToCopy.myMeshGUID;
					mySubmeshID = aSettingToCopy.mySubmeshID;
					myParticleSpawnMinRotationDirectionXY[0] = aSettingToCopy.myParticleSpawnMinRotationDirectionXY.x;
					myParticleSpawnMinRotationDirectionXY[1] = aSettingToCopy.myParticleSpawnMinRotationDirectionXY.y;
					myParticleSpawnMaxRotationDirectionXY[0] = aSettingToCopy.myParticleSpawnMaxRotationDirectionXY.x;
					myParticleSpawnMaxRotationDirectionXY[1] = aSettingToCopy.myParticleSpawnMaxRotationDirectionXY.y;

					mySpawnParticleWithRandomRotationX = aSettingToCopy.mySpawnParticleWithRandomRotationX;
					mySpawnParticleWithRandomRotationY = aSettingToCopy.mySpawnParticleWithRandomRotationY;

					myParticleSpawnSizeXY[0] = aSettingToCopy.myParticleSpawnSizeXY.x;
					myParticleSpawnSizeXY[1] = aSettingToCopy.myParticleSpawnSizeXY.y;
					myParticleEndSizeXY[0] = aSettingToCopy.myParticleEndSizeXY.x;
					myParticleEndSizeXY[1] = aSettingToCopy.myParticleEndSizeXY.y;
					myIsUniformScale = aSettingToCopy.myIsUniformScale;

					mySpawnForceMin[0] = aSettingToCopy.mySpawnForceMin.x;
					mySpawnForceMin[1] = aSettingToCopy.mySpawnForceMin.y;
					mySpawnForceMin[2] = aSettingToCopy.mySpawnForceMin.z;
					mySpawnForceMax[0] = aSettingToCopy.mySpawnForceMax.x;
					mySpawnForceMax[1] = aSettingToCopy.mySpawnForceMax.y;
					mySpawnForceMax[2] = aSettingToCopy.mySpawnForceMax.z;
					mySpawnForceRandX = aSettingToCopy.mySpawnForceRandX;
					mySpawnForceRandY = aSettingToCopy.mySpawnForceRandY;
					mySpawnForceRandZ = aSettingToCopy.mySpawnForceRandZ;
					mySpawnForceMirrorX = aSettingToCopy.mySpawnForceMirrorX;
					mySpawnForceMirrorY = aSettingToCopy.mySpawnForceMirrorY;
					mySpawnForceMirrorZ = aSettingToCopy.mySpawnForceMirrorZ;


					myRotateRandomRotationX = aSettingToCopy.myRotateRandomRotationX;
					myRotateRandomRotationY = aSettingToCopy.myRotateRandomRotationY;
					myRotateXAxis = aSettingToCopy.myRotateXAxis;
					myRotateYAxis = aSettingToCopy.myRotateYAxis;
					myRotateZAxis = aSettingToCopy.myRotateZAxis;
					myForwardIsDirection = aSettingToCopy.myForwardIsDirection;
					myUseCurvesEmission = aSettingToCopy.myUseCurvesEmission;
					myAmountOfEmissionPoints = aSettingToCopy.myAmountOfEmissionPoints;
					for (unsigned int i = 0; i < myAmountOfEmissionPoints; i++)
					{
						myEmissionOverTimePoints[i * 2] = aSettingToCopy.myEmissionOverTimePoints[i].x;
						myEmissionOverTimePoints[i * 2 + 1] = aSettingToCopy.myEmissionOverTimePoints[i].y;
					}
					myEmissiveCurveStrength = aSettingToCopy.myEmissiveCurveStrength;
					myEmissiveEndStrength = aSettingToCopy.myEmissiveEndStrength;
					myUseCurvesSize = aSettingToCopy.myUseCurvesSize;
					myAmountOfSizePoints = aSettingToCopy.myAmountOfSizePoints;
					for (unsigned int i = 0; i < myAmountOfSizePoints; i ++)
					{
						mySizeOverTimePoints[i * 2] = aSettingToCopy.mySizeOverTimePoints[i].x;
						mySizeOverTimePoints[i * 2 + 1] = aSettingToCopy.mySizeOverTimePoints[i].y;
					}
					mySizeCurveStrength = aSettingToCopy.mySizeCurveStrength;
					myBurstSpawnDelay = aSettingToCopy.myBurstSpawnDelay;
					strcpy_s(modelname, aSettingToCopy.myMeshName);
					mySpawnForcesNormalized = aSettingToCopy.mySpawnForcesNormalized;
					return(*this);
				}
				unsigned int myNumberOfColors{};
				float myColorBlendTimers[5]{ 0 };
				float myParticleColors[20]{ 0 };
				float myForce[3]{ 0 };
				float myDrag[3]{ 0 };
				float myBoxSize[3]{ 0 };
				float myOffset[3]{ 0 };
				float mySpawnRate{};
				float mySpawnAngle{};
				float myParticleSpeed{};
				float myParticleSpawnSize{};
				float myParticleEndSize{};
				float myParticleEmissiveStrength{};
				float myParticleMinLifeTime{};
				float myParticleMaxLifeTime{};
				float myParticleSpawnMinRotationDirection{};
				float myParticleSpawnMaxRotationDirection{};
				float myParticleRotationSpeed{};
				float myMinRotationSpeed{};
				float myMaxRotationSpeed{};
				unsigned int mySpawnParticleWithRandomRotation{};
				unsigned int myRotateRandomRotation{};

				float myBurstLength{};
				float myBurstSpaceTime{};
				unsigned int myBurstMode{};
				unsigned int myIsContinouslyBursting{};
				float myRadius{};

				unsigned int myEmitterShape{};
				unsigned int myNumberOfCharactersInName{};
				char mySystemName[128]{ 0 };
				unsigned int myNumberOfCharactersInMaterialName{};
				char myMaterialName[128]{ 0 };
				unsigned int myMaterialType{};

				GUID mySystemID = NIL_UUID;
				//
				bool myIsBillboard = true;
				bool myIsMeshParticle = false;

				GUID myMeshGUID = NIL_UUID;
				unsigned short mySubmeshID = 0;
				float myParticleSpawnMinRotationDirectionXY[2];
				float myParticleSpawnMaxRotationDirectionXY[2];
				bool mySpawnParticleWithRandomRotationX = false;
				bool mySpawnParticleWithRandomRotationY = false;
				float myParticleSpawnSizeXY[2] = {1, 1};
				float myParticleEndSizeXY[2] = {1.5f, 1.5f};
				bool myIsUniformScale = false;
				float mySpawnForceMin[3] = { -10, -10, -10 };
				float mySpawnForceMax[3] = { 10, 10, 10 };
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
				float myEmissionOverTimePoints[40];
				float myEmissiveCurveStrength = 1.0;
				float myEmissiveEndStrength = 1.0;
				bool myUseCurvesSize = false;
				int myAmountOfSizePoints = 3;
				float mySizeCurveStrength = 1.0;
				float mySizeOverTimePoints[40];
				float myBurstSpawnDelay = 0.f;
				char modelname[256];
				bool mySpawnForcesNormalized = false;
			} exportStruct;

			exportStruct = SystemExportStruct();
			SystemData& system = gPaEd.systems[aSystemIndex];
			std::string outpath = "Content/ParticleSystems/";
			std::filesystem::create_directories(outpath);
			std::string name = system.systemName.Data();

			if (strstr(name.c_str(), "Default") != NULL)
			{
				assert(false && "Don't try to save a file with no name dude, you're gonna get a corrupted file that's very difficult to remove.");
				return false;
			}

			std::string systemPath = "Content/ParticleSystems/";
			bool foundSystem = false;
			for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(systemPath))
			{
				std::string filename = dirEntry.path().filename().replace_extension().string();
				if (dirEntry.is_regular_file() && strcmp(filename.c_str(), name.c_str()) == 0)
				{
					outpath = dirEntry.path().relative_path().string().c_str();
					foundSystem = true;
					if (system.particleData.systemGUID == NIL_UUID)
					{
						return false;
					}
					break;
				}
			}
			if (foundSystem)
			{
				if (std::filesystem::exists(outpath.c_str()))
				{
					std::filesystem::permissions(outpath.c_str(), std::filesystem::perms::all);
					std::filesystem::remove(outpath.c_str());
				}
			}
			else
			{
				outpath = systemPath + name + ".gratsprut";
			}

			std::ofstream outMaterial;
			outMaterial.open(outpath, std::ios::out | std::ios::binary);

			unsigned int materialVersionIndex = versionNumb;
			outMaterial.write((char*)&materialVersionIndex, sizeof(unsigned int));

			exportStruct = system.pEffect.GetSettings();
			exportStruct.mySystemID = system.particleData.systemGUID;
			memcpy(&exportStruct.myOffset[0], &system.pEffect.GetSettings().myOffSetAsSubSystem[0], sizeof(v3f));
			exportStruct.myNumberOfCharactersInName = 0;

			for (unsigned short c = 0; c < 128; c++)
			{
				if (c < strlen(system.systemName))
				{
					exportStruct.myNumberOfCharactersInName += 1;
					exportStruct.mySystemName[c] = system.systemName[c];
				}
				else
				{
					exportStruct.mySystemName[c] = 0;
				}
			}
			exportStruct.myNumberOfCharactersInMaterialName = 0;
			for (unsigned short c = 0; c < 128; c++)
			{
				if (c < strlen(system.particleData.myMaterialName))
				{
					exportStruct.myNumberOfCharactersInMaterialName += 1;
					exportStruct.myMaterialName[c] = system.particleData.myMaterialName[c];
				}
				else
				{
					exportStruct.myMaterialName[c] = 0;
				}
			}
			exportStruct.myMaterialType = (unsigned int)system.particleData.myMaterialType;
			if (exportStruct.mySystemID == NIL_UUID)
			{
				UuidCreate(&exportStruct.mySystemID);
				system.particleData.systemGUID = exportStruct.mySystemID;
			}
			outMaterial.write((char*)&exportStruct, sizeof(SystemExportStruct));
			outMaterial.close();
			return true;
		}
		void ImportSystemInternal(const char* aGratkey, bool aGetSettings, bool aShouldBlend)
		{
			ParticleEffect::ParticleSettings settings;
			FixedString256 materialName;
			MaterialTypes materialType;
			SystemData& system = gPaEd.systems[gPaEd.currentSystem];
			if (ReadGratSprut(aGratkey, &settings, materialName.Data(), (unsigned int&)materialType, system.particleData.systemGUID))
			{
				if (aGetSettings)
				{
					system.particleData.blendSettings = settings;
					if (aShouldBlend)
					{
						gPaEd.blendingSystem = true;
						system.particleData.choosingBlendSystem = true;
					}
					else
					{
						system.pEffect.GetSettings() = settings;
					}
					system.systemName = aGratkey;
				}
				else
				{
					system.pEffect.GetSettings() = settings;
					system.particleData.blendSettings = settings;
					system.systemName = aGratkey;
				}
				//Getting CommonSettings
				system.particleData.myMaterialName = materialName;
				system.particleData.myMaterialType = materialType;
				system.particleData.imGuiShape = settings.myEmitterShape;
				system.renderCommand.myMaterial = EngineInterface::GetManagers()->myMaterialManager.GetGratPlat(materialName.Data(), materialType);
				system.pEffect.GetData().myMaterial = EngineInterface::GetManagers()->myMaterialManager.GetGratPlat(materialName.Data(), materialType);


				system.particleData.spawnedPerSecond = system.pEffect.GetSettings().mySpawnRate;
				system.particleData.minLifeTime = system.pEffect.GetSettings().myParticleMinLifeTime;
				system.particleData.maxLifeTime = system.pEffect.GetSettings().myParticleMaxLifeTime;

				if (settings.myIsMeshParticle)
				{
					system.model = EngineInterface::GetModelManager()->GetModel(settings.myMeshGUID);
					system.meshName = settings.myMeshName;
					system.model = EngineInterface::GetModelManager()->GetModel(settings.myMeshGUID);
					system.meshCommand.model = &system.model->GetModelData(settings.mySubmeshID);
				}
			}
		}
		bool ReadGratSprut(const char* aGratKey, void* someDataToFill, char* aMaterialName, unsigned int& aMatType, GUID& aGUIDToRead)
		{
			std::ifstream iMD;
			std::filesystem::path currentPath = std::filesystem::current_path();
			std::wstring outpathAbs = currentPath;
			std::string sKey = aGratKey;
			std::wstring key(sKey.begin(), sKey.end());
			outpathAbs.append(L"/Content/ParticleSystems/" + key + L".gratsprut");

			iMD.open(outpathAbs, std::ios::in | std::ios::binary);

			if (iMD)
			{
				ParticleEffect::ParticleSettings* settingsToFill = (ParticleEffect::ParticleSettings*)someDataToFill;
				unsigned int materialVersionIndex;
				iMD.read((char*)&materialVersionIndex, sizeof(materialVersionIndex));
				if (materialVersionIndex == 1)
				{
					struct ParticleImportStruct
					{
						float startColor[4];
						float endColor[4];
						float force[3];
						float spawnRate{};

						float spawnAngle{};
						float speed{};
						float spawnSize{};
						float endSize{};

						float emissiveStrength{};
						float minLifeTime{};
						float maxLifeTime{};
						unsigned int emitterShape;

						unsigned int numberOfCharactersInName;
						char systemName[128];
						unsigned int numberOfCharactersInMaterialName;
						char materialName[128];
						unsigned int materialType;
					}data;
					iMD.read((char*)&data, sizeof(ParticleImportStruct));
					iMD.close();
					settingsToFill->myColorsToBlendBetween[0].myVector.x = data.startColor[0];
					settingsToFill->myColorsToBlendBetween[0].myVector.y = data.startColor[1];
					settingsToFill->myColorsToBlendBetween[0].myVector.z = data.startColor[2];
					settingsToFill->myColorsToBlendBetween[0].myVector.w = data.startColor[3];
					settingsToFill->myColorsToBlendBetween[1].myVector.x = data.endColor[0];
					settingsToFill->myColorsToBlendBetween[1].myVector.y = data.endColor[1];
					settingsToFill->myColorsToBlendBetween[1].myVector.z = data.endColor[2];
					settingsToFill->myColorsToBlendBetween[1].myVector.w = data.endColor[3];
					settingsToFill->myColorBlendTimers[0] = 0.1f;
					settingsToFill->myColorBlendTimers[1] = 0.9f;
					settingsToFill->myForce = { data.force[0], data.force[1], data.force[2] };
					settingsToFill->myForce *= 1000.0f;
					settingsToFill->mySpawnRate = data.spawnRate;
					settingsToFill->myParticleSpeed = data.speed;
					settingsToFill->myParticleSpawnSizeZ = data.spawnSize;
					settingsToFill->myParticleEndSizeZ = data.endSize;
					settingsToFill->myParticleEmissiveStrength = data.emissiveStrength;
					settingsToFill->myParticleMinLifeTime = data.minLifeTime;
					settingsToFill->myParticleMaxLifeTime = data.maxLifeTime;
					settingsToFill->myEmitterShape = (ParticleEffect::EmitterShape)data.emitterShape;

					memcpy(aMaterialName, &data.materialName, data.numberOfCharactersInMaterialName);
					aMatType = data.materialType;
				}
				if (materialVersionIndex == 2)
				{
					struct ParticleImportStruct
					{
						unsigned int myNumberOfColors{};
						float myColorBlendTimers[5]{ 0 };
						float myParticleColors[20]{ 0 };
						float myForce[3]{ 0 };
						float myDrag[3]{ 0 };
						float myBoxSize[3]{ 0 };
						float myOffset[3]{ 0 };
						float mySpawnRate{};
						float mySpawnAngle{};
						float myParticleSpeed{};
						float myParticleSpawnSize{};
						float myParticleEndSize{};
						float myParticleEmissiveStrength{};
						float myParticleMinLifeTime{};
						float myParticleMaxLifeTime{};
						float myParticleSpawnMinRotationDirection{};
						float myParticleSpawnMaxRotationDirection{};
						float myParticleRotationSpeed{};
						float myMinRotationSpeed{};
						float myMaxRotationSpeed{};
						unsigned int mySpawnParticleWithRandomRotation{};
						unsigned int myRotateRandomRotation{};

						float myBurstLength{};
						float myBurstSpaceTime{};
						unsigned int myBurstMode{};
						unsigned int myIsContinouslyBursting{};
						float myRadius{};

						unsigned int myEmitterShape{};
						unsigned int myNumberOfCharactersInName{};
						char mySystemName[128]{ 0 };
						unsigned int myNumberOfCharactersInMaterialName{};
						char myMaterialName[128]{ 0 };
						unsigned int myMaterialType{};
					}data;
					unsigned int numberOfSubEmitters = 0;
					iMD.read((char*)&numberOfSubEmitters, sizeof(unsigned int));

					iMD.read((char*)&data, sizeof(ParticleImportStruct));
					iMD.close();
					ParticleEffect::ParticleSettings* settingsToFill = (ParticleEffect::ParticleSettings*)someDataToFill;
					settingsToFill->myNumberOfColors = data.myNumberOfColors;
					for (unsigned int i = 0; i < data.myNumberOfColors; i++)
					{
						settingsToFill->myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
						settingsToFill->myColorBlendTimers[i] = data.myColorBlendTimers[i];
					}
					settingsToFill->myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
					settingsToFill->myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
					settingsToFill->myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
					settingsToFill->myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

					settingsToFill->mySpawnRate = data.mySpawnRate;
					settingsToFill->myParticleSpeed = data.myParticleSpeed;
					settingsToFill->myParticleSpawnSizeZ = data.myParticleSpawnSize;
					settingsToFill->myParticleEndSizeZ = data.myParticleEndSize;
					settingsToFill->myParticleEmissiveStrength = data.myParticleEmissiveStrength;
					settingsToFill->myParticleMinLifeTime = data.myParticleMinLifeTime;
					settingsToFill->myParticleMaxLifeTime = data.myParticleMaxLifeTime;
					settingsToFill->myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
					settingsToFill->myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
					settingsToFill->myMinRotationSpeed = data.myMinRotationSpeed;
					settingsToFill->myMaxRotationSpeed = data.myMaxRotationSpeed;
					settingsToFill->mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
					settingsToFill->myRotateRandomRotation = data.myRotateRandomRotation;
					settingsToFill->myBurstLength = data.myBurstLength;
					settingsToFill->myBurstSpaceTime = data.myBurstSpaceTime;
					settingsToFill->myBurstMode = (bool)data.myBurstMode;
					settingsToFill->myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
					settingsToFill->myRadius = data.myRadius;
					settingsToFill->myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;
					memcpy(aMaterialName, &data.myMaterialName, data.myNumberOfCharactersInMaterialName);
					aMaterialName[data.myNumberOfCharactersInMaterialName] = 0;
					aMatType = data.myMaterialType;
		
				}
				if (materialVersionIndex == 3)
				{
					struct ParticleImportStruct
					{
						unsigned int myNumberOfColors{};
						float myColorBlendTimers[5]{ 0 };
						float myParticleColors[20]{ 0 };
						float myForce[3]{ 0 };
						float myDrag[3]{ 0 };
						float myBoxSize[3]{ 0 };
						float myOffset[3]{ 0 };
						float mySpawnRate{};
						float mySpawnAngle{};
						float myParticleSpeed{};
						float myParticleSpawnSize{};
						float myParticleEndSize{};
						float myParticleEmissiveStrength{};
						float myParticleMinLifeTime{};
						float myParticleMaxLifeTime{};
						float myParticleSpawnMinRotationDirection{};
						float myParticleSpawnMaxRotationDirection{};
						float myParticleRotationSpeed{};
						float myMinRotationSpeed{};
						float myMaxRotationSpeed{};
						unsigned int mySpawnParticleWithRandomRotation{};
						unsigned int myRotateRandomRotation{};

						float myBurstLength{};
						float myBurstSpaceTime{};
						unsigned int myBurstMode{};
						unsigned int myIsContinouslyBursting{};
						float myRadius{};

						unsigned int myEmitterShape{};
						unsigned int myNumberOfCharactersInName{};
						char mySystemName[128]{ 0 };
						unsigned int myNumberOfCharactersInMaterialName{};
						char myMaterialName[128]{ 0 };
						unsigned int myMaterialType{};

						unsigned int myNumberOfSubEmitters{};
						unsigned int myNumberOfCharactersInSubEmitsName[4]{ 0 };
						char mySubEmitNames[4][128]{ 0 };
						GUID mySystemUUID = NIL_UUID;
					}data;
					unsigned int numberOfSubEmitters = 0;

					iMD.read((char*)&numberOfSubEmitters, sizeof(unsigned int));

					iMD.read((char*)&data, sizeof(ParticleImportStruct));
					iMD.close();
					ParticleEffect::ParticleSettings* settingsToFill = (ParticleEffect::ParticleSettings*)someDataToFill;
					settingsToFill->myNumberOfColors = data.myNumberOfColors;
					for (unsigned int i = 0; i < data.myNumberOfColors; i++)
					{
						settingsToFill->myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
						settingsToFill->myColorBlendTimers[i] = data.myColorBlendTimers[i];
					}
					settingsToFill->myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
					settingsToFill->myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
					settingsToFill->myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
					settingsToFill->myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

					settingsToFill->mySpawnRate = data.mySpawnRate;
					settingsToFill->myParticleSpeed = data.myParticleSpeed;
					settingsToFill->myParticleSpawnSizeZ = data.myParticleSpawnSize;
					settingsToFill->myParticleEndSizeZ = data.myParticleEndSize;
					settingsToFill->myParticleEmissiveStrength = data.myParticleEmissiveStrength;
					settingsToFill->myParticleMinLifeTime = data.myParticleMinLifeTime;
					settingsToFill->myParticleMaxLifeTime = data.myParticleMaxLifeTime;
					settingsToFill->myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
					settingsToFill->myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
					settingsToFill->myMinRotationSpeed = data.myMinRotationSpeed;
					settingsToFill->myMaxRotationSpeed = data.myMaxRotationSpeed;
					settingsToFill->mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
					settingsToFill->myRotateRandomRotation = data.myRotateRandomRotation;
					settingsToFill->myBurstLength = data.myBurstLength;
					settingsToFill->myBurstSpaceTime = data.myBurstSpaceTime;
					settingsToFill->myBurstMode = (bool)data.myBurstMode;
					settingsToFill->myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
					settingsToFill->myRadius = data.myRadius;
					settingsToFill->myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;

					memcpy(aMaterialName, &data.myMaterialName, data.myNumberOfCharactersInMaterialName);
					aMaterialName[data.myNumberOfCharactersInMaterialName] = 0;
					aMatType = data.myMaterialType;
					aGUIDToRead = data.mySystemUUID;
				}
				if (materialVersionIndex == 4)
				{
					struct ParticleImportStruct
					{
						unsigned int myNumberOfColors{};
						float myColorBlendTimers[5]{ 0 };
						float myParticleColors[20]{ 0 };
						float myForce[3]{ 0 };
						float myDrag[3]{ 0 };
						float myBoxSize[3]{ 0 };
						float myOffset[3]{ 0 };
						float mySpawnRate{};
						float mySpawnAngle{};
						float myParticleSpeed{};
						float myParticleSpawnSize{};
						float myParticleEndSize{};
						float myParticleEmissiveStrength{};
						float myParticleMinLifeTime{};
						float myParticleMaxLifeTime{};
						float myParticleSpawnMinRotationDirection{};
						float myParticleSpawnMaxRotationDirection{};
						float myParticleRotationSpeed{};
						float myMinRotationSpeed{};
						float myMaxRotationSpeed{};
						unsigned int mySpawnParticleWithRandomRotation{};
						unsigned int myRotateRandomRotation{};

						float myBurstLength{};
						float myBurstSpaceTime{};
						unsigned int myBurstMode{};
						unsigned int myIsContinouslyBursting{};
						float myRadius{};

						unsigned int myEmitterShape{};
						unsigned int myNumberOfCharactersInName{};
						char mySystemName[128]{ 0 };
						unsigned int myNumberOfCharactersInMaterialName{};
						char myMaterialName[128]{ 0 };
						unsigned int myMaterialType{};

						GUID mySystemUUID = NIL_UUID;

						bool myIsBillboard = true;
						bool myIsMeshParticle = false;
						GUID myMeshGUID = NIL_UUID;
						unsigned short mySubmeshID = 0;
						float myParticleSpawnMinRotationDirectionXY[2];
						float myParticleSpawnMaxRotationDirectionXY[2];
						bool mySpawnParticleWithRandomRotationX = false;
						bool mySpawnParticleWithRandomRotationY = false;
						float myParticleSpawnSizeXY[2] = { 1, 1 };
						float myParticleEndSizeXY[2] = { 1.5f, 1.5f };
						bool myIsUniformScale = false;
						float mySpawnForceMin[3] = { -10, -10, -10 };
						float mySpawnForceMax[3] = { 10, 10, 10 };
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
						float myEmissionOverTimePoints[40];
						float myEmissiveCurveStrength = 1.0;
						float myEmissiveEndStrength = 1.0;
						bool myUseCurvesSize = false;
						int myAmountOfSizePoints = 3;
						float mySizeCurveStrength = 1.0;
						float mySizeOverTimePoints[40];
						float myBurstSpawnDelay = 0.f;
					}data;

					iMD.read((char*)&data, sizeof(ParticleImportStruct));
					iMD.close();
					ParticleEffect::ParticleSettings* settingsToFill = (ParticleEffect::ParticleSettings*)someDataToFill;
					settingsToFill->myNumberOfColors = data.myNumberOfColors;
					for (unsigned int i = 0; i < data.myNumberOfColors; i++)
					{
						settingsToFill->myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
						settingsToFill->myColorBlendTimers[i] = data.myColorBlendTimers[i];
					}
					settingsToFill->myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
					settingsToFill->myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
					settingsToFill->myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
					settingsToFill->myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

					settingsToFill->mySpawnRate = data.mySpawnRate;
					settingsToFill->myParticleSpeed = data.myParticleSpeed;
					settingsToFill->myParticleSpawnSizeZ = data.myParticleSpawnSize;
					settingsToFill->myParticleEndSizeZ = data.myParticleEndSize;
					settingsToFill->myParticleEmissiveStrength = data.myParticleEmissiveStrength;
					settingsToFill->myParticleMinLifeTime = data.myParticleMinLifeTime;
					settingsToFill->myParticleMaxLifeTime = data.myParticleMaxLifeTime;
					settingsToFill->myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
					settingsToFill->myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
					settingsToFill->myMinRotationSpeed = data.myMinRotationSpeed;
					settingsToFill->myMaxRotationSpeed = data.myMaxRotationSpeed;
					settingsToFill->mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
					settingsToFill->myRotateRandomRotation = data.myRotateRandomRotation;
					settingsToFill->myBurstLength = data.myBurstLength;
					settingsToFill->myBurstSpaceTime = data.myBurstSpaceTime;
					settingsToFill->myBurstMode = (bool)data.myBurstMode;
					settingsToFill->myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
					settingsToFill->myRadius = data.myRadius;
					settingsToFill->myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;

					settingsToFill->myIsBillboard = data.myIsBillboard;
					settingsToFill->myIsMeshParticle = data.myIsMeshParticle;

					settingsToFill->myMeshGUID = data.myMeshGUID;
					settingsToFill->mySubmeshID = data.mySubmeshID;

					settingsToFill->myParticleSpawnMinRotationDirectionXY.x = data.myParticleSpawnMinRotationDirectionXY[0];
					settingsToFill->myParticleSpawnMinRotationDirectionXY.y = data.myParticleSpawnMinRotationDirectionXY[1];
					settingsToFill->myParticleSpawnMaxRotationDirectionXY.x = data.myParticleSpawnMaxRotationDirectionXY[0];
					settingsToFill->myParticleSpawnMaxRotationDirectionXY.y = data.myParticleSpawnMaxRotationDirectionXY[1];
					settingsToFill->mySpawnParticleWithRandomRotationX = data.mySpawnParticleWithRandomRotationX;
					settingsToFill->mySpawnParticleWithRandomRotationY = data.mySpawnParticleWithRandomRotationY;

					settingsToFill->myParticleSpawnSizeXY.x = data.myParticleSpawnSizeXY[0];
					settingsToFill->myParticleSpawnSizeXY.y = data.myParticleSpawnSizeXY[1];
					settingsToFill->myParticleEndSizeXY.x = data.myParticleEndSizeXY[0];
					settingsToFill->myParticleEndSizeXY.y = data.myParticleEndSizeXY[1];
					settingsToFill->myIsUniformScale = data.myIsUniformScale;

					settingsToFill->mySpawnForceMin.x = data.mySpawnForceMin[0];
					settingsToFill->mySpawnForceMin.y = data.mySpawnForceMin[1];
					settingsToFill->mySpawnForceMin.z = data.mySpawnForceMin[2];
					settingsToFill->mySpawnForceMax.x = data.mySpawnForceMax[0];
					settingsToFill->mySpawnForceMax.y = data.mySpawnForceMax[1];
					settingsToFill->mySpawnForceMax.z = data.mySpawnForceMax[2];
					settingsToFill->mySpawnForceRandX = data.mySpawnForceRandX;
					settingsToFill->mySpawnForceRandY = data.mySpawnForceRandY;
					settingsToFill->mySpawnForceRandZ = data.mySpawnForceRandZ;
					settingsToFill->mySpawnForceMirrorX = data.mySpawnForceMirrorX;
					settingsToFill->mySpawnForceMirrorY = data.mySpawnForceMirrorY;
					settingsToFill->mySpawnForceMirrorZ = data.mySpawnForceMirrorZ;

					settingsToFill->myRotateRandomRotationX = data.myRotateRandomRotationX;
					settingsToFill->myRotateRandomRotationY = data.myRotateRandomRotationY;
					settingsToFill->myRotateXAxis = data.myRotateXAxis;
					settingsToFill->myRotateYAxis = data.myRotateYAxis;
					settingsToFill->myRotateZAxis = data.myRotateZAxis;
					settingsToFill->myForwardIsDirection = data.myForwardIsDirection;

					settingsToFill->myUseCurvesEmission = data.myUseCurvesEmission;
					settingsToFill->myAmountOfEmissionPoints = data.myAmountOfEmissionPoints;
					for (unsigned int i = 0; i < data.myAmountOfEmissionPoints; i++)
					{
						settingsToFill->myEmissionOverTimePoints[i].x = data.myEmissionOverTimePoints[i * 2];
						settingsToFill->myEmissionOverTimePoints[i].y = data.myEmissionOverTimePoints[i * 2 + 1];
					}
					settingsToFill->myEmissiveCurveStrength = data.myEmissiveCurveStrength;
					settingsToFill->myEmissiveEndStrength = data.myEmissiveEndStrength;

					settingsToFill->myUseCurvesSize = data.myUseCurvesSize;
					settingsToFill->myAmountOfSizePoints = data.myAmountOfSizePoints;
					settingsToFill->mySizeCurveStrength = data.mySizeCurveStrength;
					for (unsigned int i = 0; i < data.myAmountOfSizePoints; i++)
					{
						settingsToFill->mySizeOverTimePoints[i].x = data.mySizeOverTimePoints[i * 2];
						settingsToFill->mySizeOverTimePoints[i].y = data.mySizeOverTimePoints[i * 2 + 1];
					}
					settingsToFill->myBurstSpawnDelay = data.myBurstSpawnDelay;

					memcpy(aMaterialName, &data.myMaterialName, data.myNumberOfCharactersInMaterialName);
					aMaterialName[data.myNumberOfCharactersInMaterialName] = 0;
					aMatType = data.myMaterialType;
					aGUIDToRead = data.mySystemUUID;
				}
				if (materialVersionIndex == 5)
				{
					struct ParticleImportStruct
					{
						unsigned int myNumberOfColors{};
						float myColorBlendTimers[5]{ 0 };
						float myParticleColors[20]{ 0 };
						float myForce[3]{ 0 };
						float myDrag[3]{ 0 };
						float myBoxSize[3]{ 0 };
						float myOffset[3]{ 0 };
						float mySpawnRate{};
						float mySpawnAngle{};
						float myParticleSpeed{};
						float myParticleSpawnSize{};
						float myParticleEndSize{};
						float myParticleEmissiveStrength{};
						float myParticleMinLifeTime{};
						float myParticleMaxLifeTime{};
						float myParticleSpawnMinRotationDirection{};
						float myParticleSpawnMaxRotationDirection{};
						float myParticleRotationSpeed{};
						float myMinRotationSpeed{};
						float myMaxRotationSpeed{};
						unsigned int mySpawnParticleWithRandomRotation{};
						unsigned int myRotateRandomRotation{};

						float myBurstLength{};
						float myBurstSpaceTime{};
						unsigned int myBurstMode{};
						unsigned int myIsContinouslyBursting{};
						float myRadius{};

						unsigned int myEmitterShape{};
						unsigned int myNumberOfCharactersInName{};
						char mySystemName[128]{ 0 };
						unsigned int myNumberOfCharactersInMaterialName{};
						char myMaterialName[128]{ 0 };
						unsigned int myMaterialType{};

						GUID mySystemUUID = NIL_UUID;

						bool myIsBillboard = true;
						bool myIsMeshParticle = false;
						GUID myMeshGUID = NIL_UUID;
						unsigned short mySubmeshID = 0;
						float myParticleSpawnMinRotationDirectionXY[2];
						float myParticleSpawnMaxRotationDirectionXY[2];
						bool mySpawnParticleWithRandomRotationX = false;
						bool mySpawnParticleWithRandomRotationY = false;
						float myParticleSpawnSizeXY[2] = { 1, 1 };
						float myParticleEndSizeXY[2] = { 1.5f, 1.5f };
						bool myIsUniformScale = false;
						float mySpawnForceMin[3] = { -10, -10, -10 };
						float mySpawnForceMax[3] = { 10, 10, 10 };
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
						float myEmissionOverTimePoints[40];
						float myEmissiveCurveStrength = 1.0;
						float myEmissiveEndStrength = 1.0;
						bool myUseCurvesSize = false;
						int myAmountOfSizePoints = 3;
						float mySizeCurveStrength = 1.0;
						float mySizeOverTimePoints[40];
						float myBurstSpawnDelay = 0.f;
						char modelname[256];
					}data;

					iMD.read((char*)&data, sizeof(ParticleImportStruct));
					iMD.close();
					ParticleEffect::ParticleSettings* settingsToFill = (ParticleEffect::ParticleSettings*)someDataToFill;
					settingsToFill->myNumberOfColors = data.myNumberOfColors;
					for (unsigned int i = 0; i < data.myNumberOfColors; i++)
					{
						settingsToFill->myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
						settingsToFill->myColorBlendTimers[i] = data.myColorBlendTimers[i];
					}
					settingsToFill->myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
					settingsToFill->myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
					settingsToFill->myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
					settingsToFill->myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

					settingsToFill->mySpawnRate = data.mySpawnRate;
					settingsToFill->myParticleSpeed = data.myParticleSpeed;
					settingsToFill->myParticleSpawnSizeZ = data.myParticleSpawnSize;
					settingsToFill->myParticleEndSizeZ = data.myParticleEndSize;
					settingsToFill->myParticleEmissiveStrength = data.myParticleEmissiveStrength;
					settingsToFill->myParticleMinLifeTime = data.myParticleMinLifeTime;
					settingsToFill->myParticleMaxLifeTime = data.myParticleMaxLifeTime;
					settingsToFill->myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
					settingsToFill->myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
					settingsToFill->myMinRotationSpeed = data.myMinRotationSpeed;
					settingsToFill->myMaxRotationSpeed = data.myMaxRotationSpeed;
					settingsToFill->mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
					settingsToFill->myRotateRandomRotation = data.myRotateRandomRotation;
					settingsToFill->myBurstLength = data.myBurstLength;
					settingsToFill->myBurstSpaceTime = data.myBurstSpaceTime;
					settingsToFill->myBurstMode = (bool)data.myBurstMode;
					settingsToFill->myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
					settingsToFill->myRadius = data.myRadius;
					settingsToFill->myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;

					settingsToFill->myIsBillboard = data.myIsBillboard;
					settingsToFill->myIsMeshParticle = data.myIsMeshParticle;

					settingsToFill->myMeshGUID = data.myMeshGUID;
					settingsToFill->mySubmeshID = data.mySubmeshID;

					settingsToFill->myParticleSpawnMinRotationDirectionXY.x = data.myParticleSpawnMinRotationDirectionXY[0];
					settingsToFill->myParticleSpawnMinRotationDirectionXY.y = data.myParticleSpawnMinRotationDirectionXY[1];
					settingsToFill->myParticleSpawnMaxRotationDirectionXY.x = data.myParticleSpawnMaxRotationDirectionXY[0];
					settingsToFill->myParticleSpawnMaxRotationDirectionXY.y = data.myParticleSpawnMaxRotationDirectionXY[1];
					settingsToFill->mySpawnParticleWithRandomRotationX = data.mySpawnParticleWithRandomRotationX;
					settingsToFill->mySpawnParticleWithRandomRotationY = data.mySpawnParticleWithRandomRotationY;

					settingsToFill->myParticleSpawnSizeXY.x = data.myParticleSpawnSizeXY[0];
					settingsToFill->myParticleSpawnSizeXY.y = data.myParticleSpawnSizeXY[1];
					settingsToFill->myParticleEndSizeXY.x = data.myParticleEndSizeXY[0];
					settingsToFill->myParticleEndSizeXY.y = data.myParticleEndSizeXY[1];
					settingsToFill->myIsUniformScale = data.myIsUniformScale;

					settingsToFill->mySpawnForceMin.x = data.mySpawnForceMin[0];
					settingsToFill->mySpawnForceMin.y = data.mySpawnForceMin[1];
					settingsToFill->mySpawnForceMin.z = data.mySpawnForceMin[2];
					settingsToFill->mySpawnForceMax.x = data.mySpawnForceMax[0];
					settingsToFill->mySpawnForceMax.y = data.mySpawnForceMax[1];
					settingsToFill->mySpawnForceMax.z = data.mySpawnForceMax[2];
					settingsToFill->mySpawnForceRandX = data.mySpawnForceRandX;
					settingsToFill->mySpawnForceRandY = data.mySpawnForceRandY;
					settingsToFill->mySpawnForceRandZ = data.mySpawnForceRandZ;
					settingsToFill->mySpawnForceMirrorX = data.mySpawnForceMirrorX;
					settingsToFill->mySpawnForceMirrorY = data.mySpawnForceMirrorY;
					settingsToFill->mySpawnForceMirrorZ = data.mySpawnForceMirrorZ;

					settingsToFill->myRotateRandomRotationX = data.myRotateRandomRotationX;
					settingsToFill->myRotateRandomRotationY = data.myRotateRandomRotationY;
					settingsToFill->myRotateXAxis = data.myRotateXAxis;
					settingsToFill->myRotateYAxis = data.myRotateYAxis;
					settingsToFill->myRotateZAxis = data.myRotateZAxis;
					settingsToFill->myForwardIsDirection = data.myForwardIsDirection;

					settingsToFill->myUseCurvesEmission = data.myUseCurvesEmission;
					settingsToFill->myAmountOfEmissionPoints = data.myAmountOfEmissionPoints;
					for (unsigned int i = 0; i < data.myAmountOfEmissionPoints; i++)
					{
						settingsToFill->myEmissionOverTimePoints[i].x = data.myEmissionOverTimePoints[i * 2];
						settingsToFill->myEmissionOverTimePoints[i].y = data.myEmissionOverTimePoints[i * 2 + 1];
					}
					settingsToFill->myEmissiveCurveStrength = data.myEmissiveCurveStrength;
					settingsToFill->myEmissiveEndStrength = data.myEmissiveEndStrength;

					settingsToFill->myUseCurvesSize = data.myUseCurvesSize;
					settingsToFill->myAmountOfSizePoints = data.myAmountOfSizePoints;
					settingsToFill->mySizeCurveStrength = data.mySizeCurveStrength;
					for (unsigned int i = 0; i < data.myAmountOfSizePoints; i++)
					{
						settingsToFill->mySizeOverTimePoints[i].x = data.mySizeOverTimePoints[i * 2];
						settingsToFill->mySizeOverTimePoints[i].y = data.mySizeOverTimePoints[i * 2 + 1];
					}
					settingsToFill->myBurstSpawnDelay = data.myBurstSpawnDelay;

					memcpy(aMaterialName, &data.myMaterialName, data.myNumberOfCharactersInMaterialName);
					aMaterialName[data.myNumberOfCharactersInMaterialName] = 0;
					settingsToFill->myMeshName = data.modelname;
					aMatType = data.myMaterialType;
					aGUIDToRead = data.mySystemUUID;
				}
				if (materialVersionIndex == 6)
				{
					struct ParticleImportStruct
					{
						unsigned int myNumberOfColors{};
						float myColorBlendTimers[5]{ 0 };
						float myParticleColors[20]{ 0 };
						float myForce[3]{ 0 };
						float myDrag[3]{ 0 };
						float myBoxSize[3]{ 0 };
						float myOffset[3]{ 0 };
						float mySpawnRate{};
						float mySpawnAngle{};
						float myParticleSpeed{};
						float myParticleSpawnSize{};
						float myParticleEndSize{};
						float myParticleEmissiveStrength{};
						float myParticleMinLifeTime{};
						float myParticleMaxLifeTime{};
						float myParticleSpawnMinRotationDirection{};
						float myParticleSpawnMaxRotationDirection{};
						float myParticleRotationSpeed{};
						float myMinRotationSpeed{};
						float myMaxRotationSpeed{};
						unsigned int mySpawnParticleWithRandomRotation{};
						unsigned int myRotateRandomRotation{};

						float myBurstLength{};
						float myBurstSpaceTime{};
						unsigned int myBurstMode{};
						unsigned int myIsContinouslyBursting{};
						float myRadius{};

						unsigned int myEmitterShape{};
						unsigned int myNumberOfCharactersInName{};
						char mySystemName[128]{ 0 };
						unsigned int myNumberOfCharactersInMaterialName{};
						char myMaterialName[128]{ 0 };
						unsigned int myMaterialType{};

						GUID mySystemUUID = NIL_UUID;

						bool myIsBillboard = true;
						bool myIsMeshParticle = false;
						GUID myMeshGUID = NIL_UUID;
						unsigned short mySubmeshID = 0;
						float myParticleSpawnMinRotationDirectionXY[2];
						float myParticleSpawnMaxRotationDirectionXY[2];
						bool mySpawnParticleWithRandomRotationX = false;
						bool mySpawnParticleWithRandomRotationY = false;
						float myParticleSpawnSizeXY[2] = { 1, 1 };
						float myParticleEndSizeXY[2] = { 1.5f, 1.5f };
						bool myIsUniformScale = false;
						float mySpawnForceMin[3] = { -10, -10, -10 };
						float mySpawnForceMax[3] = { 10, 10, 10 };
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
						float myEmissionOverTimePoints[40];
						float myEmissiveCurveStrength = 1.0;
						float myEmissiveEndStrength = 1.0;
						bool myUseCurvesSize = false;
						int myAmountOfSizePoints = 3;
						float mySizeCurveStrength = 1.0;
						float mySizeOverTimePoints[40];
						float myBurstSpawnDelay = 0.f;
						char modelname[256];
						bool mySpawnForcesNormalized = false;
					}data;

					iMD.read((char*)&data, sizeof(ParticleImportStruct));
					iMD.close();
					ParticleEffect::ParticleSettings* settingsToFill = (ParticleEffect::ParticleSettings*)someDataToFill;
					settingsToFill->myNumberOfColors = data.myNumberOfColors;
					for (unsigned int i = 0; i < data.myNumberOfColors; i++)
					{
						settingsToFill->myColorsToBlendBetween[i].myVector.x = data.myParticleColors[0 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.y = data.myParticleColors[1 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.z = data.myParticleColors[2 + i * 4];
						settingsToFill->myColorsToBlendBetween[i].myVector.w = data.myParticleColors[3 + i * 4];
						settingsToFill->myColorBlendTimers[i] = data.myColorBlendTimers[i];
					}
					settingsToFill->myForce = { data.myForce[0], data.myForce[1], data.myForce[2] };
					settingsToFill->myDrag = { data.myDrag[0], data.myDrag[1], data.myDrag[2] };
					settingsToFill->myOffSetAsSubSystem = { data.myOffset[0], data.myOffset[1], data.myOffset[2] };
					settingsToFill->myBoxSize = { data.myBoxSize[0], data.myBoxSize[1], data.myBoxSize[2] };

					settingsToFill->mySpawnRate = data.mySpawnRate;
					settingsToFill->myParticleSpeed = data.myParticleSpeed;
					settingsToFill->myParticleSpawnSizeZ = data.myParticleSpawnSize;
					settingsToFill->myParticleEndSizeZ = data.myParticleEndSize;
					settingsToFill->myParticleEmissiveStrength = data.myParticleEmissiveStrength;
					settingsToFill->myParticleMinLifeTime = data.myParticleMinLifeTime;
					settingsToFill->myParticleMaxLifeTime = data.myParticleMaxLifeTime;
					settingsToFill->myParticleSpawnMinRotationDirectionZ = data.myParticleSpawnMinRotationDirection;
					settingsToFill->myParticleSpawnMaxRotationDirectionZ = data.myParticleSpawnMaxRotationDirection;
					settingsToFill->myMinRotationSpeed = data.myMinRotationSpeed;
					settingsToFill->myMaxRotationSpeed = data.myMaxRotationSpeed;
					settingsToFill->mySpawnParticleWithRandomRotationZ = data.mySpawnParticleWithRandomRotation;
					settingsToFill->myRotateRandomRotation = data.myRotateRandomRotation;
					settingsToFill->myBurstLength = data.myBurstLength;
					settingsToFill->myBurstSpaceTime = data.myBurstSpaceTime;
					settingsToFill->myBurstMode = (bool)data.myBurstMode;
					settingsToFill->myIsContinouslyBursting = (bool)data.myIsContinouslyBursting;
					settingsToFill->myRadius = data.myRadius;
					settingsToFill->myEmitterShape = (ParticleEffect::EmitterShape)data.myEmitterShape;

					settingsToFill->myIsBillboard = data.myIsBillboard;
					settingsToFill->myIsMeshParticle = data.myIsMeshParticle;

					settingsToFill->myMeshGUID = data.myMeshGUID;
					settingsToFill->mySubmeshID = data.mySubmeshID;

					settingsToFill->myParticleSpawnMinRotationDirectionXY.x = data.myParticleSpawnMinRotationDirectionXY[0];
					settingsToFill->myParticleSpawnMinRotationDirectionXY.y = data.myParticleSpawnMinRotationDirectionXY[1];
					settingsToFill->myParticleSpawnMaxRotationDirectionXY.x = data.myParticleSpawnMaxRotationDirectionXY[0];
					settingsToFill->myParticleSpawnMaxRotationDirectionXY.y = data.myParticleSpawnMaxRotationDirectionXY[1];
					settingsToFill->mySpawnParticleWithRandomRotationX = data.mySpawnParticleWithRandomRotationX;
					settingsToFill->mySpawnParticleWithRandomRotationY = data.mySpawnParticleWithRandomRotationY;

					settingsToFill->myParticleSpawnSizeXY.x = data.myParticleSpawnSizeXY[0];
					settingsToFill->myParticleSpawnSizeXY.y = data.myParticleSpawnSizeXY[1];
					settingsToFill->myParticleEndSizeXY.x = data.myParticleEndSizeXY[0];
					settingsToFill->myParticleEndSizeXY.y = data.myParticleEndSizeXY[1];
					settingsToFill->myIsUniformScale = data.myIsUniformScale;

					settingsToFill->mySpawnForceMin.x = data.mySpawnForceMin[0];
					settingsToFill->mySpawnForceMin.y = data.mySpawnForceMin[1];
					settingsToFill->mySpawnForceMin.z = data.mySpawnForceMin[2];
					settingsToFill->mySpawnForceMax.x = data.mySpawnForceMax[0];
					settingsToFill->mySpawnForceMax.y = data.mySpawnForceMax[1];
					settingsToFill->mySpawnForceMax.z = data.mySpawnForceMax[2];
					settingsToFill->mySpawnForceRandX = data.mySpawnForceRandX;
					settingsToFill->mySpawnForceRandY = data.mySpawnForceRandY;
					settingsToFill->mySpawnForceRandZ = data.mySpawnForceRandZ;
					settingsToFill->mySpawnForceMirrorX = data.mySpawnForceMirrorX;
					settingsToFill->mySpawnForceMirrorY = data.mySpawnForceMirrorY;
					settingsToFill->mySpawnForceMirrorZ = data.mySpawnForceMirrorZ;

					settingsToFill->myRotateRandomRotationX = data.myRotateRandomRotationX;
					settingsToFill->myRotateRandomRotationY = data.myRotateRandomRotationY;
					settingsToFill->myRotateXAxis = data.myRotateXAxis;
					settingsToFill->myRotateYAxis = data.myRotateYAxis;
					settingsToFill->myRotateZAxis = data.myRotateZAxis;
					settingsToFill->myForwardIsDirection = data.myForwardIsDirection;

					settingsToFill->myUseCurvesEmission = data.myUseCurvesEmission;
					settingsToFill->myAmountOfEmissionPoints = data.myAmountOfEmissionPoints;
					for (unsigned int i = 0; i < data.myAmountOfEmissionPoints; i++)
					{
						settingsToFill->myEmissionOverTimePoints[i].x = data.myEmissionOverTimePoints[i * 2];
						settingsToFill->myEmissionOverTimePoints[i].y = data.myEmissionOverTimePoints[i * 2 + 1];
					}
					settingsToFill->myEmissiveCurveStrength = data.myEmissiveCurveStrength;
					settingsToFill->myEmissiveEndStrength = data.myEmissiveEndStrength;

					settingsToFill->myUseCurvesSize = data.myUseCurvesSize;
					settingsToFill->myAmountOfSizePoints = data.myAmountOfSizePoints;
					settingsToFill->mySizeCurveStrength = data.mySizeCurveStrength;
					for (unsigned int i = 0; i < data.myAmountOfSizePoints; i++)
					{
						settingsToFill->mySizeOverTimePoints[i].x = data.mySizeOverTimePoints[i * 2];
						settingsToFill->mySizeOverTimePoints[i].y = data.mySizeOverTimePoints[i * 2 + 1];
					}
					settingsToFill->myBurstSpawnDelay = data.myBurstSpawnDelay;
					settingsToFill->mySpawnForcesNormalized = data.mySpawnForcesNormalized;

					memcpy(aMaterialName, &data.myMaterialName, data.myNumberOfCharactersInMaterialName);
					aMaterialName[data.myNumberOfCharactersInMaterialName] = 0;
					settingsToFill->myMeshName = data.modelname;
					aMatType = data.myMaterialType;
					aGUIDToRead = data.mySystemUUID;
				}
				return true;
			}
			return false;
		}
		void BlendSystems()
		{
			SystemData& system = gPaEd.systems[gPaEd.currentSystem];
			ParticleSettings& subSystem = system.particleData;
			if (subSystem.choosingBlendSystem == false)
			{
				ImGui::SetNextWindowBgAlpha(0.5f);
				int spawnPos = (int)EngineInterface::GetWindowResolution().x - (int)EngineInterface::GetWindowResolution().x * 0.5f;
				ImGui::SetNextWindowPos({ (float)spawnPos,  (int)EngineInterface::GetWindowResolution().y * 0.5f });
				ImGui::Begin("Blend Settings", nullptr);
				ImGui::Text("Chose the amount To blend the settings");
				ImGui::SliderFloat("Blend", &gPaEd.blendFactor, 0.0f, 1.0f, "%.3f");
				LerpSettings(gPaEd.blendFactor);
				if (ImGui::Button("Done"))
				{
					gPaEd.blendingSystem = false;
					subSystem.choosingBlendSystem = true;
				}
				ImGui::End();
			}
			if (subSystem.choosingBlendSystem)
			{
				ImGui::OpenPopup("Merge System Settings", ImGuiPopupFlags_MouseButtonDefault_);
				if (ImGui::BeginPopupModal("Merge System Settings"))
				{
					ImGui::Text("Chose the way you want to Merge the Settings");
					ImGui::Text("Use New : ");
					ImGui::SameLine();
					if (ImGui::Button("OverWrite"))
					{
						gPaEd.blendingSystem = false;
						subSystem.choosingBlendSystem = false;
						system.pEffect.GetSettings() = subSystem.blendSettings;
					}
					ImGui::Text("Blend with Existing : ");
					ImGui::SameLine();
					if (ImGui::Button("Blend"))
					{
						subSystem.blendedFromSettings = system.pEffect.GetSettings();
						ParticleEffect::ParticleSettings& emitterSettings = system.pEffect.GetSettings();
						emitterSettings.myNumberOfColors = subSystem.blendSettings.myNumberOfColors;
						subSystem.blendedFromSettings.myNumberOfColors = subSystem.blendSettings.myNumberOfColors;

						emitterSettings.mySpawnParticleWithRandomRotationZ = subSystem.blendSettings.mySpawnParticleWithRandomRotationZ;
						subSystem.blendedFromSettings.mySpawnParticleWithRandomRotationZ = subSystem.blendSettings.mySpawnParticleWithRandomRotationZ;

						emitterSettings.myRotateRandomRotation = subSystem.blendSettings.myRotateRandomRotation;
						subSystem.blendedFromSettings.myRotateRandomRotation = subSystem.blendSettings.myRotateRandomRotation;

						emitterSettings.myBurstMode = subSystem.blendSettings.myBurstMode;
						subSystem.blendedFromSettings.myBurstMode = subSystem.blendSettings.myBurstMode;

						emitterSettings.myIsContinouslyBursting = subSystem.blendSettings.myIsContinouslyBursting;
						subSystem.blendedFromSettings.myIsContinouslyBursting = subSystem.blendSettings.myIsContinouslyBursting;

						emitterSettings.myEmitterShape = subSystem.blendSettings.myEmitterShape;
						subSystem.blendedFromSettings.myEmitterShape = subSystem.blendSettings.myEmitterShape;

						emitterSettings.myIsBillboard = subSystem.blendSettings.myIsBillboard;
						subSystem.blendedFromSettings.myIsBillboard = subSystem.blendSettings.myIsBillboard;
						emitterSettings.myIsMeshParticle = subSystem.blendSettings.myIsMeshParticle;
						subSystem.blendedFromSettings.myIsMeshParticle = subSystem.blendSettings.myIsMeshParticle;
						emitterSettings.myMeshGUID = subSystem.blendSettings.myMeshGUID;
						subSystem.blendedFromSettings.myMeshGUID = subSystem.blendSettings.myMeshGUID;
						emitterSettings.mySubmeshID = subSystem.blendSettings.mySubmeshID;
						subSystem.blendedFromSettings.mySubmeshID = subSystem.blendSettings.mySubmeshID;
						emitterSettings.mySpawnParticleWithRandomRotationX = subSystem.blendSettings.mySpawnParticleWithRandomRotationX;
						subSystem.blendedFromSettings.mySpawnParticleWithRandomRotationX = subSystem.blendSettings.mySpawnParticleWithRandomRotationX;
						emitterSettings.mySpawnParticleWithRandomRotationY = subSystem.blendSettings.mySpawnParticleWithRandomRotationY;
						subSystem.blendedFromSettings.mySpawnParticleWithRandomRotationY = subSystem.blendSettings.mySpawnParticleWithRandomRotationY;
						emitterSettings.myIsUniformScale = subSystem.blendSettings.myIsUniformScale;
						subSystem.blendedFromSettings.myIsUniformScale = subSystem.blendSettings.myIsUniformScale;
						emitterSettings.mySpawnForceRandX = subSystem.blendSettings.mySpawnForceRandX;
						subSystem.blendedFromSettings.mySpawnForceRandX = subSystem.blendSettings.mySpawnForceRandX;
						emitterSettings.mySpawnForceRandY = subSystem.blendSettings.mySpawnForceRandY;
						subSystem.blendedFromSettings.mySpawnForceRandY = subSystem.blendSettings.mySpawnForceRandY;
						emitterSettings.mySpawnForceRandZ = subSystem.blendSettings.mySpawnForceRandZ;
						subSystem.blendedFromSettings.mySpawnForceRandZ = subSystem.blendSettings.mySpawnForceRandZ;
						emitterSettings.mySpawnForceMirrorX = subSystem.blendSettings.mySpawnForceMirrorX;
						subSystem.blendedFromSettings.mySpawnForceMirrorX = subSystem.blendSettings.mySpawnForceMirrorX;
						emitterSettings.mySpawnForceMirrorY = subSystem.blendSettings.mySpawnForceMirrorY;
						subSystem.blendedFromSettings.mySpawnForceMirrorY = subSystem.blendSettings.mySpawnForceMirrorY;
						emitterSettings.mySpawnForceMirrorZ = subSystem.blendSettings.mySpawnForceMirrorZ;
						subSystem.blendedFromSettings.mySpawnForceMirrorZ = subSystem.blendSettings.mySpawnForceMirrorZ;
						emitterSettings.myRotateRandomRotationX = subSystem.blendSettings.myRotateRandomRotationX;
						subSystem.blendedFromSettings.myRotateRandomRotationX = subSystem.blendSettings.myRotateRandomRotationX;
						emitterSettings.myRotateRandomRotationY = subSystem.blendSettings.myRotateRandomRotationY;
						subSystem.blendedFromSettings.myRotateRandomRotationY = subSystem.blendSettings.myRotateRandomRotationY;
						emitterSettings.myRotateXAxis = subSystem.blendSettings.myRotateXAxis;
						subSystem.blendedFromSettings.myRotateXAxis = subSystem.blendSettings.myRotateXAxis;
						emitterSettings.myRotateYAxis = subSystem.blendSettings.myRotateYAxis;
						subSystem.blendedFromSettings.myRotateYAxis = subSystem.blendSettings.myRotateYAxis;
						emitterSettings.myRotateZAxis = subSystem.blendSettings.myRotateZAxis;
						subSystem.blendedFromSettings.myRotateZAxis = subSystem.blendSettings.myRotateZAxis;
						emitterSettings.myForwardIsDirection = subSystem.blendSettings.myForwardIsDirection;
						subSystem.blendedFromSettings.myForwardIsDirection = subSystem.blendSettings.myForwardIsDirection;

						emitterSettings.myUseCurvesEmission = subSystem.blendSettings.myUseCurvesEmission;
						subSystem.blendedFromSettings.myUseCurvesEmission = subSystem.blendSettings.myUseCurvesEmission;
						emitterSettings.myAmountOfEmissionPoints = subSystem.blendSettings.myAmountOfEmissionPoints;
						subSystem.blendedFromSettings.myAmountOfEmissionPoints = subSystem.blendSettings.myAmountOfEmissionPoints;
						for (unsigned int i = 0; i < subSystem.blendSettings.myAmountOfEmissionPoints; i++)
						{
							emitterSettings.myEmissionOverTimePoints[i] = subSystem.blendSettings.myEmissionOverTimePoints[i];
							subSystem.blendedFromSettings.myEmissionOverTimePoints[i] = subSystem.blendSettings.myEmissionOverTimePoints[i];
						}

						emitterSettings.myUseCurvesSize = subSystem.blendSettings.myUseCurvesSize;
						subSystem.blendedFromSettings.myUseCurvesSize = subSystem.blendSettings.myUseCurvesSize;

						emitterSettings.myAmountOfSizePoints = subSystem.blendSettings.myAmountOfSizePoints;
						subSystem.blendedFromSettings.myAmountOfSizePoints = subSystem.blendSettings.myAmountOfSizePoints;
						for (unsigned int i = 0; i < subSystem.blendSettings.myAmountOfSizePoints; i++)
						{
							emitterSettings.mySizeOverTimePoints[i] = subSystem.blendSettings.mySizeOverTimePoints[i];
							subSystem.blendedFromSettings.mySizeOverTimePoints[i] = subSystem.blendSettings.mySizeOverTimePoints[i];
						}
						emitterSettings.mySpawnForcesNormalized = subSystem.blendSettings.mySpawnForcesNormalized;
						subSystem.blendedFromSettings.mySpawnForcesNormalized = subSystem.blendSettings.mySpawnForcesNormalized;
						subSystem.choosingBlendSystem = false;
						//Filebrowser::PushModal("Find System", 7569 << 16 | aSubSystemIndex);
					}				

					ImGui::EndPopup();
				}
			}
		}
		void LerpSettings(float aT)
		{
			ParticleEffect::ParticleSettings& emitterSettings = gPaEd.systems[gPaEd.currentSystem].pEffect.GetSettings();
			SystemData& system = gPaEd.systems[gPaEd.currentSystem];
			ParticleSettings& subSystem = system.particleData;
			float val = 0;
			for (unsigned int i = 0; i < 5; i++)
			{

				val = CU::Lerp(subSystem.blendedFromSettings.myColorBlendTimers[i], subSystem.blendSettings.myColorBlendTimers[i], aT);
				emitterSettings.myColorBlendTimers[i] = val;
				//R
				val = CU::Lerp(subSystem.blendedFromSettings.myColorsToBlendBetween[i][0], subSystem.blendSettings.myColorsToBlendBetween[i][0], aT);
				emitterSettings.myColorsToBlendBetween[i][0] = val;
				//G
				val = CU::Lerp(subSystem.blendedFromSettings.myColorsToBlendBetween[i][1], subSystem.blendSettings.myColorsToBlendBetween[i][1], aT);
				emitterSettings.myColorsToBlendBetween[i][1] = val;
				//B
				val = CU::Lerp(subSystem.blendedFromSettings.myColorsToBlendBetween[i][2], subSystem.blendSettings.myColorsToBlendBetween[i][2], aT);
				emitterSettings.myColorsToBlendBetween[i][2] = val;
				//A
				val = CU::Lerp(subSystem.blendedFromSettings.myColorsToBlendBetween[i][3], subSystem.blendSettings.myColorsToBlendBetween[i][3], aT);
				emitterSettings.myColorsToBlendBetween[i][3] = val;
			}
			//Force
			//X
			val = CU::Lerp(subSystem.blendedFromSettings.myForce.x, subSystem.blendSettings.myForce.x, aT);
			emitterSettings.myForce.x = val;
			//Y
			val = CU::Lerp(subSystem.blendedFromSettings.myForce.y, subSystem.blendSettings.myForce.y, aT);
			emitterSettings.myForce.y = val;
			//Z
			val = CU::Lerp(subSystem.blendedFromSettings.myForce.z, subSystem.blendSettings.myForce.z, aT);
			emitterSettings.myForce.z = val;
			//Drag
				//X
			val = CU::Lerp(subSystem.blendedFromSettings.myDrag.x, subSystem.blendSettings.myDrag.x, aT);
			emitterSettings.myDrag.x = val;
			//Y
			val = CU::Lerp(subSystem.blendedFromSettings.myDrag.y, subSystem.blendSettings.myDrag.y, aT);
			emitterSettings.myDrag.y = val;
			//Z
			val = CU::Lerp(subSystem.blendedFromSettings.myDrag.z, subSystem.blendSettings.myDrag.z, aT);
			emitterSettings.myDrag.z = val;

			val = CU::Lerp(subSystem.blendedFromSettings.mySpawnRate, subSystem.blendSettings.mySpawnRate, aT);
			emitterSettings.mySpawnRate = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpeed, subSystem.blendSettings.myParticleSpeed, aT);
			emitterSettings.myParticleSpeed = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnSizeZ, subSystem.blendSettings.myParticleSpawnSizeZ, aT);
			emitterSettings.myParticleSpawnSizeZ = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleEndSizeZ, subSystem.blendSettings.myParticleEndSizeZ, aT);
			emitterSettings.myParticleEndSizeZ = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleEmissiveStrength, subSystem.blendSettings.myParticleEmissiveStrength, aT);
			emitterSettings.myParticleEmissiveStrength = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleMinLifeTime, subSystem.blendSettings.myParticleMinLifeTime, aT);
			emitterSettings.myParticleMinLifeTime = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleMaxLifeTime, subSystem.blendSettings.myParticleMaxLifeTime, aT);
			emitterSettings.myParticleMaxLifeTime = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnMinRotationDirectionZ, subSystem.blendSettings.myParticleSpawnMinRotationDirectionZ, aT);
			emitterSettings.myParticleSpawnMinRotationDirectionZ = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnMaxRotationDirectionZ, subSystem.blendSettings.myParticleSpawnMaxRotationDirectionZ, aT);
			emitterSettings.myParticleSpawnMaxRotationDirectionZ = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myMinRotationSpeed, subSystem.blendSettings.myMinRotationSpeed, aT);
			emitterSettings.myMinRotationSpeed = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myMaxRotationSpeed, subSystem.blendSettings.myMaxRotationSpeed, aT);
			emitterSettings.myMaxRotationSpeed = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myBurstLength, subSystem.blendSettings.myBurstLength, aT);
			emitterSettings.myBurstLength = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myBurstSpaceTime, subSystem.blendSettings.myBurstSpaceTime, aT);
			emitterSettings.myBurstSpaceTime = val;


			//Shape Settings
			//Box Size
			//X
			val = CU::Lerp(subSystem.blendedFromSettings.myBoxSize.x, subSystem.blendSettings.myBoxSize.x, aT);
			emitterSettings.myBoxSize.x = val;
			//Y
			val = CU::Lerp(subSystem.blendedFromSettings.myBoxSize.y, subSystem.blendSettings.myBoxSize.y, aT);
			emitterSettings.myBoxSize.y = val;
			//Z
			val = CU::Lerp(subSystem.blendedFromSettings.myBoxSize.z, subSystem.blendSettings.myBoxSize.z, aT);
			emitterSettings.myBoxSize.z = val;
			//Radius
			val = CU::Lerp(subSystem.blendedFromSettings.myRadius, subSystem.blendSettings.myRadius, aT);
			emitterSettings.myRadius = val;

			//New Settings
			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnMinRotationDirectionXY.x, subSystem.blendSettings.myParticleSpawnMinRotationDirectionXY.x, aT);
			emitterSettings.myParticleSpawnMinRotationDirectionXY.x = val;
			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnMinRotationDirectionXY.y, subSystem.blendSettings.myParticleSpawnMinRotationDirectionXY.y, aT);
			emitterSettings.myParticleSpawnMinRotationDirectionXY.y = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnMaxRotationDirectionXY.x, subSystem.blendSettings.myParticleSpawnMaxRotationDirectionXY.x, aT);
			emitterSettings.myParticleSpawnMaxRotationDirectionXY.x = val;
			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnMaxRotationDirectionXY.y, subSystem.blendSettings.myParticleSpawnMaxRotationDirectionXY.y, aT);
			emitterSettings.myParticleSpawnMaxRotationDirectionXY.y = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnSizeXY.x, subSystem.blendSettings.myParticleSpawnSizeXY.x, aT);
			emitterSettings.myParticleSpawnSizeXY.x = val;
			val = CU::Lerp(subSystem.blendedFromSettings.myParticleSpawnSizeXY.y, subSystem.blendSettings.myParticleSpawnSizeXY.y, aT);
			emitterSettings.myParticleSpawnSizeXY.y = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myParticleEndSizeXY.x, subSystem.blendSettings.myParticleEndSizeXY.x, aT);
			emitterSettings.myParticleEndSizeXY.x = val;
			val = CU::Lerp(subSystem.blendedFromSettings.myParticleEndSizeXY.y, subSystem.blendSettings.myParticleEndSizeXY.y, aT);
			emitterSettings.myParticleEndSizeXY.y = val;

			val = CU::Lerp(subSystem.blendedFromSettings.mySpawnForceMin.x, subSystem.blendSettings.mySpawnForceMin.x, aT);
			emitterSettings.mySpawnForceMin.x = val;
			val = CU::Lerp(subSystem.blendedFromSettings.mySpawnForceMin.y, subSystem.blendSettings.mySpawnForceMin.y, aT);
			emitterSettings.mySpawnForceMin.y = val;
			val = CU::Lerp(subSystem.blendedFromSettings.mySpawnForceMin.z, subSystem.blendSettings.mySpawnForceMin.z, aT);
			emitterSettings.mySpawnForceMin.z = val;

			val = CU::Lerp(subSystem.blendedFromSettings.mySpawnForceMax.x, subSystem.blendSettings.mySpawnForceMax.x, aT);
			emitterSettings.mySpawnForceMax.x = val;
			val = CU::Lerp(subSystem.blendedFromSettings.mySpawnForceMax.y, subSystem.blendSettings.mySpawnForceMax.y, aT);
			emitterSettings.mySpawnForceMax.y = val;
			val = CU::Lerp(subSystem.blendedFromSettings.mySpawnForceMax.z, subSystem.blendSettings.mySpawnForceMax.z, aT);
			emitterSettings.mySpawnForceMax.z = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myEmissiveCurveStrength, subSystem.blendSettings.myEmissiveCurveStrength, aT);
			emitterSettings.myEmissiveCurveStrength = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myEmissiveEndStrength, subSystem.blendSettings.myEmissiveEndStrength, aT);
			emitterSettings.myEmissiveEndStrength = val;

			val = CU::Lerp(subSystem.blendedFromSettings.mySizeCurveStrength, subSystem.blendSettings.mySizeCurveStrength, aT);
			emitterSettings.mySizeCurveStrength = val;

			val = CU::Lerp(subSystem.blendedFromSettings.myBurstSpawnDelay, subSystem.blendSettings.myBurstSpawnDelay, aT);
			emitterSettings.myBurstSpawnDelay = val;

		}
		void ResetSystemEmission(unsigned short aSystemIndex)
		{
			gPaEd.systems[aSystemIndex].renderCommand.myAmountOfActiveVertices = 0;
			gPaEd.systems[aSystemIndex].emission.lifeTime = 0;
			gPaEd.systems[aSystemIndex].emission.burstTimer = 0;
			gPaEd.systems[aSystemIndex].emission.spawnTimer = 0;
			gPaEd.systems[aSystemIndex].emission.prevBurstTimer = 0;
		}
		void DestroySystem(unsigned short aSystemIndex)
		{
			SystemData& system = gPaEd.systems[aSystemIndex];
			system.isUsed = false;
			system.renderCommand.myMaterial = nullptr;
			system.pEffect.GetData().myMaterial = nullptr;
			SAFE_RELEASE(system.pEffect.GetData().myParticleVertexBuffer);
			if (system.renderCommand.myVertices)
			{
				delete[] system.renderCommand.myVertices;
				system.renderCommand.myVertices = nullptr;
			}
		}
		void ImGuiParticleRotationSettings(const char* aAxis, float& aMinRotationDir, float& aMaxRotationDir,bool& aSpawnRandomRotation, bool& aRotateDir)
		{
			ImGui::Checkbox(FixedString256::Format("Rotate in %s", aAxis), &aRotateDir);
			ImGui::SameLine();
			ImGui::Checkbox(FixedString256::Format("Spawn RandomRotation in %s", aAxis), &aSpawnRandomRotation);
			if (aSpawnRandomRotation)
			{
				ImGui::SetNextItemWidth(75);
				ImGui::SliderFloat(FixedString256::Format("Min Rotation Range in %s", aAxis), &aMinRotationDir, 0, aMaxRotationDir, "%.2f");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(75);
				ImGui::SliderFloat(FixedString256::Format("Max Rotation Range in %s", aAxis), &aMaxRotationDir, aMinRotationDir, 360, "%.2f");
			}
			else
			{
				ImGui::SetNextItemWidth(75);
				ImGui::SliderFloat(FixedString256::Format("Start Rotation in %s", aAxis), &aMaxRotationDir, 0, 360.0f, "%.2f");
			}
		}
		void ImGuiParticleScaleSettings(const char* aAxis, float& aMinScale, float& aMaxScale)
		{
			ImGui::SetNextItemWidth(75);
			ImGui::SliderFloat(FixedString256::Format("Min Scale in %s", aAxis), &aMinScale, 0, aMaxScale, "%.2f");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			ImGui::SliderFloat(FixedString256::Format("Max Scale in %s", aAxis), &aMaxScale, aMinScale, 10.0f, "%.2f");
		}
	}
}