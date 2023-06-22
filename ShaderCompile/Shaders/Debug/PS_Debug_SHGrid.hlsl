#include "../light/PBRFunctions_Probes_SH.hlsli"
struct VertexToPixel
{
	float4 myPosition	: SV_POSITION;
	float3 myNormal		: NORMAL;
	float3 myBinormal	: BINORMAL;
	float3 myTangent	: TANGENT;
	float4 myColor		: COLOR;
	float2 myUV			: TEXCOORD0;
	float2 myUV1		: TEXCOORD1;
	float2 myUV2		: TEXCOORD2;
	float2 myUV3		: TEXCOORD3;

	float4 myWorldPosition	: VPOS;
	uint InstanceID		: SV_InstanceID;
};


float4 main(VertexToPixel input) : SV_TARGET
{
	float3 gridPos =  input.myWorldPosition.xyz;
	gridPos = mul(SH_DebugGrid.toGrid, float4(gridPos, 1)).xyz;//Rotate in to the grid's space
	gridPos += ((SH_DebugGrid.halfSize) + SH_DebugGrid.spacing * 0.5f); //place the pixel in the correct position of the space
	int width = int((SH_DebugGrid.halfSize.x * 2) / SH_DebugGrid.spacing);
	int heigth = int((SH_DebugGrid.halfSize.y * 2) / SH_DebugGrid.spacing);
	int depth = int((SH_DebugGrid.halfSize.z * 2) / SH_DebugGrid.spacing);
	int currentW = int((gridPos.x - 23) / SH_DebugGrid.spacing);
	int currentH = int((gridPos.y - 23) / SH_DebugGrid.spacing);
	int currentD = int((gridPos.z - 23) / SH_DebugGrid.spacing);
	int index = (currentD * heigth * width) + (currentH * width) + currentW + SH_DebugGrid.offset;
	float3 color = irradcoeffs( SH_globalIrradiance[index], float3(input.myNormal * -1.f) );

	return float4(color, 1.0f);
}