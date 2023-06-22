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

struct VertexInputAnim
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
};


struct PixelOutput
{
	float4 myColor : SV_TARGET;
};

#define NUM_LIGHTS 8

struct Camera{
     float4x4 toView;
    float4x4 fromView;
    float4x4 projection;
    float nearPlane;
    float farPlane;
    float padding1;
    float padding2;
};

cbuffer ConstantBuffer : register(b0)
{
	float WindowResolutionX;
    float WindowResolutionY;
    float myDeltaTime;
	float myTotalTime;
    float RenderResolutionX; //Render source res
    float RenderResolutionY;
   	unsigned int myNumberOfPointLights;
    unsigned int myNumberOfSpotLights;
}

cbuffer LightBuffer : register(b1)
{
    Camera myDirectionalLight;
    float4 directionalColor;
    float4 directionalDirection;
    float4 ambientColor;
    unsigned int muNumberOfDynamicPointLights;
    unsigned int muNumberOfDynamicSpotLights;
    unsigned int PADDING;
    unsigned int PADDING2;
}

cbuffer FrameBuffer : register(b2)
{
	Camera renderCamera;
	unsigned int myEnvironmentLightMipCount;
	float3 myCameraPosition;
}

cbuffer ObjectBuffer : register(b3)
{
	float4x4 fromOB_toWorld;
	float4x4 fromWorld_toOB;
	float4x4 MVP;
	float2 OB_UVScale;
    float2 MapMidPoint;
}

cbuffer BoneBuffer : register(b4)
{
	row_major float4x4 myBones[128];
};

cbuffer EffectBuffer : register(b5)
{
	float4 effectcolor;
    float3 EffectPADDING;
    float tValue;
}

struct PointLight
{
    float4 myColor;
    float3 myPosition;
    float myRange;
} myPointLights[NUM_LIGHTS];
struct SpotLight
{
    float4 myColor;
    float3 myPosition;
    float myRange;
    float3 myDirection;
    float myAngle;
} mySpotLights[NUM_LIGHTS];

struct Skeleton
{
	row_major float4x4 bone;
};

float Map(float v, float min1, float max1, float min2, float max2)
{
	return (v - min1) / (max1 - min1) * (max2 - min2) + min2;
}
StructuredBuffer<Skeleton> SkeletonBuffer : register(t16);
StructuredBuffer<float4x4> OBs_ToWorld : register(t0);
StructuredBuffer<float4x4> MVPModel : register(t1);

TextureCube environmentTexture : register(t18);

Texture2D colorTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D MRESTexture : register(t3);
Texture2D EmissiveTexture : register(t4);
Texture2D StaticDepthTexture : register(t5);
Texture2D WorldPosition : register(t6);
Texture2D VertexNormals : register(t7);
SamplerState defaultSampler : register(s0);

