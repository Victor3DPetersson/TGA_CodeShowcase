#include "UIStructs.hlsli"

Texture2D maskTexture : register(t1);

PixelOutput main(GeometryToPixel input)
{
	PixelOutput result;
	float4 texCol = uiImage.Sample(defaultSampler, input.myUV);
	float2 offsetUV = input.myData + input.myUV;
	float4 offsetCol = maskTexture.Sample(defaultSampler, offsetUV);
	result.myColor = float4(texCol.rgb, texCol.a * offsetCol.r) * input.myColor.rgba;
	//result.myColor = float4(0.0f, .95f, .15f, 1.0f);
	//result.myColor.rgb = pow(abs(result.myColor.rgb), 2.2f);
	return result;
}