#include "../CommonBuffers.hlsli"

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

uint main(VertexToPixel input) : SV_TARGET
{
	if(effectData.gBufferVSEffectIndex == 0)
	{
		return EffectBuffer[input.InstanceID].meshIndex;
	}
	else if(effectData.gBufferVSEffectIndex == 1)
	{
		return effectData.meshIndex;
	}
	return 0;
}