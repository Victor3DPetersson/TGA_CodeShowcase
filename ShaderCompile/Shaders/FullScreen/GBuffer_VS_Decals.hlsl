#include "../CommonBuffers.hlsli"

struct VertexInput
{
	float3 position	: POSITION;
	float3 normal		: NORMAL;
	float3 binormal	: BINORMAL;
	float3 tangent	: TANGENT;
	float4 color		: COLOR;
	float2 UV			: TEXCOORD0;
	float2 UV1		: TEXCOORD1;
	float2 UV2		: TEXCOORD2;
	float2 UV3		: TEXCOORD3;
	uint InstanceID		: SV_InstanceID;
};

struct VertexToPixel
{
	float4 position	: SV_POSITION;
	float3 normal		: NORMAL;
	float3 binormal	: BINORMAL;
	float3 tangent	: TANGENT;
	float4 color		: COLOR;
	float2 UV			: TEXCOORD0;
	float2 UV1		: TEXCOORD1;
	float2 UV2		: TEXCOORD2;
	float2 UV3		: TEXCOORD3;

	float4 worldPosition	: VPOS;
	uint InstanceID		: SV_InstanceID;
};

VertexToPixel main(VertexInput input)
{
    VertexToPixel returnValue;

    float4 vertexObjectPos = float4(input.position.xyz, 1);
    float4 vertexWorldPos = mul(decalFromOB_toWorld, vertexObjectPos);
    float4 vertexViewPos = mul(renderCamera.toView, vertexWorldPos);
    float4 vertexProjectionPos = mul(renderCamera.projection, vertexViewPos);

    float3x3 toWorldRotation = (float3x3) decalFromOB_toWorld;
    float3 vertexWorldNormal = mul(toWorldRotation, input.normal);
    float3 vertexWorldBinormal = mul(toWorldRotation, input.binormal);
    float3 vertexWorldTangent = mul(toWorldRotation, input.tangent);

    returnValue.position = vertexProjectionPos;
    returnValue.worldPosition = vertexWorldPos;
    returnValue.normal = vertexWorldNormal;
    returnValue.binormal = vertexWorldBinormal;
    returnValue.tangent = vertexWorldTangent;
    returnValue.color = input.color;
	returnValue.InstanceID = input.InstanceID;
    returnValue.UV = input.UV;
    return returnValue;
}