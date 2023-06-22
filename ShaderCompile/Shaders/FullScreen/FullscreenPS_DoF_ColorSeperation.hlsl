#include "FullScreenStructs.hlsli"
// Circular DOF: Kleber "Kecho" Garcia (c) 2017
#define KERNEL_RADIUS  8
#define KERNEL_COUNT 17
float4 fetchImage(float2 coords)
{
    float4 colorImg = materialTexture1.Sample(defaultSampler, coords);
    return colorImg;
    //luma trick to mimic HDR, and take advantage of 16 bit buffers shader toy provides.
    float lum = dot(colorImg.rgb,float3(0.2126,0.7152,0.0722))*1.8;
    colorImg = colorImg *(1.0 + 0.2 * lum*lum*lum);
    return colorImg*colorImg;
}

float4 getFilters(float x)
{
    return blur_filterConstants.Load(int3(x, 0, 0));
}

struct DepthSplitOutput
{
    float4 red : SV_Target0;
    float4 green : SV_Target1;
    float4 blue : SV_Target2;
};

Texture2D<float4> fullscreenTextureMask	: register(t11);

DepthSplitOutput main(VertexToPixel input)
{
	DepthSplitOutput output;
    float2 stepVal = (1.0 / (float2(frameRenderResolutionX, frameRenderResolutionY) * 0.5f)).xy;
    
    float4 valR = float4(0,0,0,0);
    float4 valG = float4(0,0,0,0);
    float4 valB = float4(0,0,0,0);
    float filterRadius = fullscreenTextureMask.Sample(defaultSampler, input.myUV).w;
    float3 sampleTexel = fetchImage(input.myUV).rgb;
    for (int i = 0; i < KERNEL_COUNT; i++)
    {
        float2 coords = input.myUV + stepVal*float2(float(i - KERNEL_RADIUS),0.0)*filterRadius;
        float3 imageTexels = fetchImage(coords).rgb;
        float mask = fullscreenTextureMask.Sample(defaultSampler, coords).w;
        imageTexels = imageTexels * imageTexels;
        imageTexels = lerp(sampleTexel * sampleTexel, imageTexels, smoothstep(0.375f, 0.75f, mask));
        float4 c0_c1 = getFilters(i);
        valR.xy += imageTexels.r * c0_c1.xy;
        valR.zw += imageTexels.r * c0_c1.zw;

        valG.xy += imageTexels.g * c0_c1.xy;
        valG.zw += imageTexels.g * c0_c1.zw;

        valB.xy += imageTexels.b * c0_c1.xy;
        valB.zw += imageTexels.b * c0_c1.zw;
    }

    output.red = valR;
    output.green = valG;
    output.blue = valB;
	return output;
}