#include "DebugStructs.hlsli"

VertexToGeometry main( VertexInput input )
{

	float4 fromViewPos = mul(renderCamera.toView, input.myPosFrom);
	fromViewPos = mul(renderCamera.projection, fromViewPos);

	float4 toViewPos = mul(renderCamera.toView, input.myPosTo);
	toViewPos = mul(renderCamera.projection, toViewPos);

	VertexToGeometry output;
	output.myPosFrom = fromViewPos;
	output.myPosTo = toViewPos;
	output.myColor = input.myColor;
	output.mySize = input.mySize;
	return output;
}