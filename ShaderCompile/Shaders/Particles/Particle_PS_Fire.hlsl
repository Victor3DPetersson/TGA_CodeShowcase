#include "Particle_Structs.hlsli"

PixelOutput main(GeometryToPixel input)
{
PixelOutput output;
	float4 TextureColor = ParticleTexture.Sample(defaultSampler, input.myUV);

	if(TextureColor.a == 0) { discard; }
	float uvPanSpeed = 1 + input.myUVPanningSpeed;
	float2 UVPanning1 = input.myUV * 0.15f;
	UVPanning1.y += totalTime * uvPanSpeed * 0.05f;

	float2 UVPanning2 = input.myUV * 0.025f;
	UVPanning2.y += totalTime * uvPanSpeed * 0.0025;

	float4 cloudNoise = ParticleTexture1.Sample(defaultSampler, UVPanning1).rgba * 4;
	float4 cloudNoiseFaster = ParticleTexture1.Sample(defaultSampler, UVPanning2).rgba * 2;

	output.myColor.rgba = TextureColor * input.myColor.rgba * input.myEmissiveStrength * cloudNoise * cloudNoiseFaster * input.myColor.a;
	//output.myColor.rgba = cloudNoiseFaster;
	//output.myColor.a = TextureColor.a * input.myColor.a;
	return output;
}