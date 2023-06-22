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
	float4 myColor		: COLOR;
	uint InstanceID		: SV_InstanceID;
};

VertexToPixel main(VertexInput input)
{
	VertexToPixel returnValue;

	float4 vWeights = input.myWeights;
	uint4 vBones = input.myBoneIDs;
	float4x4 toBoneTransform;
	float distance = length(input.myPosition - renderCameraPosition);
	float4 vertexOffsettedPos = float4(input.myPosition, 1) + float4(normalize(input.myNormal), 0) * distance * 0.005f;
	float4 skinnedPos;
	float4 vertexWorldPos;

	ModelEffect objectEffectData;
	if(effectData.gBufferPSEffectIndex % 2 == 0)
	{
		objectEffectData = EffectBuffer[input.InstanceID];
	} 
	else
	{
		objectEffectData = effectData;
	}

	if(objectEffectData.gBufferVSEffectIndex == 0)
	{
		toBoneTransform = mul(SkeletonBuffer[input.InstanceID * 128 + vBones.x].bone, vWeights.x); 
		toBoneTransform += mul(SkeletonBuffer[input.InstanceID * 128 + vBones.y].bone, vWeights.y); 
		toBoneTransform += mul(SkeletonBuffer[input.InstanceID * 128 + vBones.z].bone, vWeights.z); 
		toBoneTransform += mul(SkeletonBuffer[input.InstanceID * 128 + vBones.w].bone, vWeights.w); 

		skinnedPos = mul(toBoneTransform, vertexOffsettedPos);
    	vertexWorldPos = mul(OBs_ToWorldBuffer[input.InstanceID], skinnedPos);
	}
	else if(objectEffectData.gBufferVSEffectIndex == 1)
	{
		toBoneTransform = mul(myBones[vBones.x], vWeights.x); 
		toBoneTransform += mul(myBones[vBones.y], vWeights.y); 
		toBoneTransform += mul(myBones[vBones.z], vWeights.z); 
		toBoneTransform += mul(myBones[vBones.w], vWeights.w); 

		skinnedPos = mul(toBoneTransform, vertexOffsettedPos);
    	vertexWorldPos = mul(fromOB_toWorld, skinnedPos);
	}

	float4 vertexViewPos = mul(renderCamera.toView, vertexWorldPos);
	float4 vertexProjectionPos = mul(renderCamera.projection, vertexViewPos);

	returnValue.myPosition	= vertexProjectionPos;
	returnValue.myColor		= input.myColor;
	return returnValue;
}