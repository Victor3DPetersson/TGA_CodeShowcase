#include "../CommonBuffers.hlsli"

struct VertexInput
{
	float3 myPosition	: POSITION;
	float3 myNormal		: NORMAL;
	float3 myBinormal	: BINORMAL;
	float3 myTangent	: TANGENT;
	float4 myColor		: COLOR;
	float2 myUV			: TEXCOORD0;
	float2 myUV1		: TEXCOORD1;
	float2 myUV2		: TEXCOORD2;
	float2 myUV3		: TEXCOORD3;
	uint InstanceID		: SV_InstanceID;
};

struct VertexToPixel
{
	float4 myPosition	: SV_POSITION;
	float4 myColor		: COLOR;
	uint InstanceID		: SV_InstanceID;
};


VertexToPixel main(VertexInput input)
{
	VertexToPixel returnValue;
	float distance = length(input.myPosition - renderCameraPosition);
	float4 vertexOffsettedPos = float4(input.myPosition, 1) + float4(normalize(input.myNormal), 0) * distance * 0.005f;
	float4 vertexWorldPos;

	if(effectData.gBufferVSEffectIndex == 0)
	{
		vertexWorldPos = mul(OBs_ToWorldBuffer[input.InstanceID], vertexOffsettedPos);
	}
	else if(effectData.gBufferVSEffectIndex == 1)
	{
		vertexWorldPos = mul(fromOB_toWorld, vertexOffsettedPos);
	}
	float4 vertexViewPos = mul(renderCamera.toView, vertexWorldPos);
	float4 vertexProjectionPos = mul(renderCamera.projection, vertexViewPos);
	returnValue.myPosition	= vertexProjectionPos;
	returnValue.myColor		= input.myColor;
	return returnValue;
}