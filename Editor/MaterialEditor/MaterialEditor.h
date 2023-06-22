#pragma once
namespace Windows
{
	void MaterialEditorInit();
	void MaterialEditorDeInit();
	bool MaterialEditor(float aDT, void*);
	namespace MaterialEditorInternal
	{
		void HandleCreation();
		void HandleEditing(unsigned short& aIndex);
		void HandleImport();
		void HandleExport();
		void CreatePreWrittenMaterials();
		void ShowShaderElements(const char* aButtonName, unsigned int aShaderIndex, unsigned short aShaderConfig, unsigned short aMaterialIndex);
		bool ExportMaterialToEngine(unsigned short aMaterialIndex);
		bool ImportMaterialToEditor(const char* aPath, unsigned short aMaterialIndex, unsigned int* aMatType = nullptr);

		void InitSpheres();
	}
}