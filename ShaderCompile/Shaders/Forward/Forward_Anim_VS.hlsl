#include "../CommonBuffers.hlsli"

struct VertexInput
{
	float4 myColor		: COLOR;
	float3 myPosition	: POSITION;
	float3 myNormal		: NORMAL;
	float3 myBinormal	: BINORMAL;
	float3 myTangent	: TANGENT;
	float2 myUV			: TEXCOORD0;
	float2 myUV1		: TEXCOORD1;
	float2 myUV2		: TEXCOORD2;
	float2 myUV3		: TEXCOORD3;
	uint4 myBoneIDs 	: BONEIDS;
	float4 myWeights 	: BONEWEIGHTS;
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
	float4 cameraPosition : POSITION;
	uint InstanceID		: SV_InstanceID;
};


VertexToPixel main(VertexInput input)
{
	VertexToPixel returnValue;

	float4 vWeights = input.myWeights;
	uint4 vBones = input.myBoneIDs;

	float4x4 toBoneTransform;

	float4 skinnedPos;
    float4 vertexWorldPos;
    float3x3 toWorldRotation;


	if(effectData.gBufferVSEffectIndex == 0)
	{
		toBoneTransform = mul(SkeletonBuffer[input.InstanceID * 128 + vBones.x].bone, vWeights.x); 
		toBoneTransform += mul(SkeletonBuffer[input.InstanceID * 128 + vBones.y].bone, vWeights.y); 
		toBoneTransform += mul(SkeletonBuffer[input.InstanceID * 128 + vBones.z].bone, vWeights.z); 
		toBoneTransform += mul(SkeletonBuffer[input.InstanceID * 128 + vBones.w].bone, vWeights.w); 

		skinnedPos = mul(toBoneTransform, float4(input.myPosition.xyz, 1.0));
    	vertexWorldPos = mul(OBs_ToWorldBuffer[input.InstanceID], skinnedPos);
    	toWorldRotation = (float3x3)OBs_ToWorldBuffer[input.InstanceID];
	}
	else if(effectData.gBufferVSEffectIndex == 1)
	{
		toBoneTransform = mul(myBones[vBones.x], vWeights.x); 
		toBoneTransform += mul(myBones[vBones.y], vWeights.y); 
		toBoneTransform += mul(myBones[vBones.z], vWeights.z); 
		toBoneTransform += mul(myBones[vBones.w], vWeights.w); 

		skinnedPos = mul(toBoneTransform, float4(input.myPosition.xyz, 1.0));
    	vertexWorldPos = mul(fromOB_toWorld, skinnedPos);
    	toWorldRotation = (float3x3)fromOB_toWorld;
	}


  	float3x3 toBoneRotation = (float3x3)toBoneTransform;
    float3 vertexBoneNormal = mul(toBoneRotation, input.myNormal);
    float3 vertexBoneBinormal = mul(toBoneRotation, input.myBinormal);
    float3 vertexBoneTangent = mul(toBoneRotation, input.myTangent);

    float3 vertexWorldNormal = mul(toWorldRotation, vertexBoneNormal);
    float3 vertexWorldBinormal = mul(toWorldRotation, vertexBoneBinormal);
    float3 vertexWorldTangent = mul(toWorldRotation, vertexBoneTangent);

	float4 vertexViewPos = mul(renderCamera.toView, vertexWorldPos);
	float4 vertexProjectionPos = mul(renderCamera.projection, vertexViewPos);

    returnValue.myPosition = vertexProjectionPos;
    returnValue.myWorldPosition = vertexWorldPos;
    returnValue.myNormal = vertexWorldNormal;
    returnValue.myBinormal = vertexWorldBinormal;
    returnValue.myTangent = vertexWorldTangent;
    returnValue.myColor = input.myColor;

    returnValue.myUV =  float2(input.myUV.x, 1 - input.myUV.y);
	returnValue.myUV1 = float2(input.myUV1.x, 1 - input.myUV1.y);
	returnValue.myUV2 = float2(input.myUV2.x, 1 - input.myUV2.y);
	returnValue.myUV3 = float2(input.myUV3.x, 1 - input.myUV3.y);
	return returnValue;
}