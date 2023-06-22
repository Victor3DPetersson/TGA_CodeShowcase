#pragma once
namespace CU
{
	template <class T> 
	class Vector3;
	template <class T>
	class Vector4;
}
namespace Engine
{
	namespace Noise
	{
		void CreateWeightedUnitHemisphere(const size_t aNumbOfsamples, CU::Vector4<float>* aArrayToFill);
		void CreateRandomKernelRotations(const size_t aNumberOfPixels, CU::Vector4<float>* aArrayToFill);
		void GeneratePoissonSphereSamples(CU::Vector2<float>* aArrayToFill);
	}
}