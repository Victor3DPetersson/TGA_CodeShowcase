#include "MaterialEditor.h"

#include "../Externals/imgui/imgui.h"
#include <Misc\IconsFontAwesome5.h>
#include <FileBrowser\FileBrowser.h>

#include "LevelState\LevelState.h"
#include "..\Engine\Core\Rendering/Resources\FullScreenTexture_Factory.h"
#include "..\Engine\Core\DirectXFramework.h"
#include "..\Engine\Core\Rendering/Renderer.h"
#include "../Engine/GameObjects/Material.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <filesystem>

#include "../Engine/EngineInterface.h"
#include "../Engine/Managers/MaterialManager.h"
#include "../Engine/Managers/ModelManager.h"
#include "Misc\ViewerCamControls.h"
#include "Cmn.h"

namespace f = std::filesystem;

namespace Windows
{
	namespace MaterialEditorInternal
	{
		struct MaterialData
		{
			unsigned short baseAmountOfTextures = 0;
			unsigned short textureSlotAmount = 0;
			unsigned short primitiveTopology = 0;
			int deferredEffect = 0;
			MaterialTypes materialType;
			ShaderConfiguration shaderConfiguration;
			FixedString256 materialName;
			FixedString256 texturePaths[MAX_TEXTURE_AMOUNT];
			FixedString256 texturePathsNames[MAX_TEXTURE_AMOUNT];
			FixedString256 PSshaderPath;
			FixedString256 VSshaderPath;
			FixedString256 GSshaderPath;
			bool isCutOut = false;
			GUID id = NIL_UUID;
		};
		struct SingleTabData
		{
			bool hasCreatedMaterial = false;
			int chosenMaterialType = 0;
			MaterialEditorInternal::MaterialData data;
		};
	}

	struct MaterialEditorData
	{
		CU::GrowingArray<MaterialEditorInternal::SingleTabData> tabData;
		Model staticSphere;
		ModelAnimated animSphere;
		ParticleRenderCommand particlePlane;
		DecalCommand decal;
		CU::GrowingArray<bool> openTabs;
		Engine::RenderTarget renderTarget;
		Engine::FullScreenTexture* texture = 0;
		Engine::FullScreenTexture* depth = 0;
		Engine::FullScreenTexture* intermediate = 0;
		Engine::GBuffer* gbuf = 0;
		int currentTabBar = 0;
		int currentShader = 0;
		int currentTexture = 0;
		FixedString256 nameToFill;
		MaterialEditorInternal::MaterialData preWrittenMaterials[(int)MaterialTypes::ECount];
		CamControlsSettings cameraSettings;
		CU::Transform lightRotateDirection;
		ID3D11Buffer* debugStateBuffer = nullptr;
		int debugState = 1;
		const char* deferredMaterials[9] = { "Standard Lit", 
			"Multiply Color on Albedo", 
			"Emissive is Albedo", 
			"Transparent Cutout with Albedo blend", 
			"Effect Colors Emissive", 
			"Material.b channel masks color", 
			"UV Panning Vertically",
			"UV Panning Vertically Code Controlled",
			"Emissive No Lighting With panning"
		};

	} matEdData;

