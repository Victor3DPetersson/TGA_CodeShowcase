#include "../Light/PBRFunctions_Probes_SH.hlsli"
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

cbuffer tempSH_Buff : register(b6)
{
	float4 data;
}
TextureCube cubeMapTexture : register(t8);

float4 main(VertexToPixel input) : SV_TARGET
{
	float3 color;
	int time = (int)(totalTime * 0.5f) % 2;
	color = (time == 0) ? irradcoeffs( levelReflectionProbes[data.x].irradianceLight, input.myNormal * -1.f ) : ReflectionProbesTextures.Sample(defaultSampler, float4(input.myNormal, data.x)).rgb; 
	return float4(color, 1);
}