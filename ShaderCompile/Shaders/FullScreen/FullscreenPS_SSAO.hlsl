#include "FullScreenStructs.hlsli"


PixelOutput main(VertexToPixel input)
{
	PixelOutput output;
	float3 pixelNormal = tangentNormal_metalness_Pass.Sample(pointSampler, input.myUV).rgb * 2.0 - 1.0f;
	pixelNormal = mul(renderCamera.toView, float4(pixelNormal, 0)).xyz;
	float3 pixelWorldPos = worldPos_detailStrength_Pass.Sample(pointSampler, input.myUV).rgb;
	float3 pixelVSpos = mul((renderCamera.toView), float4(pixelWorldPos, 1.0f));
	float pixelDepth = depth_Pass.Sample(pointSampler, input.myUV).r;
	pixelDepth = worldDepthFromCameraZDepth(renderCamera.nearPlane, renderCamera.farPlane, pixelDepth);

	const float2 noiseScale = float2(frameRenderResolutionX / NOISE_RotationalTextureSize, frameRenderResolutionY / NOISE_RotationalTextureSize);
    
	float3 randomRotation = rotationalNoiseTexture.Sample(trilinearWrapSampler, input.myUV * noiseScale).rgb;
	float3 tangent = normalize(randomRotation - pixelNormal * dot(randomRotation, pixelNormal));
	float3 bitangent = cross(pixelNormal, tangent);
	float3x3 TBN = (float3x3(tangent, bitangent, pixelNormal));

	float radius = pp_SSAO_scale; //0.5f
	radius += lerp(renderCamera.nearPlane * 0.5f, renderCamera.farPlane * 0.015f, pixelDepth / (renderCamera.farPlane - renderCamera.nearPlane));
	float bias = pp_SSAO_bias; //1.0f

	float occlusion = 0.0f;
	for (int i = 0; i < NOISE_HemisphereMaxAmount; ++i)
	{
		float3 samplePos = mul(NOISE_halfHemisphere[i].xyz, TBN);
		samplePos = samplePos * radius + pixelVSpos;
		
		float4 offset = float4(samplePos, 1.0f);
		offset = mul((renderCamera.projection), offset);
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5f + 0.5f;

		float sampleDepth = depth_Pass.SampleLevel(pointSampler, float2(offset.x, 1 - offset.y), 0).r;
		sampleDepth = worldDepthFromCameraZDepth(renderCamera.nearPlane, renderCamera.farPlane, sampleDepth);
		float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(pixelDepth - sampleDepth));
		occlusion += (sampleDepth < pixelDepth - bias ? 1.0f : 0.0f) * rangeCheck;
	}
	occlusion = 1.0f - (occlusion / (NOISE_HemisphereMaxAmount)) * (occlusion / (NOISE_HemisphereMaxAmount));
	
	output.myColor = float4( occlusion, 0, 0, 1);
	return output;
}