	void MaterialEditorInit()
	{
		ID3D11Device* const dev = EngineInterface::GetDXFramework()->GetDevice();
		ID3D11DeviceContext* const cont = EngineInterface::GetDXFramework()->GetDeviceContext();

		matEdData.texture =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		matEdData.depth =
			Engine::CreateDepthTexture({ 1920U, 1080U }, DXGI_FORMAT_D32_FLOAT, dev, cont, Engine::EDepthStencilSRV::CREATE, Engine::EDepthStencilFlag::BOTH);
		matEdData.intermediate =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		matEdData.gbuf =
			Engine::CreateGBuffer({ 1920U, 1080U }, dev, cont);

		matEdData.renderTarget.texture = &matEdData.texture;
		matEdData.renderTarget.depthTexture = &matEdData.depth;
		matEdData.renderTarget.intermediateTexture = &matEdData.intermediate;
		matEdData.renderTarget.gBufferTexture = &matEdData.gbuf;
		matEdData.renderTarget.camera.RecalculateProjectionMatrix(90, { 1920.0f, 1080.0f }, true, 100.0f, 50000.0f);
		matEdData.renderTarget.renderFlag = RenderFlag::RenderFlag_NoUiOrPost;

		matEdData.tabData.Init(10);
		matEdData.openTabs.Init(10);
		MaterialEditorInternal::CreatePreWrittenMaterials();
		MaterialEditorInternal::InitSpheres();
		matEdData.renderTarget.camera.GetTransform().SetPosition(v3f(0, 0, -500));
		matEdData.lightRotateDirection.LookAt(v3f(), v3f(150, -450, 250));
	}
	void MaterialEditorDeInit()
	{
		matEdData.staticSphere.myModelData = nullptr;
	}
	bool MaterialEditor(float aDT, void*)
	{
		bool result = true;
		unsigned short amountOfTabs = matEdData.tabData.Size();
		ImGui::Begin(ICON_FA_PALETTE " Material Editor###materialeditorwalla", &result, ImGuiWindowFlags_MenuBar);

		ImGui::BeginMenuBar();
		if (ImGui::Button(" Create New Material "))
		{
			matEdData.tabData.Add(MaterialEditorInternal::SingleTabData());
			matEdData.openTabs.Add(true);
			//Model* sphere = EngineInterface::GetModelManager()->LoadPrimitive(PrimitiveType::PrimitiveType_Sphere);
		}
		if (ImGui::Button("Import Material"))
		{
			Filebrowser::PushModal("Find Material", 8181 << 16 | matEdData.tabData.Size());
		}
		MaterialEditorInternal::HandleImport();

		ImGui::EndMenuBar();

		ImGui::Columns(2, 0, true);
	
		if (amountOfTabs > 0)
		{
			matEdData.texture->ClearTexture();
			matEdData.depth->ClearDepth();

			if (matEdData.tabData.Size() > 0)
			{
				CameraControls(aDT, matEdData.renderTarget.camera, matEdData.cameraSettings, v3f());
				const unsigned rendererPreviousDebugState = EngineInterface::GetRenderer()->GetDebugState();
				if (matEdData.debugState == 0)
				{
					matEdData.renderTarget.renderFlag = RenderFlag_AllPasses;
				}
				else if (matEdData.debugState == 1)
				{
					matEdData.renderTarget.renderFlag = RenderFlag_NoUiOrPost;
				}
				else
				{
					matEdData.renderTarget.renderFlag = RenderFlag_GbufferDebug;
					EngineInterface::GetRenderer()->SetDebugState(matEdData.debugState - 2);
				}
				//matEdData.spheres[matEdData.currentTabBar].GetModelData(0).myMaterial = ;
				switch ((MaterialTypes)matEdData.tabData[matEdData.currentTabBar].chosenMaterialType)
				{
				case MaterialTypes::EPBR:
					matEdData.staticSphere.myModelData[0].myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matEdData.tabData[matEdData.currentTabBar].data.materialName.Data(), MaterialTypes::EPBR);
					EngineInterface::GetRenderer()->RenderModelToResource(&matEdData.staticSphere, matEdData.renderTarget.texture, matEdData.renderTarget, matEdData.lightRotateDirection.GetForward());
					break;
				case MaterialTypes::EPBR_Transparent:
					matEdData.staticSphere.myModelData[0].myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matEdData.tabData[matEdData.currentTabBar].data.materialName.Data(), MaterialTypes::EPBR_Transparent);
					EngineInterface::GetRenderer()->RenderModelToResource(&matEdData.staticSphere, matEdData.renderTarget.texture, matEdData.renderTarget, matEdData.lightRotateDirection.GetForward());
					break;
				case MaterialTypes::EPBR_Anim:
				{
					m4f bone = m4f(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
					
					matEdData.animSphere.myModelData[0].myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matEdData.tabData[matEdData.currentTabBar].data.materialName.Data(), MaterialTypes::EPBR_Anim);
					ModelAnimated* pointer = &matEdData.animSphere;
					EngineInterface::GetRenderer()->RenderModelAnimatedToResource(&pointer, matEdData.renderTarget.texture, matEdData.renderTarget, &bone, false, 1, matEdData.lightRotateDirection.GetForward());
				}
					break;
				case MaterialTypes::EPBRTransparent_Anim:
				{
					m4f bone;
					matEdData.animSphere.myModelData[0].myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matEdData.tabData[matEdData.currentTabBar].data.materialName.Data(), MaterialTypes::EPBRTransparent_Anim);
					ModelAnimated* pointer = &matEdData.animSphere;
					EngineInterface::GetRenderer()->RenderModelAnimatedToResource(&pointer, matEdData.renderTarget.texture, matEdData.renderTarget, &bone, false, 1, matEdData.lightRotateDirection.GetForward());
				}
					break;
				case MaterialTypes::EParticle_Default:
					matEdData.particlePlane.myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matEdData.tabData[matEdData.currentTabBar].data.materialName.Data(), MaterialTypes::EParticle_Default);
					EngineInterface::GetRenderer()->RenderModelParticleToResource(&matEdData.particlePlane, 1, nullptr, 0, matEdData.renderTarget.texture, matEdData.renderTarget, matEdData.lightRotateDirection.GetForward(), true);
					break;
				case MaterialTypes::EParticle_Glow:
					matEdData.particlePlane.myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matEdData.tabData[matEdData.currentTabBar].data.materialName.Data(), MaterialTypes::EParticle_Glow);
					EngineInterface::GetRenderer()->RenderModelParticleToResource(&matEdData.particlePlane, 1, nullptr, 0, matEdData.renderTarget.texture, matEdData.renderTarget, matEdData.lightRotateDirection.GetForward(), true);
					break;
				case MaterialTypes::EDecal:
					break;
				case MaterialTypes::ERenderTarget:
					matEdData.staticSphere.myModelData[0].myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matEdData.tabData[matEdData.currentTabBar].data.materialName.Data(), MaterialTypes::ERenderTarget);
					EngineInterface::GetRenderer()->RenderModelToResource(&matEdData.staticSphere, matEdData.renderTarget.texture, matEdData.renderTarget, matEdData.lightRotateDirection.GetForward());
					break;
				default:
					break;
				}
				EngineInterface::GetRenderer()->SetDebugState(rendererPreviousDebugState);
			}

			ImGui::Image(ImTextureID(matEdData.texture->GetSRV()), ImGui::GetContentRegionAvail());
		}
		ImGui::NextColumn();
		v3f rot = matEdData.lightRotateDirection.GetRotation();
		ImGui::DragFloat("Light Rotation", &rot.y, 1.f, -180, 180);
		matEdData.lightRotateDirection.SetRotation(rot);
		const char* items[] = { "All Post Processing", "No Post Processing", "Albedo", "Tangent Normal", "Vertex Normal", "Roughness", "Metalness", "AO", "Emissive" };
		ImGui::Combo("combo", &matEdData.debugState, items, IM_ARRAYSIZE(items));
		if (ImGui::BeginTabBar("MaterialTabs"))
		{
			for (unsigned short i = 0; i < amountOfTabs; i++)
			{
				if (ImGui::BeginTabItem(FixedString256().Format("%s : %i", matEdData.tabData[i].data.materialName, (int)i), &matEdData.openTabs[i]))
				{
					matEdData.currentTabBar = i;
					MaterialEditorInternal::HandleEditing(i);
					ImGui::EndTabItem();
				}			
			}
			for (unsigned short i = amountOfTabs; i > 0; i--)
			{
				if (matEdData.openTabs[i - 1] == false)
				{
					matEdData.tabData.RemoveCyclicAtIndex(i - 1);
					matEdData.openTabs.RemoveCyclicAtIndex(i - 1);
					matEdData.currentTabBar = matEdData.tabData.Size() - 1;
					amountOfTabs--;
				}
			}
		}
		MaterialEditorInternal::HandleCreation();
		if (amountOfTabs > 0)
		{
			MaterialEditorInternal::HandleExport();
		}
		ImGui::EndTabBar();

		ImGui::End();

		return result;
	}


	namespace MaterialEditorInternal
	{
		void HandleCreation()
		{			
			if (matEdData.tabData.Size() > 0 && matEdData.tabData[matEdData.currentTabBar].hasCreatedMaterial == false)
			{
				ImGui::Separator();
				MaterialEditorInternal::SingleTabData& data = matEdData.tabData[matEdData.currentTabBar];
				const char* materials[] = { "Default", "Default_Transparent", "Default_Animated", "Default_Transparent_Animated", "Particle_Default", "Particle_Glow", "Decal", "RenderTarget"};
				ImGui::ListBox("Materials", &data.chosenMaterialType, materials, IM_ARRAYSIZE(materials), 8);
				ImGui::Text("Material Type To Create : ");
				ImGui::SameLine();
				ImGui::Text(materials[data.chosenMaterialType]);
				if (ImGui::Button(" Create Material "))
				{
					data.hasCreatedMaterial = true;
					data.data = matEdData.preWrittenMaterials[data.chosenMaterialType];
				}			
			}
		}

		void HandleEditing(unsigned short& aIndex)
		{
			SingleTabData& tabData = matEdData.tabData[aIndex];
			MaterialData& matData = tabData.data;
			if (tabData.hasCreatedMaterial)
			{
				if (ImGui::InputTextWithHint("Material Name", "Add the name of the material here", &matEdData.nameToFill[0], 256, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					tabData.data.materialName = matEdData.nameToFill.Data();
					matEdData.nameToFill = FixedString256();
				}
				if (matData.materialType == MaterialTypes::EPBR || matData.materialType == MaterialTypes::EPBR_Anim)
				{
					ImGui::Combo("Deferred Materials", &matData.deferredEffect, matEdData.deferredMaterials, IM_ARRAYSIZE(matEdData.deferredMaterials), 7);

				}
				if (ImGui::BeginTabBar("Texture Tabs"))
				{
					if (ImGui::BeginTabItem("Textures"))
					{
						switch (matData.materialType)
						{
						case MaterialTypes::EPBR:
							ImGui::Text("Material Type: PBR");
							break;
						case MaterialTypes::EPBR_Transparent:
							ImGui::Text("Material Type: PBR_Transparent");
							break;
						case MaterialTypes::EPBR_Anim:
							ImGui::Text("Material Type: PBR_Anim");
							break;
						case MaterialTypes::EPBRTransparent_Anim:
							ImGui::Text("Material Type: PBR_Transparent_Anim");
							break;
						case MaterialTypes::EParticle_Default:
							ImGui::Text("Material Type: Particle_Default");
							break;
						case MaterialTypes::EParticle_Glow:
							ImGui::Text("Material Type: Particle_Glow");
							break;
						case MaterialTypes::ERenderTarget:
							ImGui::Text("Material Type: Render Target");
								break;
						case MaterialTypes::EDecal:
							ImGui::Text("Material Type: Decal");
							break;
						default:
							break;
						}
						ImGui::SameLine();
						if (matData.textureSlotAmount < MAX_TEXTURE_AMOUNT && ImGui::Button("Add Texture"))
						{
							matData.textureSlotAmount++;
							std::string TextureName = "";
							TextureName.append("Custom Texture : \0");
							TextureName.append(std::to_string(matData.textureSlotAmount - matData.baseAmountOfTextures));
							matData.texturePathsNames[matData.textureSlotAmount - 1] = TextureName;
							matData.texturePaths[matData.textureSlotAmount - 1] = ("\0");
						}
						if (matData.textureSlotAmount > matData.baseAmountOfTextures)
						{
							if (matData.textureSlotAmount < MAX_TEXTURE_AMOUNT)
							{
								ImGui::SameLine();
							}
							if (ImGui::Button("Remove Texture"))
							{
								matData.textureSlotAmount--;
								matData.texturePathsNames[matData.textureSlotAmount] = "\0";
							}
						}
						for (unsigned short i = 0; i < matData.textureSlotAmount; i++)
						{
							if (ImGui::Button(matData.texturePathsNames[i].Data()))
							{
								matEdData.currentTexture = i;
								Filebrowser::PushModal("Open Texture", 6969 << 16 | matEdData.currentTexture);
							}
							if (i == matEdData.currentTexture)
							{
								Filebrowser::FBResult res =
									Filebrowser::UpdateModal("Open Texture", 6969 << 16 | matEdData.currentTexture, L"Content/Textures/", { L".dds" });

								if (res.doubleClicked[0])
								{
									FixedString256 pathstr =
										FixedString256::Format("%S", f::path(res.doubleClicked.Data()).c_str());
									matData.texturePaths[i] = pathstr.Data();
								}
							}

							ImGui::SameLine();
							FixedString256 slotName = FixedString256::Format(" Texture Path : %s", matData.texturePathsNames[i]);
							ImGui::Text(slotName.Data());
							ImGui::SameLine();
							ImGui::Text(matData.texturePaths[i].Data());
						}
						if (matData.materialType == MaterialTypes::EPBR_Transparent || matData.materialType == MaterialTypes::EPBRTransparent_Anim)
						{
							ImGui::Checkbox("Render Opague", &matData.isCutOut);
						}
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Shaders"))
					{
						for (unsigned int i = 0; i < 3; i++)
						{
							FixedString256 shaderName;
							switch (i)
							{
							case 0:
								shaderName = "Search for Vertex Shader";
								break;
							case 1:
								shaderName = "Search for Geometry Shader";
								break;
							case 2:
								shaderName = "Search for Pixel Shader";
								break;
							default:
								break;
							}
							ShowShaderElements(shaderName, i, (unsigned short)matData.shaderConfiguration, aIndex);
						}
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
			}
		}

		void HandleImport()
		{		
			Filebrowser::FBResult res =
				Filebrowser::UpdateModal("Find Material", 8181 << 16 | matEdData.tabData.Size(), L"Content/Materials/", { L".gratplat" });

			if (res.doubleClicked[0])
			{
				FixedString256 pathstr =
					FixedString256::Format("%S", f::path(res.doubleClicked.Data()).c_str());
				matEdData.tabData.Add(SingleTabData());
				matEdData.openTabs.Add(true);
				if (ImportMaterialToEditor(pathstr.Data(), matEdData.tabData.Size() - 1))
				{
					matEdData.tabData.GetLast().hasCreatedMaterial = true;
					matEdData.tabData.GetLast().chosenMaterialType = (int)matEdData.tabData.GetLast().data.materialType;
				}
				else
				{
					matEdData.tabData.RemoveAtIndex(matEdData.tabData.Size());
					matEdData.openTabs.RemoveAtIndex(matEdData.openTabs.Size());
				}
			}
		}
		void HandleExport()
		{
			if (strlen(matEdData.tabData[matEdData.currentTabBar].data.materialName) > 0)
			{
				ImGui::Separator();
				if (ImGui::Button(" Export Material "))
				{
					ExportMaterialToEngine(matEdData.currentTabBar);
				}
			}
		}

		void ShowShaderElements(const char* aButtonName, unsigned int aShaderIndex, unsigned short aShaderConfig, unsigned short aMaterialIndex)
		{
			bool showButton = false;
			switch ((ShaderConfiguration)aShaderConfig)
			{
			case ShaderConfiguration::VS:
				if (aShaderIndex == 0) { showButton = true; }
				break;
			case ShaderConfiguration::GS:
				if (aShaderIndex == 1) { showButton = true; }
				break;
			case ShaderConfiguration::PS:
				if (aShaderIndex == 2) { showButton = true; }
				break;
			case ShaderConfiguration::VS_PS:
				if (aShaderIndex == 0 || aShaderIndex == 2) { showButton = true; }
				break;
			case ShaderConfiguration::VS_GS:
				if (aShaderIndex == 0 || aShaderIndex == 1) { showButton = true; }
				break;
			case ShaderConfiguration::GS_PS:
				if (aShaderIndex == 1 || aShaderIndex == 2) { showButton = true; }
				break;
			case ShaderConfiguration::VS_GS_PS:
				showButton = true;
				break;
			default:
				break;
			}

			if (showButton)
			{

				if (ImGui::Button(aButtonName))
				{
					matEdData.currentShader = aShaderIndex;
					Filebrowser::PushModal("Chose Shader", 9669 << 16 | matEdData.currentShader);
				}
				if (aShaderIndex == matEdData.currentShader)
				{
					Filebrowser::FBResult res =
						Filebrowser::UpdateModal("Chose Shader", 9669 << 16 | matEdData.currentShader, L"Content/Shaders/", { L".cso" });

					if (res.doubleClicked[0])
					{
						FixedString256 pathstr =
							FixedString256::Format("%S", f::path(res.doubleClicked.Data()).relative_path().c_str());

						switch (matEdData.currentShader)
						{
						case 0:
							matEdData.tabData[aMaterialIndex].data.VSshaderPath = pathstr.Data();
							break;
						case 1:
							matEdData.tabData[aMaterialIndex].data.GSshaderPath = pathstr.Data();
							break;
						case 2:
							matEdData.tabData[aMaterialIndex].data.PSshaderPath = pathstr.Data();
							break;
						default:
							break;
						}
					}
				}
				switch (aShaderIndex)
				{
				case 0:
					ImGui::Text(matEdData.tabData[aMaterialIndex].data.VSshaderPath.Data());
					break;
				case 1:
					ImGui::Text(matEdData.tabData[aMaterialIndex].data.GSshaderPath.Data());
					break;
				case 2:
					ImGui::Text(matEdData.tabData[aMaterialIndex].data.PSshaderPath.Data());
					break;
				default:
					break;
				}
			}
		}

		void CreatePreWrittenMaterials()
		{
			for (unsigned int i = 0; i < (int)MaterialTypes::ECount; i++)
			{
				MaterialTypes currentType = (MaterialTypes)i;
				switch (currentType)
				{
				case MaterialTypes::EPBR:
					matEdData.preWrittenMaterials[i].materialType = currentType;

					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Albedo : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[1] = "Normal : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[2] = "Material : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[3] = "Emissive : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 4;

					matEdData.preWrittenMaterials[i].VSshaderPath = "Content/Shaders/Deferred_GBuffer_VS.cso\0";
					matEdData.preWrittenMaterials[i].GSshaderPath = "";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/Deferred_GBuffer_PS.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::VS_PS;
					matEdData.preWrittenMaterials[i].primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					break;
				case MaterialTypes::EPBR_Transparent:
					matEdData.preWrittenMaterials[i].materialType = currentType;

					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Albedo : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[1] = "Normal : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[2] = "Material : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[3] = "Emissive : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 4;
					matEdData.preWrittenMaterials[i].VSshaderPath = "Content/Shaders/VertexShader_Forward.cso\0";
					matEdData.preWrittenMaterials[i].GSshaderPath = "\0";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/PBR_Forward.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::VS_PS;
					matEdData.preWrittenMaterials[i].primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					break;

				case MaterialTypes::EPBR_Anim:
					matEdData.preWrittenMaterials[i].materialType = currentType;

					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Albedo : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[1] = "Normal : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[2] = "Material : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[3] = "Emissive : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 4;

					matEdData.preWrittenMaterials[i].VSshaderPath = "Content/Shaders/GBuffer_VS_Animated.cso\0";
					matEdData.preWrittenMaterials[i].GSshaderPath = "\0";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/Deferred_GBuffer_PS.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::VS_PS;
					matEdData.preWrittenMaterials[i].primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					break;

				case MaterialTypes::EPBRTransparent_Anim:
					matEdData.preWrittenMaterials[i].materialType = currentType;

					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Albedo : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[1] = "Normal : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[2] = "Material : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[3] = "Emissive : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 4;

					matEdData.preWrittenMaterials[i].VSshaderPath = "Content/Shaders/Forward_Anim_VS.cso\0";
					matEdData.preWrittenMaterials[i].GSshaderPath = "\0";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/PBR_Forward.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::VS_PS;
					matEdData.preWrittenMaterials[i].primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					break;

				case MaterialTypes::EParticle_Glow:
					matEdData.preWrittenMaterials[i].materialType = currentType;
					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Alpha Texture : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 1;

					matEdData.preWrittenMaterials[i].VSshaderPath = "Content/Shaders/Particle_VS.cso\0";
					matEdData.preWrittenMaterials[i].GSshaderPath = "Content/Shaders/Particle_GS.cso\0";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/Particle_PS.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::VS_GS_PS;
					matEdData.preWrittenMaterials[i].primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
					break;

				case MaterialTypes::EParticle_Default:
					matEdData.preWrittenMaterials[i].materialType = currentType;
					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Alpha Texture : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 1;

					matEdData.preWrittenMaterials[i].VSshaderPath = "Content/Shaders/Particle_VS.cso\0";
					matEdData.preWrittenMaterials[i].GSshaderPath = "Content/Shaders/Particle_GS.cso\0";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/Particle_PS.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::VS_GS_PS;
					matEdData.preWrittenMaterials[i].primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
					break;


				case MaterialTypes::EDecal:
					matEdData.preWrittenMaterials[i].materialType = currentType;

					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Albedo : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[1] = "Normal : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[2] = "Material : \0";
					matEdData.preWrittenMaterials[i].texturePathsNames[3] = "Emissive : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 4;
					matEdData.preWrittenMaterials[i].VSshaderPath = "\0";
					matEdData.preWrittenMaterials[i].GSshaderPath = "\0";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/Decal_DefaultPBR.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::PS;
					break;
				case MaterialTypes::ERenderTarget:
					matEdData.preWrittenMaterials[i].materialType = currentType;

					matEdData.preWrittenMaterials[i].texturePathsNames[0] = "Render Target Texture : \0";
					matEdData.preWrittenMaterials[i].baseAmountOfTextures = 1;
					matEdData.preWrittenMaterials[i].VSshaderPath = "Content/Shaders/Deferred_GBuffer_VS.cso";
					matEdData.preWrittenMaterials[i].GSshaderPath = "\0";
					matEdData.preWrittenMaterials[i].PSshaderPath = "Content/Shaders/PS_RenderTarget_Basic.cso\0";
					matEdData.preWrittenMaterials[i].shaderConfiguration = ShaderConfiguration::VS_PS;
					matEdData.preWrittenMaterials[i].primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					break;
				default:
					break;
				}
				matEdData.preWrittenMaterials[i].textureSlotAmount = matEdData.preWrittenMaterials[i].baseAmountOfTextures;
			}
		}

		static unsigned int versionNumb = 7;
		constexpr unsigned int NUMB_TEXTURES = 12;
		bool ExportMaterialToEngine(unsigned short aMaterialIndex)
		{
			struct MaterialExportStruct
			{
				unsigned int materialVersionIndex;
				GUID id;
				unsigned int shaderConfiguration;
				unsigned int materialType;
				unsigned int primitiveTopology;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInName;
				unsigned int renderCutout;
				unsigned int deferredEffect;
				char myMaterialName[128];
				char myTextures[MAX_TEXTURE_AMOUNT][128];
				char myShaders[3][128];
			};

			SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
			MaterialData& matData = tabData.data;

			MaterialExportStruct exportStruct;
			exportStruct.materialVersionIndex = versionNumb;
			exportStruct.shaderConfiguration = (unsigned int)matData.shaderConfiguration;
			exportStruct.materialType = (unsigned int)matData.materialType;
			exportStruct.primitiveTopology = (unsigned int)matData.primitiveTopology;
			exportStruct.numberOfTextures = (unsigned int)matData.textureSlotAmount;
			exportStruct.numberOfCharactersInName = (unsigned int)0;
			exportStruct.deferredEffect = matData.deferredEffect;
			size_t nameLength = strlen(matData.materialName);
			for (unsigned short c = 0; c < 128; c++)
			{
				if (c < nameLength)
				{
					exportStruct.numberOfCharactersInName++;
					exportStruct.myMaterialName[c] = matData.materialName[c];
				}
				else
				{
					exportStruct.myMaterialName[c] = 0;
				}
			}
			for (unsigned int i = 0; i < MAX_TEXTURE_AMOUNT; i++)
			{
				size_t numbCharsInTexture = strlen(matData.texturePaths[i]);
				for (unsigned short c = 0; c < 128; c++)
				{
					if (i < exportStruct.numberOfTextures)
					{
						if (c < numbCharsInTexture)
						{
							exportStruct.myTextures[i][c] = matData.texturePaths[i][c];
						}
						else
						{
							exportStruct.myTextures[i][c] = 0;
						}
					}
					else
					{
						exportStruct.myTextures[i][c] = 0;
					}
				}
			}
			if (matData.isCutOut)
			{
				exportStruct.renderCutout = 1;
			}
			else
			{
				exportStruct.renderCutout = 0;
			}
			/// Koden här inne är as äcklig, men man behöver aldrig peta på den igen, Den går över mina strings baserat på vilken Shader config man kör 
			/// och sparar rätt shader paths bsaerat på vilken config det är
			std::string shaderType;
			switch (matData.materialType)
			{
			case MaterialTypes::EPBR:
				shaderType = "PBR";
				break;
			case MaterialTypes::EPBR_Transparent:
				shaderType = "PBR_Transparent";
				break;
			case MaterialTypes::EPBR_Anim:
				shaderType = "PBR_Anim";
				break;
			case MaterialTypes::EPBRTransparent_Anim:
				shaderType = "PBR_Transparent_Anim";
				break;
			case MaterialTypes::EParticle_Default:
				shaderType = "Particle_Default";
				break;
			case MaterialTypes::EParticle_Glow:
				shaderType = "Particle_Glow";
				break;
			case MaterialTypes::EDecal:
				shaderType = "Decal";
				break;
			case MaterialTypes::ERenderTarget:
				shaderType = "RenderTarget";
				break;
			default:
				break;
			}
			for (unsigned short c = 0; c < 128; c++)
			{
				size_t VSSize = strlen(matData.VSshaderPath);
				if (c < VSSize)
				{
					exportStruct.myShaders[0][c] = matData.VSshaderPath[c];
				}
				else
				{
					exportStruct.myShaders[0][c] = 0;
				}
				size_t GSSize = strlen(matData.GSshaderPath);
				if (c < GSSize)
				{
					exportStruct.myShaders[1][c] = matData.GSshaderPath[c];
				}
				else
				{
					exportStruct.myShaders[1][c] = 0;
				}
				size_t PSSize = strlen(matData.PSshaderPath);
				if (c < PSSize)
				{
					exportStruct.myShaders[2][c] = matData.PSshaderPath[c];
				}
				else
				{
					exportStruct.myShaders[2][c] = 0;
				}
			}

			//std::string modelPath = "Content/Materials/" + shaderType;// + "/" + std::string(matData.materialName.Data()) + ".gratplat";
			std::string modelPath = "Content/Materials/" + shaderType + "/";

			std::filesystem::path currentPath = std::filesystem::current_path();
			std::string outpathAbs = currentPath.u8string();
			outpathAbs.append('/' + modelPath);

			bool foundMat = false;
			for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
			{
				std::string filename = dirEntry.path().filename().replace_extension().string();
				if (dirEntry.is_regular_file() && strcmp(filename.c_str(), exportStruct.myMaterialName) == 0)
				{
					outpathAbs = dirEntry.path().relative_path().string().c_str();
					foundMat = true;
					break;
				}
			}
			if (foundMat)
			{
				if (std::filesystem::exists(outpathAbs.c_str()))
				{
					std::filesystem::permissions(outpathAbs.c_str(), std::filesystem::perms::all);
					std::filesystem::remove(outpathAbs.c_str());
				}
			}
			else
			{
				UuidCreate(&matData.id);
				outpathAbs = modelPath + matData.materialName.Data() + ".gratplat";
			}
			if (matData.id == NIL_UUID)
			{
				UuidCreate(&matData.id);
			}
			exportStruct.id = matData.id;

			std::ofstream outMaterial;
			outMaterial.open(outpathAbs, std::ios::out | std::ios::binary);
			outMaterial.write((char*)&exportStruct, sizeof(MaterialExportStruct));
			outMaterial.close();
			//Reload Material in Engine

			EngineInterface::GetMaterialManager()->ReloadMaterial(matData.materialName.Data(), matData.materialType);
			return true;
		}

		bool ImportMaterialToEditor(const char* aPath, unsigned short aMaterialIndex, unsigned int* aMatType)
		{
			std::ifstream iMD;
			iMD.open(aPath, std::ios::in | std::ios::binary);
			if (iMD)
			{
				unsigned int materialVersionIndex;
				iMD.read((char*)&materialVersionIndex, sizeof(materialVersionIndex));
				if (materialVersionIndex == 1)
				{
					struct ints
					{
						unsigned int shaderConfiguration;
						unsigned int materialType;
						unsigned int PrimitiveTopology;
						unsigned int numberOfShaders;
						unsigned int numberOfTextures;
						unsigned int numberOfCharactersInTexture[4];
						unsigned int numberOfCharactersInShader[3];
						unsigned int numberOfCharactersInName;
					} intsToRead;

					struct strings
					{
						char myMaterialName[128];
						char myTextures[4][128];
						char myShaders[3][128];
					} stringsToRead;

					iMD.read((char*)&intsToRead, sizeof(ints));
					iMD.read((char*)&stringsToRead, sizeof(strings));
					iMD.close();
					if (aMatType)
					{
						*aMatType = intsToRead.materialType;
						return true;
					}
					SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
					MaterialData& matData = tabData.data;
					matData = matEdData.preWrittenMaterials[intsToRead.materialType];


					matData.materialType = (MaterialTypes)intsToRead.materialType;
					matData.textureSlotAmount = intsToRead.numberOfTextures;
					matData.shaderConfiguration = (ShaderConfiguration)intsToRead.shaderConfiguration;
					matData.materialName = stringsToRead.myMaterialName;
					for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
					{
						matData.texturePaths[i] = stringsToRead.myTextures[i];
					}
					switch (matData.shaderConfiguration)
					{
					case ShaderConfiguration::VS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						break;
					case ShaderConfiguration::GS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::PS:
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::GS_PS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					default:
						break;
					}
					return true;
				}
				if (materialVersionIndex == 2)
				{
					struct ints
					{
						unsigned int shaderConfiguration;
						unsigned int materialType;
						unsigned int PrimitiveTopology;
						unsigned int numberOfShaders;
						unsigned int numberOfTextures;
						unsigned int numberOfCharactersInTexture[8];
						unsigned int numberOfCharactersInShader[3];
						unsigned int numberOfCharactersInName;
					} intsToRead;

					struct strings
					{
						char myMaterialName[128];
						char myTextures[8][128];
						char myShaders[3][128];
					} stringsToRead;

					iMD.read((char*)&intsToRead, sizeof(ints));
					iMD.read((char*)&stringsToRead, sizeof(strings));
					iMD.close();
					if (aMatType)
					{
						*aMatType = intsToRead.materialType;
						return true;
					}
					SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
					MaterialData& matData = tabData.data;
					matData = matEdData.preWrittenMaterials[intsToRead.materialType];


					matData.materialType = (MaterialTypes)intsToRead.materialType;
					matData.textureSlotAmount = intsToRead.numberOfTextures;
					matData.shaderConfiguration = (ShaderConfiguration)intsToRead.shaderConfiguration;
					matData.materialName = stringsToRead.myMaterialName;
					for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
					{
						if (i >= matData.baseAmountOfTextures)
						{
							std::string TextureName = "";
							TextureName.append("Custom Texture : ");
							TextureName.append(std::to_string(matData.textureSlotAmount - matData.baseAmountOfTextures));
							matData.texturePathsNames[i] = (TextureName);
						}
						matData.texturePaths[i] = stringsToRead.myTextures[i];
					}
					switch (matData.shaderConfiguration)
					{
					case ShaderConfiguration::VS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						break;
					case ShaderConfiguration::GS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::PS:
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::GS_PS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					default:
						break;
					}
					return true;
				}
				if (materialVersionIndex == 3)
				{
					struct ints
					{
						unsigned int shaderConfiguration;
						unsigned int materialType;
						unsigned int PrimitiveTopology;
						unsigned int numberOfShaders;
						unsigned int numberOfTextures;
						unsigned int numberOfCharactersInTexture[NUMB_TEXTURES];
						unsigned int numberOfCharactersInShader[3];
						unsigned int numberOfCharactersInName;
					} intsToRead;

					struct strings
					{
						char myMaterialName[128];
						char myTextures[NUMB_TEXTURES][128];
						char myShaders[3][128];
					} stringsToRead;

					iMD.read((char*)&intsToRead, sizeof(ints));
					iMD.read((char*)&stringsToRead, sizeof(strings));
					iMD.close();
					if (aMatType)
					{
						*aMatType = intsToRead.materialType;
						return true;
					}
					SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
					MaterialData& matData = tabData.data;
					matData = matEdData.preWrittenMaterials[intsToRead.materialType];
					matData.materialType = (MaterialTypes)intsToRead.materialType;
					matData.textureSlotAmount = intsToRead.numberOfTextures;
					matData.shaderConfiguration = (ShaderConfiguration)intsToRead.shaderConfiguration;
					matData.materialName = stringsToRead.myMaterialName;
					for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
					{
						if (i >= matData.baseAmountOfTextures)
						{
							std::string TextureName = "";
							TextureName.append("Custom Texture : ");
							TextureName.append(std::to_string(i - matData.baseAmountOfTextures + 1));
							matData.texturePathsNames[i] = (TextureName);
						}
						matData.texturePaths[i] = stringsToRead.myTextures[i];
					}
					switch (matData.shaderConfiguration)
					{
					case ShaderConfiguration::VS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						break;
					case ShaderConfiguration::GS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::PS:
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::GS_PS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					default:
						break;
					}
					return true;
				}
				if (materialVersionIndex == 4)
				{
					struct ints
					{
						unsigned int shaderConfiguration;
						unsigned int materialType;
						unsigned int PrimitiveTopology;
						unsigned int numberOfShaders;
						unsigned int numberOfTextures;
						unsigned int numberOfCharactersInTexture[NUMB_TEXTURES];
						unsigned int numberOfCharactersInShader[3];
						unsigned int numberOfCharactersInName;
						unsigned int myRenderOpague;
					} intsToRead;

					struct strings
					{
						char myMaterialName[128];
						char myTextures[NUMB_TEXTURES][128];
						char myShaders[3][128];
					} stringsToRead;

					iMD.read((char*)&intsToRead, sizeof(ints));
					iMD.read((char*)&stringsToRead, sizeof(strings));
					iMD.close();
					if (aMatType)
					{
						*aMatType = intsToRead.materialType;
						return true;
					}
					SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
					MaterialData& matData = tabData.data;
					matData = matEdData.preWrittenMaterials[intsToRead.materialType];

					if (intsToRead.myRenderOpague == 1)
					{
						matData.isCutOut = true;
					}
					else
					{
						matData.isCutOut = false;
					}
					matData.materialType = (MaterialTypes)intsToRead.materialType;
					matData.textureSlotAmount = intsToRead.numberOfTextures;
					matData.shaderConfiguration = (ShaderConfiguration)intsToRead.shaderConfiguration;
					matData.materialName = stringsToRead.myMaterialName;
					for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
					{
						if (i >= matData.baseAmountOfTextures)
						{
							std::string TextureName = "";
							TextureName.append("Custom Texture : ");
							TextureName.append(std::to_string(i - matData.baseAmountOfTextures + 1));
							matData.texturePathsNames[i] = (TextureName);
						}
						matData.texturePaths[i] = stringsToRead.myTextures[i];
					}
					switch (matData.shaderConfiguration)
					{
					case ShaderConfiguration::VS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						break;
					case ShaderConfiguration::GS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::PS:
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::GS_PS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					default:
						break;
					}
					return true;
				}
				if (materialVersionIndex == 5)
				{
					struct ints
					{
						unsigned int shaderConfiguration;
						unsigned int materialType;
						unsigned int primitiveTopology;
						unsigned int numberOfTextures;
						unsigned int numberOfCharactersInName;
						unsigned int renderCutout;
					} intsToRead;

					struct strings
					{
						char myMaterialName[128];
						char myTextures[MAX_TEXTURE_AMOUNT][128];
						char myShaders[3][128];
					} stringsToRead;

					iMD.read((char*)&intsToRead, sizeof(ints));
					iMD.read((char*)&stringsToRead, sizeof(strings));
					iMD.close();
					if (aMatType)
					{
						*aMatType = intsToRead.materialType;
						return true;
					}
					SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
					MaterialData& matData = tabData.data;
					matData = matEdData.preWrittenMaterials[intsToRead.materialType];

					if (intsToRead.renderCutout == 1)
					{
						matData.isCutOut = true;
					}
					else
					{
						matData.isCutOut = false;
					}
					matData.materialType = (MaterialTypes)intsToRead.materialType;
					matData.textureSlotAmount = intsToRead.numberOfTextures;
					matData.shaderConfiguration = (ShaderConfiguration)intsToRead.shaderConfiguration;
					matData.materialName = stringsToRead.myMaterialName;
					for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
					{
						if (i >= matData.baseAmountOfTextures)
						{
							std::string TextureName = "";
							TextureName.append("Custom Texture : ");
							TextureName.append(std::to_string(i - matData.baseAmountOfTextures + 1));
							matData.texturePathsNames[i] = (TextureName);
						}
						matData.texturePaths[i] = stringsToRead.myTextures[i];
					}
					switch (matData.shaderConfiguration)
					{
					case ShaderConfiguration::VS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						break;
					case ShaderConfiguration::GS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::PS:
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::GS_PS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					default:
						break;
					}
					return true;
				}
				if (materialVersionIndex == 6)
				{
					struct ints
					{
						unsigned int shaderConfiguration;
						unsigned int materialType;
						unsigned int primitiveTopology;
						unsigned int numberOfTextures;
						unsigned int numberOfCharactersInName;
						unsigned int renderCutout;
					} intsToRead;

					struct strings
					{
						char myMaterialName[128];
						char myTextures[MAX_TEXTURE_AMOUNT][128];
						char myShaders[3][128];
					} stringsToRead;
					GUID id;
					iMD.read((char*)&id, sizeof(GUID));
					iMD.read((char*)&intsToRead, sizeof(ints));
					iMD.read((char*)&stringsToRead, sizeof(strings));
					iMD.close();
					if (aMatType)
					{
						*aMatType = intsToRead.materialType;
						return true;
					}
					SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
					MaterialData& matData = tabData.data;
					matData = matEdData.preWrittenMaterials[intsToRead.materialType];
					matData.id = id;
					if (intsToRead.renderCutout == 1)
					{
						matData.isCutOut = true;
					}
					else
					{
						matData.isCutOut = false;
					}
					matData.materialType = (MaterialTypes)intsToRead.materialType;
					matData.textureSlotAmount = intsToRead.numberOfTextures;
					matData.shaderConfiguration = (ShaderConfiguration)intsToRead.shaderConfiguration;
					matData.materialName = stringsToRead.myMaterialName;
					for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
					{
						if (i >= matData.baseAmountOfTextures)
						{
							std::string TextureName = "";
							TextureName.append("Custom Texture : ");
							TextureName.append(std::to_string(i - matData.baseAmountOfTextures + 1));
							matData.texturePathsNames[i] = (TextureName);
						}
						matData.texturePaths[i] = stringsToRead.myTextures[i];
					}
					switch (matData.shaderConfiguration)
					{
					case ShaderConfiguration::VS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						break;
					case ShaderConfiguration::GS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::PS:
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::GS_PS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					default:
						break;
					}
					return true;
				}
				if (materialVersionIndex == 7)
				{
					struct ints
					{
						unsigned int shaderConfiguration;
						unsigned int materialType;
						unsigned int primitiveTopology;
						unsigned int numberOfTextures;
						unsigned int numberOfCharactersInName;
						unsigned int renderCutout;
						unsigned int deferredEffect;
					} intsToRead;

					struct strings
					{
						char myMaterialName[128];
						char myTextures[MAX_TEXTURE_AMOUNT][128];
						char myShaders[3][128];
					} stringsToRead;
					GUID id;
					iMD.read((char*)&id, sizeof(GUID));
					iMD.read((char*)&intsToRead, sizeof(ints));
					iMD.read((char*)&stringsToRead, sizeof(strings));
					iMD.close();
					if (aMatType)
					{
						*aMatType = intsToRead.materialType;
						return true;
					}
					SingleTabData& tabData = matEdData.tabData[aMaterialIndex];
					MaterialData& matData = tabData.data;
					matData = matEdData.preWrittenMaterials[intsToRead.materialType];
					matData.id = id;
					if (intsToRead.renderCutout == 1)
					{
						matData.isCutOut = true;
					}
					else
					{
						matData.isCutOut = false;
					}
					matData.materialType = (MaterialTypes)intsToRead.materialType;
					matData.textureSlotAmount = intsToRead.numberOfTextures;
					matData.shaderConfiguration = (ShaderConfiguration)intsToRead.shaderConfiguration;
					matData.materialName = stringsToRead.myMaterialName;
					matData.deferredEffect = intsToRead.deferredEffect;
					for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
					{
						if (i >= matData.baseAmountOfTextures)
						{
							std::string TextureName = "";
							TextureName.append("Custom Texture : ");
							TextureName.append(std::to_string(i - matData.baseAmountOfTextures + 1));
							matData.texturePathsNames[i] = (TextureName);
						}
						matData.texturePaths[i] = stringsToRead.myTextures[i];
					}
					switch (matData.shaderConfiguration)
					{
					case ShaderConfiguration::VS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						break;
					case ShaderConfiguration::GS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::PS:
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						break;
					case ShaderConfiguration::GS_PS:
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					case ShaderConfiguration::VS_GS_PS:
						strcpy_s(&matData.VSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[0]);
						strcpy_s(&matData.GSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[1]);
						strcpy_s(&matData.PSshaderPath[0], sizeof(FixedString256), stringsToRead.myShaders[2]);
						break;
					default:
						break;
					}
					return true;
				}
			}
			return false;
		}

		void InitSpheres()
		{
			Model* sphere = EngineInterface::GetModelManager()->GetModel(EngineInterface::GetModelManager()->LoadPrimitive(PrimitiveType::PrimitiveType_Sphere));

			for (unsigned short i = 0; i < (unsigned short)MaterialTypes::ECount; i++)
			{
				switch ((MaterialTypes)i)
				{
				case MaterialTypes::EPBR:
					matEdData.staticSphere = *sphere;
					matEdData.staticSphere.myName = "MaterialSphere";
					break;
				case MaterialTypes::EPBR_Anim:
				{
					matEdData.animSphere.myModelData.Add(AnimatedModelData());
					AnimatedModelData& model = matEdData.animSphere.myModelData[0];
					model.mySubModelName = "MaterialSphereAnimated";
					model = AnimatedModelData(sphere->GetModelData().myNumberOfVertices, sphere->GetModelData().myNumberOfIndices, 1);
					model.myBoundingVolume = sphere->GetCollider();
					matEdData.animSphere.SetCollider(model.myBoundingVolume);
					for (unsigned int i = 0; i < sphere->GetModelData().myNumberOfVertices; i++)
					{
						model.myVertices[i].myPosition = sphere->GetModelData().myVertices[i].myPosition;
						model.myVertices[i].myNormal = sphere->GetModelData().myVertices[i].myNormal;
						model.myVertices[i].myBinormal = sphere->GetModelData().myVertices[i].myBinormal;
						model.myVertices[i].myTangent = sphere->GetModelData().myVertices[i].myTangent;
						model.myVertices[i].myColor = sphere->GetModelData().myVertices[i].myColor;
						model.myVertices[i].myUVCoords = sphere->GetModelData().myVertices[i].myUVCoords;
						model.myVertices[i].Weights[0] = 1.0f;
						model.myVertices[i].IDs[0] = 0;
					}
					memcpy(&model.myIndices[0], &sphere->GetModelData().myIndices[0], sizeof(unsigned int) * sphere->GetModelData().myNumberOfIndices);
					D3D11_BUFFER_DESC bufferDescription = { 0 };
					bufferDescription.ByteWidth = sizeof(model.myVertices[0]) * model.myNumberOfVertices;
					bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
					bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
					D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
					subresourceData.pSysMem = model.myVertices;

					ID3D11Buffer* vertexBuffer;
					HRESULT result;
					result = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&bufferDescription, &subresourceData, &vertexBuffer);
					if (FAILED(result))
					{
						assert(false && "Failed to create Vertex Buffer");
					}

					D3D11_BUFFER_DESC indexBufferDescription = { 0 };
					indexBufferDescription.ByteWidth = sizeof(model.myIndices[0]) * model.myNumberOfIndices;
					indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
					indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
					D3D11_SUBRESOURCE_DATA indexSubresourceData = { 0 };
					indexSubresourceData.pSysMem = model.myIndices;

					ID3D11Buffer* indexBuffer;
					result = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&indexBufferDescription, &indexSubresourceData, &indexBuffer);
					if (FAILED(result))
					{
						assert(false && "Failed to create Index Buffer");
					}
					model.myIndexBuffer = indexBuffer;
					model.myVertexBuffer = vertexBuffer;


					matEdData.animSphere.myAnimationData.myAnimations.Init(1);
					matEdData.animSphere.myAnimationData.myIndexedSkeleton.Init(1);
					Bone bone;
					bone = m4f(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
					matEdData.animSphere.myAnimationData.myIndexedSkeleton.Add(bone);
				}
				break;
				case MaterialTypes::EParticle_Default:
				{
					ParticleRenderCommand& model = matEdData.particlePlane;
					model.myAmountOfActiveVertices = 1;
					model.myNumberOfParticles = 1;
					model.myVertices = new Vertex_Particle[1];

					HRESULT result;
					D3D11_BUFFER_DESC particleVertexBufferDesc = { 0 };
					particleVertexBufferDesc.ByteWidth = sizeof(Vertex_Particle);
					particleVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
					particleVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
					particleVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
					subresourceData.pSysMem = model.myVertices;
					ID3D11Buffer* vertexBuffer = nullptr;
					result = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&particleVertexBufferDesc, &subresourceData, &vertexBuffer);
					if (FAILED(result))
					{
						assert(false && "Failed to create Vertex Buffer");
					}
					model.myStride = sizeof(Vertex_Particle);
					model.myParticleVertexBuffer = vertexBuffer;

					model.myVertices[0].mySize = v2f(1, 1);
					model.myVertices[0].myPosition = v3f(1, 1, 1);
					model.myVertices[0].myColor = CU::Color(255, 255, 255, 255);
					model.myVertices[0].myCurrentColor = 0;
					model.myVertices[0].myEmissiveStrength = 1;
					model.myVertices[0].mySize = v2f(1, 1);
				}
				break;
				default:
					break;
				}


			}

		}
	}
}


