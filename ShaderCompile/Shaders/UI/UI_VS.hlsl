#include "UIStructs.hlsli"

VertexToGeometry main( VertexInput input )
{
	VertexToGeometry output;
	float4 position = float4(input.myPosition.xy, 0, 1);
	output.myPosition = position;
	output.myColor = input.myColor;
	output.mySize = input.mySize;
	output.myUVOffsetTL = input.myUVOffsetTL;	
	output.myUVOffsetBR = input.myUVOffsetBR;	
	output.myPivotOffset = input.myPivotOffset;
	output.myRotation = input.myRotation;
	output.myData = input.myData;
	return output;
}