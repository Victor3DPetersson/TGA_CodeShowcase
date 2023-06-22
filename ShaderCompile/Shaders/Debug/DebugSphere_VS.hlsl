#include "DebugStructs.hlsli"

DS_VertexToPixel main( DS_VertexInput input )
{
	DS_VertexToPixel output;
	const float radius = myStructuredSphereData[input.InstanceID].radius * 100;
	// float4x4 scaleMatrix = {
	// 	radius, 0.0f, 0.0f, 0.0f,
	// 	0.0f, radius, 0.0f, 0.0f,
	// 	0.0f, 0.0f, radius, 0.0f,
	// 	0.0f, 0.0f, 0.0f, 1.0f
	// };
	// float4x4 transform = {
	// 	1.0f, 0.0f, 0.0f, 0.0f,
	// 	0.0f, 1.0f, 0.0f, 0.0f,
	// 	0.0f, 0.0f, 1.0f, 0.0f,
	// 	input.myPosition.xyz, 1.0f
	// };
	float4 pos = float4(input.myPosition.xyz * radius, 1.0f);
	//transform = mul(scaleMatrix, transform);		
	float4 vertexProjectionPos	= mul(myStructuredSphereData[input.InstanceID].MVP, pos);
	
	output.myPosition = vertexProjectionPos;
	output.myColor = myStructuredSphereData[input.InstanceID].color;

	return output;
}