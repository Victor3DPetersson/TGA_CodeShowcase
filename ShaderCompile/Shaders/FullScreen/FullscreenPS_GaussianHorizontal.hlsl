#include "FullScreenStructs.hlsli"
PixelOutput main(VertexToPixel input)
{
    PixelOutput result;
    
    float texelSize = 1.0f / (frameRenderResolutionX / 8);
    float3 blurColor = 0;
    
    unsigned int kernelSize = 5;
    float start = (((float)kernelSize - 1.0f) / 2.0f) * -1.0f;
    for (unsigned int idx = 0; idx < kernelSize; idx++)
    {
        float2 uv = clamp(input.myUV + float2(texelSize * (start + (float)idx), 0.0f), 0.f, 1.f);
        float3 resource = materialTexture1.Sample(defaultSampler, uv).rgb;
        blurColor += resource * GaussianKernel[idx];
    }
    result.myColor.rgb = blurColor;
    result.myColor.a = materialTexture1.Sample(defaultSampler, input.myUV).a;
    
    return result;
}