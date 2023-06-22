#include "Particle_Structs.hlsli"

PixelOutput main(GeometryToPixel input)
{
	PixelOutput output;
	float TextureColor = ParticleTexture.Sample(defaultSampler, input.myUV).g;
	if(TextureColor == 0) { discard; }

	float uvPanSpeed = 1 + input.myUVPanningSpeed;
	float2 UVPanning1 = input.myUV * 0.075f;
	UVPanning1.x += totalTime * uvPanSpeed * 0.001f;
	UVPanning1.y += totalTime * uvPanSpeed * 0.001f;

	float2 UVPanning2 = input.myUV * 0.25f;
	UVPanning2.x += totalTime * uvPanSpeed * 0.0085;
	UVPanning2.y += totalTime * uvPanSpeed * 0.0085;

	float cloudNoise = ParticleTexture1.Sample(defaultSampler, UVPanning1).r * 1.75;
	float cloudNoiseFaster = ParticleTexture1.Sample(defaultSampler, UVPanning2).r * 1.05;

	float2 NormalizedPixelPos = float2(input.myPosition.x / frameRenderResolutionX, input.myPosition.y / frameRenderResolutionY);
	float z = DepthTexture.Sample(defaultSampler, NormalizedPixelPos).r;

	if(z == 1)
	{
		output.myColor.rgba = TextureColor * input.myColor.rgba * input.myEmissiveStrength * cloudNoise * cloudNoiseFaster;
		return output;
	}

	float near = renderCamera.nearPlane;
	float far = renderCamera.farPlane;
	float worldDepth = -(near * far) / (z * far - near * z - far);

    float particleDepth = -(near * far) / (input.myPosition.z * far - near * input.myPosition.z - far);
	float distance = worldDepth - particleDepth;

	float alphaThreshold = 1;
	if(distance < 0 )
	{
		discard;
	}
	if(distance < 250)
	{
		alphaThreshold = saturate(Map(distance, 100.0f, 250.f, 0, 1));
	}
	output.myColor.rgb = TextureColor * input.myColor.rgb * input.myEmissiveStrength * cloudNoise * cloudNoiseFaster * alphaThreshold * input.myColor.a;
	output.myColor.a = TextureColor * input.myColor.a * alphaThreshold;
	return output;
}