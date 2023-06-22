#include "FullScreenStructs.hlsli"
PixelOutput main(VertexToPixel input)
{
    PixelOutput result;
    float3 screen = materialTexture1.Sample(defaultSampler, input.myUV).rgb;
    
    float2 resolution = float2(frameRenderResolutionX, frameRenderResolutionY);
    float2 uv = input.myPosition.xy / resolution;
   
    uv *= 1.0 - uv.yx; //vec2(1.0)- uv.yx; -> 1.-u.yx; Thanks FabriceNeyret !
    
    float vig = uv.x * uv.y * pp_VIGNETTE_strength; // 10
    vig = saturate(pow(vig, pp_VIGNETTE_extent)); // change pow for modifying the extend of the  vignette
    vig = smoothstep(0, 0.5f, vig);
  
    result.myColor.rgb = screen * vig;
    result.myColor.a = 1.0f;
    return result;
}