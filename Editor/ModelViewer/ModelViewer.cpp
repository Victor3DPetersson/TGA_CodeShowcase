#include "ModelViewer.h"
#include "Cmn.h"

#include "../Externals/imgui/imgui.h"
#include <Misc\IconsFontAwesome5.h>
#include <FileBrowser\FileBrowser.h>

#include "LevelState\LevelState.h"
#include "..\Engine\Core\Rendering/Resources\FullScreenTexture_Factory.h"
#include "..\Engine\Core\DirectXFramework.h"
#include "..\Engine\Core\Rendering/Renderer.h"
#include "..\Engine\Managers\ModelManager.h"
#include "..\Engine\Managers\MaterialManager.h"
#include "Windows\MaterialEditor\MaterialEditor.h"
#include "..\Engine\Core\ModelExporter.h"
#include "Misc\ModelPorter\ModelPorter.h"
#include "Misc\ViewerCamControls.h"
#include "../Engine/ECS/Systems/Animation/AnimationSystem.h"

namespace f = std::filesystem;
namespace Windows
{
	namespace ModelViewerInternal
	{
		void HandleOpeningOfFile();
		void ReplaceFBXHandling(unsigned short aModelIndex);
		void ModelProperties(unsigned short aModelIndex);
		void AnimatedModelProperties(float aDeltaTime, unsigned short aModelIndex);
		void AnimationProperties(unsigned short aModelIndex);
		void MaterialHandling(unsigned short aModelIndex);
		v3f FindMiddle(unsigned short aModelIndex);
		void UpdateAnimation(float aDeltaTime, unsigned short aModelIndex);
		
	}

	struct AnimationPlayData
	{
		ShortString myAnimationPlaying;
		float myElapsedTime = 0;
		float myDuration = 0;
		float myTicksPerSec = 0;
		int myCurrentSelectedAnimation = 0;
	};

	struct ModelViewerModelData
	{
		bool animated = false;
		unsigned short index = 0;
		bool isUsed = true;
		UUID guid;
		FixedString256 modelName;
		FixedString256 originalModelName;
	};

	struct ModelViewerAnimationModelData
	{
		AnimationPlayData myAnimation;
		AnimationPlayData myAnimation1;
		bool myAnimationPlaying = false;
		bool myBlendAnimations = false;
		bool myLoop = true;
		bool myIsPlaying = false;
		float blendFactor = 0;
		bool showSkeleton = false;
		CU::GrowingArray <ModelPorter::AnimationImportData> importData;
		AnimatedMeshComponent animComp;
		m4f bones[128];
	};
	struct ModelViewerData
	{
		Engine::RenderTarget renderTarget;
		Engine::FullScreenTexture* texture = 0;
		Engine::FullScreenTexture* depth = 0;
		Engine::FullScreenTexture* intermediate = 0;
		Engine::GBuffer* gbuf = 0;

		CU::GrowingArray<ModelViewerModelData> models;

		CU::GrowingArray<std::pair<ModelAnimated, ModelViewerAnimationModelData>> animModels;
		CU::GrowingArray<Model> staticModels;
		unsigned short currentModel = 0;
		unsigned short currentSubModel = 0;
		unsigned short currentAnimation = 0;
		CamControlsSettings cameraSettings;
		bool openingStatic = false;
		bool openingAnimated = false;

		ID3D11Buffer* debugStateBuffer = nullptr;
		int debugState = 1;
		CU::Transform lightRotateDirection;
	} modEdData;

	void ModelViewerInit()
	{
		ID3D11Device* const dev = EngineInterface::GetDXFramework()->GetDevice();
		ID3D11DeviceContext* const cont = EngineInterface::GetDXFramework()->GetDeviceContext();
		EngineInterface::GetRenderer();
		modEdData.texture =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		modEdData.depth =
			Engine::CreateDepthTexture({ 1920U, 1080U }, DXGI_FORMAT_D32_FLOAT, dev, cont, Engine::EDepthStencilSRV::CREATE, Engine::EDepthStencilFlag::BOTH);
		modEdData.intermediate =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		modEdData.gbuf =
			Engine::CreateGBuffer({ 1920U, 1080U }, dev, cont);

		modEdData.renderTarget.texture = &modEdData.texture;
		modEdData.renderTarget.depthTexture = &modEdData.depth;
		modEdData.renderTarget.intermediateTexture = &modEdData.intermediate;
		modEdData.renderTarget.gBufferTexture = &modEdData.gbuf;
		modEdData.renderTarget.camera.RecalculateProjectionMatrix(90, { 1920.0f, 1080.0f }, true, 100.0f, 50000.0f);
		modEdData.renderTarget.renderFlag = RenderFlag::RenderFlag_NoUiOrPost;

		modEdData.models.Init(10);
		modEdData.animModels.Init(10);
		modEdData.staticModels.Init(10);
		modEdData.renderTarget.camera.GetTransform().SetPosition(v3f(0, 150, -1000));
		modEdData.lightRotateDirection.LookAt(v3f(), v3f(150, -350, 250));
	}

