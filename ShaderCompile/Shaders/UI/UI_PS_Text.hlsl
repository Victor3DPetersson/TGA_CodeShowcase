#include "UIStructs.hlsli"

PixelOutput main(GeometryToPixel input)
{
	PixelOutput result;
	float4 texCol = uiImage.Sample(defaultSampler, input.myUV);
	texCol.a = texCol.r;
	if (texCol.a == 0)
		discard;
	result.myColor = texCol.rgba * input.myColor.rgba;
	//result.myColor.rgb = pow(abs(result.myColor.rgb), 2.2f);
	return result;
}