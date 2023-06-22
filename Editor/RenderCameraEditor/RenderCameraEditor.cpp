#include "RenderCameraEditor.h"
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

namespace f = std::filesystem;

namespace Windows
{
	struct CameraEditorData
	{
		Engine::RenderTarget renderTarget;
		Engine::FullScreenTexture* texture = 0;
		Engine::FullScreenTexture* depth = 0;
		Engine::FullScreenTexture* intermediate = 0;
		Engine::GBuffer* gbuf = 0;
		EditorRenderCamera cameras[NUMBOF_RENDERTARGETS];
		bool cameraActive = false;
		bool renderGizmos = true;
		size_t activeIndex = 0;
		int currentRenderMode = 0;
	} gCamEdData;

	void RenderCameraEditorInit()
	{
		ID3D11Device* const dev = EngineInterface::GetDXFramework()->GetDevice();
		ID3D11DeviceContext* const cont = EngineInterface::GetDXFramework()->GetDeviceContext();

		gCamEdData.texture =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		gCamEdData.depth =
			Engine::CreateDepthTexture({ 1920U, 1080U }, DXGI_FORMAT_D32_FLOAT, dev, cont, Engine::EDepthStencilSRV::CREATE, Engine::EDepthStencilFlag::BOTH);
		gCamEdData.intermediate =
			Engine::CreateFullScreenTexture({ 1920U, 1080U }, DXGI_FORMAT_R16G16B16A16_FLOAT, dev, cont);
		gCamEdData.gbuf =
			Engine::CreateGBuffer({ 1920U, 1080U }, dev, cont);

		gCamEdData.renderTarget.texture = &gCamEdData.texture;
		gCamEdData.renderTarget.depthTexture = &gCamEdData.depth;
		gCamEdData.renderTarget.intermediateTexture = &gCamEdData.intermediate;
		gCamEdData.renderTarget.gBufferTexture = &gCamEdData.gbuf;
		gCamEdData.renderTarget.renderFlag = RenderFlag::RenderFlag_NoUiOrPost;

		gCamEdData.renderTarget.camera.RecalculateProjectionMatrix(90, { 1920.0f, 1080.0f }, true, 20.f, 2500.f);

		for (int i = 0; i < NUMBOF_RENDERTARGETS; ++i) {
			gCamEdData.cameras[i].renderResolution = { 1920, 1080 };
		}
	}

