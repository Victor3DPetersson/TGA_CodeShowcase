#include "stdafx.h"
#include "NoiseGeneration.h"

void Engine::Noise::CreateWeightedUnitHemisphere(const size_t aNumbOfsamples, v4f* aArrayToFill)
{
	v3f random;
	for (size_t i = 0; i < aNumbOfsamples; i++)
	{
		random.x = Random.RandNumbInRange(0.f, 1.f) * 2.0f - 1.0f;
		random.y = Random.RandNumbInRange(0.f, 1.f) * 2.0f - 1.0f;
		random.z = 1.f;
		random.Normalize();

		float scale = (float)i / aNumbOfsamples;
		scale = CU::Lerp(0.1f, 1.0f, scale * scale);
		random *= scale;
		aArrayToFill[i].x = random.x;
		aArrayToFill[i].y = random.y;
		aArrayToFill[i].z = random.z;
		aArrayToFill[i].w = 0.f;

	}
}

void Engine::Noise::CreateRandomKernelRotations(const size_t aNumberOfPixels, v4f* aArrayToFill)
{
	for (size_t i = 0; i < (aNumberOfPixels * aNumberOfPixels) * 2; i++)
	{
		aArrayToFill[i].x = Random.RandNumbInRange(0.f, 1.f) * 2.0f - 1.0f;
		aArrayToFill[i].y = Random.RandNumbInRange(0.f, 1.f) * 2.0f - 1.0f;
		aArrayToFill[i].z = 0;
		aArrayToFill[i].w = 0;
	}
}

void Engine::Noise::GeneratePoissonSphereSamples(CU::Vector2<float>* aArrayToFill)
{
	const int RESOLUTION = 32;
	for (size_t k = 0; k < RESOLUTION; ++k)
	{
		float randomAngle = static_cast<float>(rand()) / RAND_MAX * 2 * CU::pif;
		aArrayToFill[k] = v2f(cosf(randomAngle) * 0.5f + 0.5f, sinf(randomAngle) * 0.5f + 0.5f);
	}
}
