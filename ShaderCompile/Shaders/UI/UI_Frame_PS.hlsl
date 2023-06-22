#include "UIStructs.hlsli"
#include "UIFrame.hlsli"

float CalculateAxis(float coord, float textureBorder, float windowBorder, float tileSize)
{
	if (coord <= windowBorder)
	{
		return Map(coord, 0, windowBorder, 0, textureBorder);
	}
	if (coord <= 1 - windowBorder)
	{
		coord = windowBorder + fmod(coord - windowBorder, tileSize);
		return Map(coord, windowBorder, 1 - windowBorder, textureBorder, 1 - textureBorder);
	}
	return Map(coord, 1 - windowBorder, 1, 1 - textureBorder, 1);
}

PixelOutput main(FrameGeometryToPixel input)
{
	float2 uvSize = input.OGUV.zw - input.OGUV.xy;
	float2 size = input.mySize;

	float2 textureBorders = input.myData;
	uint2 imgSize;
	uiImage.GetDimensions(imgSize.x, imgSize.y);
	imgSize *= 2.0f;
	imgSize *= uvSize;
	float2 windowBorders = textureBorders * (imgSize / size);
	float2 tileSize = (imgSize - imgSize * float2(textureBorders * 2)) / size;

	float2 uv = float2(
		CalculateAxis(input.myUV.x, textureBorders.x, windowBorders.x, tileSize.x),
		CalculateAxis(input.myUV.y, textureBorders.y, windowBorders.y, tileSize.y)
	);

	uv.x = Map(uv.x, 0, 1, input.OGUV.x, input.OGUV.z);
	uv.y = Map(uv.y, 0, 1, input.OGUV.y, input.OGUV.w);

	PixelOutput result;
	result.myColor = uiImage.Sample(defaultSampler, uv) * input.myColor.rgba;
	if (result.myColor.a == 0.0f) discard;
	//result.myColor.rgb = pow(abs(result.myColor.rgb), 2.2f);
	return result;
}