	bool RenderCameraEditor(float aDT, void*)
	{
		bool result = true;

		ImGui::Begin(ICON_FA_FILE_VIDEO " Render Camera Editor###RenderCameras", &result, ImGuiWindowFlags_MenuBar);

		ImGui::Columns(2, 0, true);

		gCamEdData.depth->ClearDepth();
		if (gCamEdData.cameraActive)
		{
			gCamEdData.texture->ClearTexture({ 0, 0, 0, 255 });
			gCamEdData.renderTarget.camera.GetTransform().SetScale(v3f(1, 1, 1));
			EngineInterface::GetRenderer()->RenderSpecifiedTarget(gCamEdData.renderTarget);
		}
		else
		{
			gCamEdData.texture->ClearTexture({ 240, 92, 169, 255 });
		}
		ImGui::Image(ImTextureID(gCamEdData.texture->GetSRV()), ImGui::GetContentRegionAvail());

		ImGui::NextColumn();
		ImGui::Checkbox("Camera Gizmos", &gCamEdData.renderGizmos);
		if (ImGui::BeginTabBar("Camera"))
		{
			for (size_t i = 0; i < NUMBOF_RENDERTARGETS; i++)
			{
				if (gCamEdData.cameras[i].activated)
				{
					ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4( 0.45f, 0.60f, 0.1f, 1.0f ));
				}
				if (ImGui::BeginTabItem(FixedString256::Format("Render Camera %i", i + 1)))
				{
					if (gCamEdData.cameras[i].activated)
					{
						ImGui::PopStyleColor();
					}
					EditorRenderCamera* cam = &gCamEdData.cameras[i];
					if (ImGui::Checkbox(FixedString256::Format("Activate Camera %i", i + 1), &gCamEdData.cameras[i].activated))
					{
						//Get entity and all that Jazz
						if (gCamEdData.cameras[i].activated)
						{
							if (gCamEdData.cameras[i].ent == INVALID_ENTITY)
							{
								gCamEdData.cameras[i].ent = levelState.ecs.GetEntity();

								levelState.transformComponents.AddComponent(gCamEdData.cameras[i].ent).transform = gCamEdData.cameras[i].transform;
							}
						}
						else
						{
							gCamEdData.cameraActive = false;
							//Return entity and component
							levelState.transformComponents.RemoveComponent(gCamEdData.cameras[i].ent);
							levelState.ecs.ReturnEntity(gCamEdData.cameras[i].ent);
							gCamEdData.cameras[i].ent = INVALID_ENTITY;
						}
						////nulling other cameras
						//for (size_t otherCameras = 0; otherCameras < NUMBOF_RENDERTARGETS; otherCameras++)
						//{
						//	if (i != otherCameras)
						//	{
						//		gCamEdData.cameras[otherCameras].beingEdited = false;
						//	}
						//}
					}
					ImGui::SameLine();
					if (ImGui::Button(FixedString256::Format("Reset Camera %i", i + 1)))
					{
						gCamEdData.cameras[i] = EditorRenderCamera();
						gCamEdData.renderTarget.camera.RecalculateProjectionMatrix(cam->fov, { 1920.0f, 1080.0f }, cam->perspectiveProjection, cam->nearClipDistance, cam->farClipDistance);
						gCamEdData.cameraActive = gCamEdData.cameras[i].activated;
					}
					if (gCamEdData.cameras[i].activated)
					{
						gCamEdData.cameras[i].transform.SetScale(v3f(1, 1, 1));
						if (gCamEdData.cameras[i].ent == INVALID_ENTITY)
						{
							gCamEdData.cameras[i].ent = levelState.ecs.GetEntity();

							levelState.transformComponents.AddComponent(gCamEdData.cameras[i].ent).transform = gCamEdData.cameras[i].transform;
						}
						CU::Transform& trans = levelState.transformComponents.GetComponent(cam->ent).transform;
						gCamEdData.cameraActive = true;
						gCamEdData.activeIndex = i;
						v3f& pos = trans.GetPosition();
						ImGui::DragFloat3("Camera Position", (float*)&pos[0]);
						v3f& rot = trans.GetRotation();
						ImGui::DragFloat3("Camera Rotation", (float*)&rot[0], 1.0f, -360.f, 360.f, "%.1f");
						//CameraControls(aDT, gCamEdData.renderTarget.camera, modEdData.cameraSettings, ModelViewerInternal::FindMiddle(modEdData.currentModel));

						ImGui::Checkbox("Perspective", &cam->perspectiveProjection);
						ImGui::DragFloat("Near plane distance", (float*)&cam->nearClipDistance, 10.0f, 15.f, cam->farClipDistance - 200.0f);
						cam->nearClipDistance = CU::Max(10.f, cam->nearClipDistance);
						ImGui::DragFloat("Far plane distance", (float*)&cam->farClipDistance, 10.0f,cam->nearClipDistance + 200.0f, 50000.f);
						cam->farClipDistance = CU::Max(cam->nearClipDistance + 200.0f, cam->farClipDistance);
						ImGui::DragFloat("FOV", (float*)&cam->fov, 1.f, 10.f, 120.f);
						v2ui& resolution = gCamEdData.cameras[i].renderResolution;
						if (gCamEdData.cameras[i].perspectiveProjection == false)
						{
							ImGui::DragInt("Resolution Width", (int*)&resolution.x, 1, 1000, 50000);
							ImGui::DragInt("Resolution Heigth", (int*)&resolution.y, 1, 1000, 50000);


							const char* items[] = { "All Passes", "Wireframe", "No Lighting" };
							ImGui::Combo("combo", &gCamEdData.currentRenderMode, items, IM_ARRAYSIZE(items));
							switch (gCamEdData.currentRenderMode)
							{
							case(0):
								gCamEdData.renderTarget.renderFlag = RenderFlag_NoUiOrPost;
								break;
							case(1):
								gCamEdData.renderTarget.renderFlag = RenderFlag_Wireframe;
								break;
							case(2):
								gCamEdData.renderTarget.renderFlag = RenderFlag_NoLighting;
								break;
							default:
								break;
							}
						}
						gCamEdData.renderTarget.camera.RecalculateProjectionMatrix(cam->fov, resolution, cam->perspectiveProjection, cam->nearClipDistance, cam->farClipDistance);
 						gCamEdData.cameras[i].frustumData = gCamEdData.renderTarget.camera.GetFrustumDrawData();
						m4f& camTtrans = gCamEdData.renderTarget.camera.GetMatrix();
						camTtrans = trans.GetMatrix();
						gCamEdData.renderTarget.camera.GetTransform() = trans;
						cam->transform = trans;
					}
			
					ImGui::EndTabItem();
				}
				else
				{
					if (gCamEdData.cameras[i].activated)
					{
						ImGui::PopStyleColor();
					}
				}
				
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
		return result;
	}
	bool DrawCamerasInEditor()
	{
		return gCamEdData.renderGizmos;
	}
	int GetCameraEntity(unsigned int aCameraIndex)
	{
		return gCamEdData.cameras[aCameraIndex].ent;
	}
	void* GetSceneCameras()
	{
		return gCamEdData.cameras;
	}
}