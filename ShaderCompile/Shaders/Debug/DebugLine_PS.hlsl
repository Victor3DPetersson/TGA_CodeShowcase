#include "DebugStructs.hlsli"

PixelOutput main(GeometryToPixel input)
{
	PixelOutput output;
	output.myColor = input.myColor;
	output.myColor.a = 1.0f;
	return output;
}