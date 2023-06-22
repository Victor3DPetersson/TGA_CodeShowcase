struct VertexInput
{
    unsigned int myIndex : SV_VertexID;
};


struct VertexToPixel
{
    float4 myPosition : SV_POSITION;
    float2 myUV : UV;
};
struct PixelOutput
{
    float4 myColor : SV_TARGET;
};

struct PointLight
{
    float4 myColor;
    float3 myPosition;
    float myRange;
    unsigned int lightFlag;
    float3 padding;
};
struct Camera{
     float4x4 toView;
    float4x4 fromView;
    float4x4 projection;
    float nearPlane;
    float farPlane;
    float padding1;
    float padding2;
};
struct SpotLight
{
    float4 myColor;
    float3 myPosition;
    float myRange;
    float3 myDirection;
    float myAngle;
    unsigned int lightFlag;
    float3 offset;
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

cbuffer lightBuffer : register(b1)
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
    float4x4 OB_toWorld;
	float4x4 fromWorld_toOB;
    float2 OB_UVScale;
    float2 junk;
}

struct DynamicSpotLightBuffer
{
    SpotLight mySpotLight_Dynamic;
    Camera mySpotlightCamera;
};
struct DynamicPointLightBuffer
{
    PointLight myPointLightDynamic;
};

//Add StructuredBuffer of lights

TextureCube environmentTexture : register(t0);

Texture2D worldPosPass          : register(t1);
Texture2D albedoPass            : register(t2);
Texture2D normalPass            : register(t3);
Texture2D vertexNormalPass      : register(t4);
Texture2D materialPass          : register(t5);
Texture2D EmissiveAOPass        : register(t6);
Texture2D staticDepthPass       : register(t7);
Texture2D vertexColorPass       : register(t8);

StructuredBuffer<PointLight> pointLights    : register(t9);
StructuredBuffer<SpotLight> spotLights      : register(t10);

Texture2D directionalShadowMap      : register(t11);
Texture2D spotLightShadowMap        : register(t12);
TextureCube pointLightShadowMap     : register(t13);
Texture2D depthPass                 : register(t14);

StructuredBuffer<DynamicSpotLightBuffer> dynamicSpotLights    : register(t15);
StructuredBuffer<DynamicPointLightBuffer> dynamicPointLights      : register(t16);

Texture2D spotLightShadowMap0        : register(t17);
Texture2D spotLightShadowMap1        : register(t18);
Texture2D spotLightShadowMap2        : register(t19);
Texture2D spotLightShadowMap3        : register(t20);
Texture2D spotLightShadowMap4        : register(t21);
Texture2D spotLightShadowMap5        : register(t22);
Texture2D spotLightShadowMap6        : register(t23);
Texture2D spotLightShadowMap7        : register(t24);
Texture2D spotLightShadowMap8        : register(t25);
Texture2D spotLightShadowMap9        : register(t26);

TextureCube pointLightShadowMap0        : register(t27);
TextureCube pointLightShadowMap1        : register(t28);
TextureCube pointLightShadowMap2        : register(t29);
TextureCube pointLightShadowMap3        : register(t30);
TextureCube pointLightShadowMap4        : register(t31);
TextureCube pointLightShadowMap5        : register(t32);
TextureCube pointLightShadowMap6        : register(t33);
TextureCube pointLightShadowMap7        : register(t34);
TextureCube pointLightShadowMap8        : register(t35);
TextureCube pointLightShadowMap9        : register(t36);

SamplerState defaultSampler : register(s0);
SamplerState pointSampler   : register(s1);