	void ModelViewerDeInit()
	{
		for (size_t i = 0; i < modEdData.staticModels.Size(); i++)
		{
			modEdData.staticModels[i].myModelData = nullptr;
		}
		for (unsigned short i = 0; i < modEdData.animModels.Size(); i++)
		{
			for (unsigned short sub = 0; sub < modEdData.animModels[i].first.myModelData.Size(); sub++)
			{
				modEdData.animModels[i].first.myModelData[sub].myIndices = nullptr;
				modEdData.animModels[i].first.myModelData[sub].myVertices = nullptr;
			}
		}
	}

	bool ModelViewer(float aDT, void*)
	{
		bool result = true;

		ImGui::Begin(ICON_FA_STREET_VIEW " Model Viewer###modelviewer", &result, ImGuiWindowFlags_MenuBar);

		ImGui::BeginMenuBar();
		if (ImGui::Button("Open Static Model"))
		{
			Filebrowser::PushModal("Find Material", 8172 << 16 | modEdData.models.Size());
			modEdData.openingStatic = true;
		}
		if (ImGui::Button("Open Animated Model"))
		{
			Filebrowser::PushModal("Find Material", 8169 << 16 | modEdData.models.Size());
			modEdData.openingAnimated = true;
		}
		ModelViewerInternal::HandleOpeningOfFile();

		ImGui::EndMenuBar();

		ImGui::BeginMenuBar();

		if (ImGui::BeginTabBar("ModelTabs"))
		{
			for (unsigned short i = 0; i < modEdData.models.Size(); i++)
			{
				if (modEdData.models[i].animated)
				{
					if (ImGui::BeginTabItem(FixedString256().Format("%s : %i", EngineInterface::GetModelManager()->GetFileNameAnim(modEdData.models[i].guid), (int)i), &modEdData.models[i].isUsed))
					{
						modEdData.currentModel = i;
						ImGui::EndTabItem();
					}
				}
				else
				{
					if (ImGui::BeginTabItem(FixedString256().Format("%s : %i", EngineInterface::GetModelManager()->GetFileName(modEdData.models[i].guid), (int)i), &modEdData.models[i].isUsed))
					{
						modEdData.currentModel = i;
						ImGui::EndTabItem();
					}
				}
			}
			for (unsigned short i = modEdData.models.Size(); i > 0; i--)
			{
				if (modEdData.models[i - 1].isUsed == false)
				{
					unsigned short index = modEdData.models[i - 1].index;
					if (modEdData.models[i - 1].animated)
					{
						modEdData.animModels.RemoveCyclicAtIndex(index);
					}
					else
					{
						modEdData.staticModels.RemoveCyclicAtIndex(index);
					}
					modEdData.models.RemoveCyclicAtIndex(i - 1);
					if (modEdData.currentModel >= modEdData.models.Size())
					{
						modEdData.currentModel = modEdData.models.Size() - 1;
					}
					if (i - 1 != modEdData.models.Size())
					{
						modEdData.models[i - 1].index = index;
					
					}
				}
			}
			ImGui::EndTabBar();
			ImGui::EndMenuBar();

			ImGui::Columns(2, 0, true);

			modEdData.texture->ClearTexture();
			modEdData.depth->ClearDepth();
			if (modEdData.models.Size() > 0)
			{
				CameraControls(aDT, modEdData.renderTarget.camera, modEdData.cameraSettings, ModelViewerInternal::FindMiddle(modEdData.currentModel));
				const unsigned rendererPreviousDebugState = EngineInterface::GetRenderer()->GetDebugState();
				if (modEdData.debugState == 0)
				{
					modEdData.renderTarget.renderFlag = RenderFlag_AllPasses;
				}
				else if (modEdData.debugState == 1)
				{
					modEdData.renderTarget.renderFlag = RenderFlag_NoUiOrPost;
				}
				else
				{
					modEdData.renderTarget.renderFlag = RenderFlag_GbufferDebug;
					EngineInterface::GetRenderer()->SetDebugState(modEdData.debugState - 2);
				}
				if (modEdData.models[modEdData.currentModel].animated)
				{
					std::pair<ModelAnimated, ModelViewerAnimationModelData>& animModel = modEdData.animModels[modEdData.models[modEdData.currentModel].index];
					AnimationSystem::UpdateAnimationViewer(aDT, animModel.second.animComp, &animModel.second.bones[0], &animModel.first);
					ModelAnimated* pointer = &animModel.first;
					EngineInterface::GetRenderer()->RenderModelAnimatedToResource(&pointer, modEdData.renderTarget.texture, modEdData.renderTarget, &animModel.second.bones[0], animModel.second.showSkeleton, 1, modEdData.lightRotateDirection.GetForward());
				}
				else
				{
					EngineInterface::GetRenderer()->RenderModelToResource(&modEdData.staticModels[modEdData.models[modEdData.currentModel].index], modEdData.renderTarget.texture, modEdData.renderTarget, modEdData.lightRotateDirection.GetForward());
				}
				EngineInterface::GetRenderer()->SetDebugState(rendererPreviousDebugState);
			}
			ImGui::Image(ImTextureID(modEdData.texture->GetSRV()), ImGui::GetContentRegionAvail());

			ImGui::NextColumn();
			if (modEdData.models.Size() > 0)
			{
				v3f rot = modEdData.lightRotateDirection.GetRotation();
				ImGui::DragFloat("Light Rotation", &rot.y, 1.f, -180, 180);
				modEdData.lightRotateDirection.SetRotation(rot);
				const char* items[] = { "All Post Processing", "No Post Processing", "Albedo", "Tangent Normal", "Vertex Normal", "Roughness", "Metalness", "AO", "Emissive" };
				ImGui::Combo("combo", &modEdData.debugState, items, IM_ARRAYSIZE(items));

				if (modEdData.models[modEdData.currentModel].animated)
				{
					ModelViewerInternal::AnimatedModelProperties(aDT, modEdData.currentModel);
				}
				else
				{
					ModelViewerInternal::ModelProperties(modEdData.currentModel);
				}
			}
		}
		ImGui::End();
		return result;
	}
	void SetMVModel(UUID id)
	{
		Model* model = EngineInterface::GetModelManager()->GetModel(id);
		if (model)
		{
			for (unsigned short i = 0; i < modEdData.models.Size(); i++)
			{
				if (modEdData.models[i].animated == false && strcmp(modEdData.models[i].originalModelName.Data(), EngineInterface::GetModelManager()->GetFileName(id).Data()) == 0)
				{
					return;
				}
			}
			ModelViewerModelData data;
			data.guid = id;
			data.animated = false;
			data.index = modEdData.staticModels.Size();
			data.modelName = EngineInterface::GetModelManager()->GetFileName(data.guid);
			data.originalModelName = data.modelName;
			modEdData.staticModels.Add(*model);
			modEdData.models.Add(data);

			if(modEdData.models.Size() == 1)
			{
				modEdData.currentModel = 0;
			}
		}
	}
	void SetMVAnimModel(UUID id)
	{
		ModelAnimated* model = EngineInterface::GetModelManager()->GetAnimatedModel(id);
		if (model)
		{
			for (unsigned short i = 0; i < modEdData.models.Size(); i++)
			{
				if(modEdData.models[i].animated && strcmp(modEdData.models[i].originalModelName.Data(), EngineInterface::GetModelManager()->GetFileNameAnim(id).Data()) == 0)
				{
					return;
				}
			}

			std::pair <ModelAnimated, ModelViewerAnimationModelData> createdModel;
			ModelViewerModelData data;
			data.guid = id;
			data.animated = true;
			data.index = modEdData.animModels.Size();

			data.modelName = EngineInterface::GetModelManager()->GetFileNameAnim(data.guid);
			data.originalModelName = data.modelName;
			createdModel.first = *model;
			createdModel.second.importData.Init(model->GetAnimationData().myAnimations.Size() + 1);
			for (unsigned short i = 0; i < model->GetAnimationData().myAnimations.Size(); i++)
			{
				ModelPorter::AnimationImportData dataAnim;
				dataAnim.loadedAnim = true;
				dataAnim.Name = model->GetAnimationData().myAnimations[i].animationName.Data();
				createdModel.second.importData.Add(dataAnim);
			}
			modEdData.animModels.Add(createdModel);
			modEdData.models.Add(data);
			for (unsigned short i = 0; i < createdModel.first.myModelData.Size(); i++)
			{
				createdModel.first.myModelData[i].myVertices = nullptr;
				createdModel.first.myModelData[i].myIndices = nullptr;
			}
			
			if (modEdData.models.Size() == 1)
			{
				modEdData.currentModel = 0;
			}
		}
	}
	namespace ModelViewerInternal
	{
		void HandleOpeningOfFile()
		{
			if (modEdData.openingStatic)
			{
				Filebrowser::FBResult res =
					Filebrowser::UpdateModal("Find Material", 8172 << 16 | modEdData.models.Size(), L"Content/Models/Static", { L".grat" });
				if (res.doubleClicked[0])
				{
					f::path ext = f::path(res.doubleClicked.Data()).extension();
					if (wcscmp(ext.c_str(), L".grat") == 0)
					{
						Windows::SetMVModel(EngineInterface::GetModelManager()->GetModel((const char*)
							FixedString256::Format("%S", f::path(res.doubleClicked.Data()).filename().replace_extension().c_str()), true));
						modEdData.openingStatic = false;
					}
				}
			}
			if (modEdData.openingAnimated)
			{
				Filebrowser::FBResult res =
					Filebrowser::UpdateModal("Find Material", 8169 << 16 | modEdData.models.Size(), L"Content/Models/Animated", { L".gratmotorik" });
				if (res.doubleClicked[0])
				{
					f::path ext = f::path(res.doubleClicked.Data()).extension();
					if (wcscmp(ext.c_str(), L".gratmotorik") == 0)
					{
						Windows::SetMVAnimModel(EngineInterface::GetModelManager()->GetAnimatedModel((const char*)
							FixedString256::Format("%S", f::path(res.doubleClicked.Data()).filename().replace_extension().c_str())));
						modEdData.openingAnimated = false;
					}
				}
			}
		}
		void ReplaceFBXHandling(unsigned short aModelIndex)
		{
			ModelViewerModelData& data = modEdData.models[aModelIndex];
			ModelData* modelData = nullptr;
			AnimatedModelData* animModelData = nullptr;
			Entity entity;
			Material* material = nullptr;
			Model* model = nullptr;
			ModelAnimated* animModel = nullptr;
			unsigned int* selectedMat;
			if (data.animated)
			{
				animModel = &modEdData.animModels[data.index].first;
				if (ImGui::Button("Replace Bind Pose model with new FBX"))
				{
					Filebrowser::PushModal(u8"Import FBX Bind Pose", aModelIndex);
					modEdData.currentModel = aModelIndex;
				}
				Filebrowser::FBResult result =
					Filebrowser::UpdateModal(u8"Import FBX Bind Pose", aModelIndex, L"..\\..", { L".fbx" }, ICON_FA_FILE_IMPORT " Import");
				if (result.doubleClicked[0])
				{
					const wchar_t* denizSerinkaya = wcsrchr(result.doubleClicked, L'.');
					if (denizSerinkaya)
					{
						if (wcscmp(denizSerinkaya, L".fbx") == 0)
						{
							ModelPorter::AnimationModelImportData data;
							data.modelPath = FixedString256::Format("%S", result.doubleClicked);
							ModelAnimated* m = new ModelAnimated();
							auto result = ModelPorter::ImportAnimatedModelFromFile(m, &data);
							if (result)
							{
								// sumthin bad happen wtf log or whatever
							}
							else
							{
								if (m->GetAnimationData().myIndexedSkeleton.Size() == animModel->GetAnimationData().myIndexedSkeleton.Size())
								{
									animModel->GetAnimationData().myIndexedSkeleton = m->GetAnimationData().myIndexedSkeleton;
									animModel->GetAnimationData().myBoneNameToIndex = m->GetAnimationData().myBoneNameToIndex;
									animModel->ReleaseResources();
									for (unsigned short i = 0; i < m->GetAmountOfSubModels(); i++)
									{
										animModel->GetModelData(i).myVertices = nullptr;
										animModel->GetModelData(i).myIndices = nullptr;
										if (animModel->myModelData.Size() == i)
										{
											animModel->AddModelData(m->GetModelData(i));
										}
										else
										{
											AnimatedModelData oldData = animModel->GetModelData(i);
											animModel->GetModelData(i) = m->GetModelData(i);
											if (oldData.mySubModelName == animModel->GetModelData(i).mySubModelName)
											{
												animModel->GetModelData(i).myMaterial = oldData.myMaterial;
											}
										}
										m->GetModelData(i).myVertices = nullptr;
										m->GetModelData(i).myIndices = nullptr;
										D3D11_BUFFER_DESC bufferDescription = { 0 };
										bufferDescription.ByteWidth = sizeof(animModel->GetModelData(i).myVertices[0]) * animModel->GetModelData(i).myNumberOfVertices;
										bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
										bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
										D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
										subresourceData.pSysMem = animModel->GetModelData(i).myVertices;
										HRESULT hRes;
										
										hRes = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&bufferDescription, &subresourceData, &animModel->GetModelData(i).myVertexBuffer);
										if (FAILED(hRes))
										{
										}

										D3D11_BUFFER_DESC indexBufferDescription = { 0 };
										indexBufferDescription.ByteWidth = sizeof(animModel->GetModelData(i).myIndices[0]) * animModel->GetModelData(i).myNumberOfIndices;
										indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
										indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
										D3D11_SUBRESOURCE_DATA indexSubresourceData = { 0 };
										indexSubresourceData.pSysMem = animModel->GetModelData(i).myIndices;

										
										hRes = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&indexBufferDescription, &indexSubresourceData, &animModel->GetModelData(i).myIndexBuffer);
										if (FAILED(hRes))
										{
										}
									}
								}
							}
							delete m;
						}
					}
				}
			}
			else
			{
				model = &modEdData.staticModels[data.index];
				if (ImGui::Button("Replace Model with new FBX"))
				{
					Filebrowser::PushModal(u8"Import FBX", 611);
				}
				Filebrowser::FBResult result =
					Filebrowser::UpdateModal(u8"Import FBX", 611, L"..\\..", { L".fbx" }, ICON_FA_FILE_IMPORT " Import");
				if (result.doubleClicked[0])
				{
					const wchar_t* denizSerinkaya = wcsrchr(result.doubleClicked, L'.');
					if (denizSerinkaya)
					{
						if (wcscmp(denizSerinkaya, L".fbx") == 0)
						{
							ModelPorter::ModelImportData data;
							data.modelPath = FixedString256::Format("%S", result.doubleClicked);
							CU::GrowingArray<ModelData, uint16_t> gurkskit;
							auto result = ModelPorter::ImportModelFromFile(&data, gurkskit);
							if (result)
							{
								// sumthin bad happen wtf log or whatever
							}
							else
							{
								Model* m = new Model();
								m->Init(gurkskit[0]);
								const uint16_t pictorVetesson = gurkskit.Size();
								for (uint16_t i = 0U; i < pictorVetesson; ++i)
								{
									m->AddModelData(gurkskit[i]);
								}
								model->ReleaseResources();
								for (unsigned short i = 0; i < m->GetAmountOfSubModels(); i++)
								{
									if (model->myAmountOfSubModels == i)
									{
										model->AddModelData(m->GetModelData(i));
									}
									else
									{
										ModelData oldData = model->GetModelData(i);
										model->GetModelData(i) = m->GetModelData(i);
										if (oldData.mySubModelName == model->GetModelData(i).mySubModelName)
										{
											model->GetModelData(i).myMaterial = oldData.myMaterial;
										}
									}
									D3D11_BUFFER_DESC bufferDescription = { 0 };
									bufferDescription.ByteWidth = sizeof(model->GetModelData(i).myVertices[0]) * model->GetModelData(i).myNumberOfVertices;
									bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
									bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
									D3D11_SUBRESOURCE_DATA subresourceData = { 0 };
									subresourceData.pSysMem = model->GetModelData(i).myVertices;
									HRESULT hRes;
									
									hRes = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&bufferDescription, &subresourceData, &model->GetModelData(i).myVertexBuffer);
									if (FAILED(result))
									{
									}

									D3D11_BUFFER_DESC indexBufferDescription = { 0 };
									indexBufferDescription.ByteWidth = sizeof(model->GetModelData(i).myIndices[0]) * model->GetModelData(i).myNumberOfIndices;
									indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
									indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
									D3D11_SUBRESOURCE_DATA indexSubresourceData = { 0 };
									indexSubresourceData.pSysMem = model->GetModelData(i).myIndices;

									
									hRes = EngineInterface::GetDXFramework()->GetDevice()->CreateBuffer(&indexBufferDescription, &indexSubresourceData, &model->GetModelData(i).myIndexBuffer);
									if (FAILED(result))
									{
									}
								}
								model->myAABB = m->myAABB;
								model->myIsInited = true;
								delete m;
							}
						}
					}
				}
			}
		}
		void ModelProperties(unsigned short aModelIndex)
		{
			ModelViewerModelData& data = modEdData.models[aModelIndex];
			ImGui::InputTextWithHint("Model Name", "Add the name of Grat Model here", data.modelName, IM_ARRAYSIZE(data.modelName));
			ReplaceFBXHandling(aModelIndex);
			MaterialHandling(aModelIndex);
			
			if (ImGui::Button("Save Changes"))
			{
				std::string modelPath = "Content/Models/Static/";
				for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
				{
					std::string filename = dirEntry.path().filename().replace_extension().string();
					if (dirEntry.is_regular_file() && strcmp(filename.c_str(), data.originalModelName.Data()) == 0)
					{
						modelPath = dirEntry.path().relative_path().string().c_str();
						break;
					}
				}
				modEdData.staticModels[data.index].myModelData[0].myGUID = data.guid;
				Engine::ExportModelToEngineWithPath(&modEdData.staticModels[data.index], modelPath.c_str(), false);
				EngineInterface::GetModelManager()->ReloadGratModel(data.guid);
			}
			ImGui::SameLine();
			if (ImGui::Button("Save As New Model"))
			{
				std::string modelPath = "Content/Models/Static/";
				for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
				{
					std::string filename = dirEntry.path().filename().replace_extension().string();
					if (dirEntry.is_regular_file() && strcmp(filename.c_str(), data.originalModelName.Data()) == 0)
					{
						modelPath = dirEntry.path().relative_path().remove_filename().string().c_str();
						modelPath.append(data.modelName);
						modelPath.append(".grat");
						break;
					}
				}				
				Engine::ExportModelToEngineWithPath(&modEdData.staticModels[data.index], modelPath.c_str(), true);
				EngineInterface::GetModelManager()->LoadGratModel(data.modelName.Data(), true);
			}
		}

		void AnimatedModelProperties(float aDeltaTime, unsigned short aModelIndex)
		{
			ModelViewerModelData& data = modEdData.models[aModelIndex];
			ImGui::InputTextWithHint("Model Name", "Add the name of Grat Model here", data.modelName, IM_ARRAYSIZE(data.modelName));
			ReplaceFBXHandling(aModelIndex);
			MaterialHandling(aModelIndex);
			//ImGui::Separator();
			AnimationProperties(aModelIndex);
			//ImGui::Separator();
			if (ImGui::Button("Save Changes"))
			{
				std::string modelPath = "Content/Models/Animated/";
				for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
				{
					std::string filename = dirEntry.path().filename().replace_extension().string();
					if (dirEntry.is_regular_file() && strcmp(filename.c_str(), data.originalModelName.Data()) == 0)
					{
						modelPath = dirEntry.path().relative_path().string().c_str();
						break;
					}
				}
				modEdData.animModels[data.index].first.myModelData[0].myGUID = data.guid;
				Engine::ExportAnimatedModelToEngineWithPath(&modEdData.animModels[data.index].first, modelPath.c_str(), false);
				EngineInterface::GetModelManager()->ReloadGratMotorikModel(data.guid);
			}
			ImGui::SameLine();
			if (ImGui::Button("Save As New Model"))
			{
				std::string modelPath = "Content/Models/Animated/";
				for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
				{
					std::string filename = dirEntry.path().filename().replace_extension().string();
					if (dirEntry.is_regular_file() && strcmp(filename.c_str(), data.originalModelName.Data()) == 0)
					{
						modelPath = dirEntry.path().relative_path().remove_filename().string().c_str();
						modelPath.append(data.modelName);
						modelPath.append(".gratmotorik");
						break;
					}
				}
				Engine::ExportAnimatedModelToEngineWithPath(&modEdData.animModels[data.index].first, modelPath.c_str(), true);
				EngineInterface::GetModelManager()->LoadGratMotorikModel(data.modelName.Data(), true);
			}
			UpdateAnimation(aDeltaTime, aModelIndex);
		}

		void AnimationProperties(unsigned short aModelIndex)
		{
			ModelViewerModelData& data = modEdData.models[aModelIndex];
			ModelAnimated& model = modEdData.animModels[data.index].first;
			AnimationData& animData = model.GetAnimationData();
			CU::GrowingArray<ModelPorter::AnimationImportData>& animationImportData = modEdData.animModels[data.index].second.importData;

			if (ImGui::CollapsingHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Add Animation : ");
				ImGui::SameLine();
				if (ImGui::Button("+"))
				{
					animationImportData.Add(ModelPorter::AnimationImportData());
				}
				for (unsigned int i = 0; i < animationImportData.Size(); i++)
				{
					std::string animationButtonRemoverTitle = "Remove Animation: ";
					std::string animationTreeName = "Animation : ";
					std::string titleIndex = std::to_string(i + 1);
					animationTreeName.append(titleIndex);
					if (!animationImportData[i].Name.empty())
					{
						animationTreeName.append(" " + animationImportData[i].Name);
					}
					animationButtonRemoverTitle.append(titleIndex);
					if (ImGui::CollapsingHeader(animationTreeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						std::string title = "Animation Name";
						title.append(titleIndex);
						if (!animationImportData[i].loadedAnim)
						{
							ImGui::InputTextWithHint(title.c_str(), "Add the name of the animation here", animationImportData[i].animationNames, IM_ARRAYSIZE(animationImportData[i].animationNames));
							if (ImGui::IsItemDeactivatedAfterEdit())
							{
								animationImportData[i].Name = animationImportData[i].animationNames;
							}
							std::string animationPathTitle = "Animation Path ";
							animationPathTitle.append(titleIndex);

							if (animationImportData[i].Name.size() > 0 && ImGui::Button(animationPathTitle.c_str()))
							{
								modEdData.currentAnimation = i;
								Filebrowser::PushModal("Find Animation", 9889 << 16 | modEdData.currentAnimation);
							}
							if (i == modEdData.currentAnimation)
							{
								Filebrowser::FBResult res =
									Filebrowser::UpdateModal("Find Animation", 9889 << 16 | modEdData.currentAnimation, L"..", { L".fbx" });

								if (res.doubleClicked[0])
								{
									animationImportData[i].Path = f::path(res.doubleClicked.Data()).string();
									Animation anim;
									ModelPorter::ModelResult result = ModelPorter::ImportAnimationFromFile(animationImportData[i].Name, animationImportData[i].Path, anim, &model);
									if (result == ModelPorter::ModelResult_Success)
									{
										animationImportData[i].loadedAnim = true;
									}
									else
									{
										animationImportData[i].loadedAnim = false;
									}
									animData.myAnimationNameToIndex[anim.animationName] = animData.myAnimations.Size();
									animData.myAnimations.Add(anim);
								}
							}
							ImGui::SameLine();
						}

						bool removeThis = false;
						if (ImGui::Button(animationButtonRemoverTitle.c_str()))
						{
							if (animationImportData[i].loadedAnim)
							{
								ShortString animation = animData.myAnimations[i].animationName;
								if (animData.myAnimations.Size() > 1)
								{
									ShortString animNameAtPreviousIndex = animData.myAnimations.GetLast().animationName;
									animData.myAnimationNameToIndex[animNameAtPreviousIndex] = i;
								}
								animData.myAnimations.RemoveCyclicAtIndex(i);
								animData.myAnimationNameToIndex.erase(animation);


								ModelViewerAnimationModelData& mvAnimData = modEdData.animModels[data.index].second;
								mvAnimData.myAnimation.myCurrentSelectedAnimation = 0;
								if (animData.myAnimations.Size() == 0)
								{
									animationImportData[i].loadedAnim = false;
								}
							}
							animationImportData.RemoveAtIndex(i);
							i--;
						}
					}
				}
			}
		}

		void MaterialHandling(unsigned short aModelIndex)
		{
			ModelViewerModelData& data = modEdData.models[aModelIndex];
			ModelData* modelData = nullptr;
			AnimatedModelData* animModelData = nullptr;
			Entity entity;
			Material* material;
			Model* model = nullptr;
			ModelAnimated* animModel = nullptr;
			unsigned int* selectedMat;
			if (data.animated)
			{
				animModel = &modEdData.animModels[data.index].first;
			}
			else
			{
				model = &modEdData.staticModels[data.index];
			}

			if (ImGui::CollapsingHeader("Material Editing", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::BeginTabBar("Materials"))
				{
					unsigned short numberOfSubmeshes = data.animated ? animModel->GetModelData().myNumberOfSubmeshes : model->GetModelData().myNumberOfSubmeshes;
					for (unsigned short i = 0; i < numberOfSubmeshes; i++)
					{
						std::string materialButtonTitle = "Mesh_Mat: ";
						std::string titleIndex = std::to_string(i + 1);
						std::string materialTypeName = "Material Type : ";
						if (data.animated)
						{
							animModelData = &animModel->GetModelData(i);
							materialButtonTitle.append(animModelData->mySubModelName.GetString());
							material = animModelData->myMaterial;
						}
						else
						{
							modelData = &model->GetModelData(i);
							materialButtonTitle.append(modelData->mySubModelName.GetString());
							material = modelData->myMaterial;
						}

						if (ImGui::BeginTabItem(materialButtonTitle.c_str()))
						{
							if (data.animated)
							{
								if (ImGui::Button(animModelData->mySubModelName.Data()))
								{
									modEdData.currentSubModel = i;
									Filebrowser::PushModal("Find Material", 7117 << 16 | modEdData.currentSubModel);
								}
							}
							else
							{
								if (ImGui::Button(modelData->mySubModelName.Data()))
								{
									modEdData.currentSubModel = i;
									Filebrowser::PushModal("Find Material", 7117 << 16 | modEdData.currentSubModel);
								}
							}
							if (i == modEdData.currentSubModel)
							{
								Filebrowser::FBResult res =
									Filebrowser::UpdateModal("Find Material", 7117 << 16 | modEdData.currentSubModel, L"Content/Materials/", { L".gratplat" });

								if (res.doubleClicked[0])
								{
									FixedString256 pathstr =
										FixedString256::Format("%S", f::path(res.doubleClicked.Data()).c_str());
									FixedString256 matName = f::path(res.doubleClicked.Data()).filename().replace_extension().string().c_str();
									unsigned int materialType = 0;
									MaterialEditorInternal::ImportMaterialToEditor(pathstr, 0, &materialType);

									bool loadMat = false;
									if (data.animated)
									{
										switch ((MaterialTypes)materialType)
										{
										case MaterialTypes::EPBR_Anim:
											loadMat = true;
											break;
										case MaterialTypes::EPBRTransparent_Anim:
											loadMat = true;
											break;
										default:
											break;
										}
										if (loadMat)
										{
											animModelData->myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matName.Data(), (MaterialTypes)materialType);
										}
									}
									else
									{
										switch ((MaterialTypes)materialType)
										{
										case MaterialTypes::EPBR:
											loadMat = true;
											break;
										case MaterialTypes::EPBR_Transparent:
											loadMat = true;
											break;
										case MaterialTypes::ERenderTarget:
											loadMat = true;
											break;
										default:
											break;
										}
										if (loadMat)
										{
											modelData->myMaterial = EngineInterface::GetMaterialManager()->GetGratPlat(matName.Data(), (MaterialTypes)materialType);
										}
									}
								}
							}

							switch (material->myMaterialType)
							{
							case MaterialTypes::EPBR:
								materialTypeName += "PBR | Name :";
								break;
							case MaterialTypes::EPBR_Transparent:
								materialTypeName += "PBR_Transparent | Name :";
								break;
							case MaterialTypes::EPBR_Anim:
								materialTypeName += "PBR| Name :";
								break;
							case MaterialTypes::EPBRTransparent_Anim:
								materialTypeName += "PBR_Transparent | Name :";
								break;
							case MaterialTypes::ERenderTarget:
								materialTypeName += "RenderTarget | Name :";
								break;
							default:
								break;
							}

							ImGui::SameLine();
							ImGui::Text(materialTypeName.c_str());
							ImGui::SameLine();
							if (data.animated)
							{
								ImGui::Text(animModelData->myMaterial->myMaterialName.Data());
							}
							else
							{
								ImGui::Text(modelData->myMaterial->myMaterialName.Data());
							}
							ImGui::EndTabItem();
						}
					}
				}
				ImGui::EndTabBar();
			}
		}
		v3f FindMiddle(unsigned short aModelIndex)
		{
			CU::AABB3Df col;
			if (modEdData.models[aModelIndex].animated)
			{
				col = modEdData.animModels[modEdData.models[aModelIndex].index].first.GetCollider();
			}
			else
			{
				col = modEdData.staticModels[modEdData.models[aModelIndex].index].GetCollider();
			}
			return col.GetMiddlePosition();
		}
		void UpdateAnimation(float aDeltaTime, unsigned short aModelIndex)
		{
			ModelViewerAnimationModelData& animData = modEdData.animModels[modEdData.models[aModelIndex].index].second;
			//ImGui::Checkbox("Draw Skeleton", &animData.showSkeleton);
			if (modEdData.animModels[modEdData.models[aModelIndex].index].first.GetAnimationData().myAnimations.Size() != 0)
			{
				AnimationData& animModData = modEdData.animModels[modEdData.models[aModelIndex].index].first.GetAnimationData();
				if (!animData.myBlendAnimations)
				{
					ImGui::Checkbox("Loop", &animData.myLoop);
					ImGui::SameLine();
				}
				//ImGui::Checkbox("Show Skeleton", &animData->drawSkeleton);
				if (animModData.myAnimations.Size() > 1)
				{
					ImGui::SameLine();
					ImGui::Checkbox("Blend Animations", &animData.myBlendAnimations);
					if (animData.myBlendAnimations)
					{
						animData.myLoop = true;
					}
					else
					{
						animData.blendFactor = 0;
					}
				}

				ImGui::Text("Currently Playing Animation");
				if (ImGui::BeginCombo("Main Animation", animData.myAnimation.myAnimationPlaying.GetString()))
				{
					for (int n = 0; n < animModData.myAnimations.Size(); n++)
					{
						const bool is_selected = (animData.myAnimation.myCurrentSelectedAnimation == n);
						if (animData.myBlendAnimations)
						{
							if (ImGui::Selectable(animModData.myAnimations[n].animationName.GetString(), is_selected, (n == animData.myAnimation1.myCurrentSelectedAnimation) ? ImGuiSelectableFlags_Disabled : 0))
							{
								animData.myAnimation.myCurrentSelectedAnimation = n;
							}
						}
						else
						{
							if (ImGui::Selectable(animModData.myAnimations[n].animationName.GetString(), is_selected, (n == animData.myAnimation.myCurrentSelectedAnimation) ? ImGuiSelectableFlags_Disabled : 0))
							{
								animData.myAnimation.myCurrentSelectedAnimation = n;
							}
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				animData.myAnimation.myAnimationPlaying = animModData.myAnimations[animData.myAnimation.myCurrentSelectedAnimation].animationName;
				animData.myAnimation.myDuration = animModData.myAnimations[animData.myAnimation.myCurrentSelectedAnimation].duration;
				animData.myAnimation.myTicksPerSec = animModData.myAnimations[animData.myAnimation.myCurrentSelectedAnimation].ticksPerSec;
				animData.animComp.activeAnimation1 = animData.myAnimation.myAnimationPlaying;
				animData.myAnimation.myElapsedTime = animData.animComp.currentActiveTime1;

				float animTimeInSeconds = animData.myAnimation.myDuration;
				float timeInSeconds = fmodf(animData.myAnimation.myElapsedTime, animTimeInSeconds);
				ImGui::SliderFloat("Scrub", &timeInSeconds, 0.0f, animTimeInSeconds, "%.3f");
				animData.animComp.currentActiveTime1 = timeInSeconds;

				if (animData.myBlendAnimations)
				{
					if (ImGui::BeginCombo("Blend Animation", animData.myAnimation1.myAnimationPlaying.GetString()))
					{
						for (int n = 0; n < animModData.myAnimations.Size(); n++)
						{
							const bool is_selected = (animData.myAnimation1.myCurrentSelectedAnimation == n);
							if (n != animData.myAnimation.myCurrentSelectedAnimation)
							{
								if (ImGui::Selectable(animModData.myAnimations[n].animationName.GetString(), is_selected))
								{
									animData.myAnimation1.myCurrentSelectedAnimation = n;
								}
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					animData.myAnimation1.myAnimationPlaying = animModData.myAnimations[animData.myAnimation1.myCurrentSelectedAnimation].animationName;
					animData.myAnimation1.myDuration = animModData.myAnimations[animData.myAnimation1.myCurrentSelectedAnimation].duration;
					animData.myAnimation1.myTicksPerSec = animModData.myAnimations[animData.myAnimation1.myCurrentSelectedAnimation].ticksPerSec;
					animData.animComp.activeAnimation2 = animData.myAnimation1.myAnimationPlaying;
					animData.myAnimation1.myElapsedTime = animData.animComp.currentActiveTime2;

					float blend_animTimeInSeconds = animData.myAnimation1.myDuration;
					float blend_timeInSeconds = fmodf(animData.myAnimation1.myElapsedTime, blend_animTimeInSeconds);
					ImGui::SliderFloat("Blend Scrub", &blend_timeInSeconds, 0.0f, blend_animTimeInSeconds, "%.3f");
					animData.animComp.currentActiveTime2 = blend_timeInSeconds;
					ImGui::SliderFloat("Blend Factor", &animData.animComp.blendFactor, 0.0f, 1.0f, "%.3f");
				}

				if (animData.myIsPlaying)
				{
					if (!animData.myLoop)
					{
						if (animData.myAnimation.myElapsedTime > animTimeInSeconds)
						{
							animData.animComp.shouldAnimate = false;
							animData.myIsPlaying = false;
						}
					}
					if (ImGui::Button("Pause"))
					{
						animData.animComp.shouldAnimate = false;
						animData.myIsPlaying = false;
					}
				}
				else
				{
					if (animModData.myAnimations.Size() > 1)
					{
						if (animData.myLoop)
						{
							if (ImGui::Button("Resume"))
							{
								animData.myIsPlaying = true;
								animData.animComp.shouldAnimate = true;
							}
						}
						else
						{
							if (ImGui::Button("Play Once"))
							{
								animData.myIsPlaying = true;
								animData.animComp.shouldAnimate = true;
								animData.myAnimation.myElapsedTime = 0;
								animData.animComp.currentActiveTime1 = 0;
							}
						}
					}
				}
			}
		}
	}
}

