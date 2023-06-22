#include "DebugStructs.hlsli"

PixelOutput main(DS_VertexToPixel input)
{
	PixelOutput output;
	output.myColor = input.myColor;
	return output;
}