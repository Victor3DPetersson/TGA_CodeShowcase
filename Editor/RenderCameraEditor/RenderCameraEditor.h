#pragma once

namespace Windows
{

	void RenderCameraEditorInit();
	bool RenderCameraEditor(float aDT, void*);
	bool DrawCamerasInEditor();
	int GetCameraEntity(unsigned int aCameraIndex);
	void* GetSceneCameras(); // Will return data of the type EditorRenderCamera
}