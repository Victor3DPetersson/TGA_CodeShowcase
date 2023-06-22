#include "World_UIStructs.hlsli"


VertexToGeometry main( VertexInput input )
{
	VertexToGeometry output;
	float4 vertexViewPos = mul(renderCamera.toView, float4(input.myPosition, 1));

	output.myPosition = vertexViewPos.xyz;
	output.myColor = input.myColor;
	output.mySize = input.mySize;
	output.myUVOffsetTL = input.myUVOffsetTL;	
	output.myUVOffsetBR = input.myUVOffsetBR;	
	output.myPivotOffset = input.myPivotOffset;
	output.myRotation = input.myRotation;
	return output;
}