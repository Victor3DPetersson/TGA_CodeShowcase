#pragma once
#include "Cmn.h"
namespace Windows
{
	void ParticleEditorInit();
	bool ParticleEditor(float aDT, void*);
	namespace ParticleEditorInternal
	{
		void ImGuiUpdate(float aDeltaTime, unsigned short aSystemIndex);
		void SystemUpdate(float aDeltaTime);
		void SetImGuiOptions(unsigned short aSystemIndex);
		//void UpdateSystem(unsigned short aSystemIndex);

		void CreateSystem(unsigned short aSystemIndex);
		void ResetSystem(unsigned short aSystemIndex, bool aResetName);


		bool ExportSystem(unsigned short aSystemIndex);

		void ImportSystemInternal(const char* aGratkey, bool aGetSettings, bool aShouldBlend);

		bool ReadGratSprut(const char* aGratKey, void* someDataToFill, char* aMaterialName, unsigned int& aMatType, GUID& aGUIDToRead);
		void BlendSystems();
		void LerpSettings(float aTVal);
		void ResetSystemEmission(unsigned short aSystemIndex);
		void DestroySystem(unsigned short aSystemIndex);
		void ImGuiParticleRotationSettings(const char* aAxis, float& aMinRotationDir, float& aMaxRotationDir, bool& aSpawnRandomRotation, bool& aRotateDir);
		void ImGuiParticleScaleSettings(const char* aAxis, float& aMinScale, float& aMaxScale);
	}
}