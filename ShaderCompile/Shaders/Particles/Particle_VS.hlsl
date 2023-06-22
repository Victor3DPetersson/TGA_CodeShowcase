#include "Particle_Structs.hlsli"

VertexToGeometry main( VertexInput input)
{
	VertexToGeometry output;

	float4 vertexObjectPos = float4(input.myPosition, 1);
	float4 vertexViewPos = mul(renderCamera.toView, vertexObjectPos);

	output.myPosition = vertexViewPos;
	output.myColor = input.myColor;
	output.mySize = input.mySize;
	output.myEmissiveStrength = input.myEmissiveStrength;
	output.myDistToCam = input.myDistToCam;
	output.myRotation = input.myRotation;
	output.myUVPanningSpeed = input.myUVPanningSpeed;
	output.myRotationXY = float2(input.myRotationY, input.myRotationZ);
	output.myBillboardState = input.myBillboardState;
	return output;
}