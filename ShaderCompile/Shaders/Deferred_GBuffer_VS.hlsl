#include "CommonBuffers.hlsli"
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

VertexToPixel main(VertexInput input) 
{
	VertexToPixel returnValue;
	ModelEffect objectEffectData;
	if(effectData.gBufferVSEffectIndex % 2 == 0)
	{
		objectEffectData = EffectBuffer[input.InstanceID];
	} 
	else
	{
		objectEffectData = effectData;
	}


    float4 vertexObjectPos = float4(input.myPosition.xyz, 1);
    float4 vertexWorldPos;

    float3x3 toWorldRotation;
	if(objectEffectData.gBufferVSEffectIndex == 0)
	{
		vertexWorldPos = mul(OBs_ToWorldBuffer[input.InstanceID], vertexObjectPos);
		toWorldRotation = (float3x3) OBs_ToWorldBuffer[input.InstanceID];
	}
	else if(objectEffectData.gBufferVSEffectIndex == 1)
	{
		vertexWorldPos = mul(fromOB_toWorld, vertexObjectPos);
		toWorldRotation = (float3x3) fromOB_toWorld;
	}
    float3 vertexWorldNormal = mul(toWorldRotation, input.myNormal);
    float3 vertexWorldBinormal = mul(toWorldRotation, input.myBinormal);
    float3 vertexWorldTangent = mul(toWorldRotation, input.myTangent);

	float4 vertexViewPos = mul(renderCamera.toView, vertexWorldPos);
	float4 vertexProjectionPos = mul(renderCamera.projection, vertexViewPos);

    returnValue.myPosition = vertexProjectionPos;
    returnValue.myWorldPosition = vertexWorldPos;
    returnValue.myNormal = vertexWorldNormal;
    returnValue.myBinormal = vertexWorldBinormal;
    returnValue.myTangent = vertexWorldTangent;
    returnValue.myColor = input.myColor;
    returnValue.myUV = input.myUV;
    returnValue.myUV1 = input.myUV1;
    returnValue.myUV2 = input.myUV2;
    returnValue.myUV3 = input.myUV3;
	returnValue.InstanceID = input.InstanceID;
    return returnValue;
}