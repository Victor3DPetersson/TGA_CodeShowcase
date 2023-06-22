#include "FullScreenStructs.hlsli"
PixelOutput main(VertexToPixel input)
{
    PixelOutput result;
    
    float3 resource1 = materialTexture1.Sample(defaultSampler, input.myUV).rgb;
    float3 resource2 = materialTexture2.Sample(defaultSampler, input.myUV).rgb;
    
    result.myColor.rgb = resource1 + pow(smoothstep(0.2, 1.0f, resource2), pp_BLOOM_strength);
    result.myColor.a = 1.0f;
    
	return result;
}