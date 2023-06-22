#include "FullScreenStructs.hlsli"
PixelOutput main(VertexToPixel input)
{
    PixelOutput returnValue;
    returnValue.myColor.rgb = materialTexture1.Sample(defaultSampler, input.myUV).rgb;
    returnValue.myColor.a = 1.0f;
	if(returnValue.myColor.r == 0.00f && returnValue.myColor.g == 0.00f && returnValue.myColor.b == 0.00f)
	{
		returnValue.myColor.rgb = 0;
		returnValue.myColor.a = 0;
	}
	return returnValue;
}