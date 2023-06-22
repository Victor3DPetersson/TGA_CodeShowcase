#include "FullScreenStructs.hlsli"
PixelOutput main(VertexToPixel input)
{
    PixelOutput returnValue;
    returnValue.myColor.rgb = materialTexture1.Sample(defaultSampler, input.myUV).rgb;
    returnValue.myColor.a = 1.0f;
	return returnValue;
}