#include "FullScreenStructs.hlsli"
PixelOutput main(VertexToPixel input)
{
    PixelOutput result;
    
    float2 resolution = float2(frameRenderResolutionX, frameRenderResolutionY);
    float2 uv = input.myUV.xy;

    //float ChromaticAberration = 100.0f * max(totalTime - 100, 0) * 0.001f;
    float ChromaticAberration = pp_CA_Strength;

    float2 texel = 1.0 / resolution.xy;
    
    float2 coords = (uv - 0.5) * 2.0;
    float coordDot = dot (coords, coords);
    
    float2 precompute = ChromaticAberration * coordDot * coords;
    float2 uvR = uv - texel.xy * precompute;
    float2 uvB = uv + texel.xy * precompute;
    
    float4 color;
    color.r = materialTexture1.Sample(defaultSampler, uvR).r;
    color.g = materialTexture1.Sample(defaultSampler, uv).g;
    color.b = materialTexture1.Sample(defaultSampler, uvB).b;
    color.a = 1;
    result.myColor = color;
    return result;
}