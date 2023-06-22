#define EPSILON 0.001f
#define NOISE_HemisphereMaxAmount 64
#define DIRECTIONALLIGHT_RESOLUTION 4096.f
#define DIRECTIONALLIGHT_SIZE 0.00001f
float Map(float v, float min1, float max1, float min2, float max2)
{
	return (v - min1) / (max1 - min1) * (max2 - min2) + min2;
}

float worldDepthFromCameraZDepth(float aNear, float aFar, float aZ)
{
    return -(aNear * aFar) / (aZ * aFar - aNear * aZ - aFar);
}

static const float ratio16_9 = 16.0f / 9.0f;

struct Camera{
    float4x4 toView;
    float4x4 fromView;
    float4x4 projection;
    float nearPlane;
    float farPlane;
    float clusterPreNumerator;
    float clusterPreDenominator;
    float4x4 invProjection;
};
struct ShadowCamera{
    float4x4 toView;
    float4x4 projection;
};

struct Skeleton
{
	row_major float4x4 bone;
};

//Color A = Strength
struct PointLight
{
    float4 myColor;
    float3 myPosition;
    float myRange;
    float3 padding;
    float size;
};
struct  PointLightShadowData
{
    uint4 shadowAtlasIndex;
};
//Color A = Strength
struct SpotLight
{
    float4 myColor;
    float3 myPosition;
    float myRange;
    float3 myDirection;
    float myAngle;
    float3 padding;
    float size;
};
struct SpotLightShadowData
{
    uint4 shadowAtlasIndex;
};

struct ModelEffect
{
	float4 color;
	float4 outlineColor;
  	unsigned int gBufferPSEffectIndex;
	unsigned int gBufferVSEffectIndex;
	unsigned int meshIndex;
	float tValue;
};

struct SH //Spherical-Harmonic
{
    float4 bands[9];
};

struct ReflectionProbe
{
    float3 position;
    float reflectionProbePadding1;
    float outerRadius;
    float innerRadius;
    float brightness;
    float reflectionProbePadding2;
    SH irradianceLight;
};

struct DynamicSpotLightBuffer
{
    SpotLight mySpotLight_Dynamic;
    Camera mySpotlightCamera;
};

struct DynamicPointLightBuffer
{
    PointLight myPointLightDynamic;
};


//---------Changes only when screen size changes or render resolution changes----------//
cbuffer ScreenResBuffer : register(b0)
{
    float windowResolutionX;
    float windowResolutionY;
    float frameRenderResolutionX; 
    float frameRenderResolutionY;
};
//---------This is set at beginning of rendering and only once each frame ------------//
cbuffer GlobalFrameBuffer : register(b1)
{

    float4 directionalLightColor;
    float3 directionalLightDirection;
    float deltaTime;
    
    float4 globalFogColor;
    float4 globalAmbientLightColor;

    float globalNearFogDistance;
    float globalFarFogDistance;
    float globalFogExponent;   
    float SH_GridSpacing;

    float3 levelMiddle;
    float totalTime;
    float3 levelHalfSize;
    float DoF_focusDistance;

    uint gPointLightCount;
    uint gSpotLightCount;
    uint gLevelRProbeAmount;
    uint gLevelSHGridAmount;
};

cbuffer DefinesBuffer : register(b2)
{
    unsigned int CLUSTER_WIDTH;
    unsigned int CLUSTER_HEIGTH;
    unsigned int CLUSTER_DEPTH;
    unsigned int lightIndexMapMaxSize;

    unsigned int SHADOWMAP_size;
    unsigned int SHADOWMAP_tileSize;
    unsigned int SHADOWMAP_tileAmount;
    unsigned int SHADOWMAP_tileTotal;

    unsigned int NOISE_RotationalTextureSize;
    uint gNumberOfMipsEnvironment;
    uint2 defines_padding;
}
StructuredBuffer<PointLight> clusteredPointLights                       : register(t0);
StructuredBuffer<PointLightShadowData> clusteredPointLightsShadowData   : register(t1);
StructuredBuffer<SpotLight> clusteredSpotLights                         : register(t2);
StructuredBuffer<SpotLightShadowData> clusteredSpotLightsShadowData     : register(t3);

struct cluster{   // A cluster volume is represented using an AABB
   float4 minPoint; // We use vec4s instead of a vec3 for memory alignment purposes
   float4 maxPoint;
};
//Model Buffers for rendering normalMeshes
cbuffer ObjectBuffer : register(b3)
{
	float4x4 fromOB_toWorld;
};

cbuffer ObjectBoneBuffer : register(b4)
{
	row_major float4x4 myBones[128];
};

cbuffer ObjectEffectBuffer : register(b5)
{
	ModelEffect effectData;
};

cbuffer DecalObjectBuffer : register(b6)
{
    float4x4 decalFromOB_toWorld;
    float4x4 decalFromWorld_toOB;
};

cbuffer RenderCameraBuffer : register(b7)
{
    Camera renderCamera; 
    float3 renderCameraPosition;
    float renderCamerPadding;
}
struct PoissonSamples
{
    float2 samples[32];
};
cbuffer NoiseBuffer : register(b8)
{
    float4 NOISE_halfHemisphere[NOISE_HemisphereMaxAmount]; 
    float4 poissonSamples[16];
}
static float2 NOISE_PoissonSamples[32] = (float2[32])poissonSamples;
struct SH_GridData
{
    float4x4 toGrid;
    float3 halfSize;
    float brightness;
    float spacing;
    uint numberOfProbes;
    uint offset;
    uint padding;
};
cbuffer LightProbeBuffer : register(b9)
{
    SH_GridData SH_DebugGrid;
    float4 padding[2];
}

cbuffer PostProcessingVariables : register(b10)
{
    float           pp_SSAO_scale;
    float           pp_SSAO_bias;
    float           pp_VIGNETTE_strength;
    float           pp_VIGNETTE_extent;

    float           pp_CA_Strength;
    float           pp_BLOOM_strength;
    float           pp_FOG_nearDistance;
    float           pp_FOG_farDistance;

    float3          pp_FOG_color;   
    float           pp_FOG_exponent; 
    float3          pp_colorFilter;
    float           pp_postExposure;
    float3          pp_splitToneShadowTint;
    float           pp_contrast;
    float3          pp_splitToneHighLightTint;
    float           pp_saturation;
    float3          pp_channelMixerR;
    float           pp_splitToningBalance;
    float3          pp_channelMixerG;
    float           pp_hueShift;
    float3          pp_channelMixerB;
    float           pp_SMH_shadowStart;
    float3          pp_SMH_shadows;
    float           pp_SMH_shadowEnd;
    float3          pp_SMH_midtones;
    float           pp_SMH_highlightStart;
    float3          pp_SMH_highlights;
    float           pp_SMH_highlightEnd;

};

//-----------------Instancing--------------//
StructuredBuffer<float4x4> OBs_ToWorldBuffer    : register(t4);
StructuredBuffer<ModelEffect> EffectBuffer      : register(t6);
StructuredBuffer<Skeleton> SkeletonBuffer       : register(t7);

//Textures and their Texture Slots

Texture2D materialTexture1                      : register(t8);//Albedo
Texture2D materialTexture2                      : register(t9);//Normal
Texture2D materialTexture3                      : register(t10);//Material
Texture2D materialTexture4                      : register(t11);//Emissive
Texture2D materialTexture5                      : register(t12);
Texture2D materialTexture6                      : register(t13);
Texture2D materialTexture7                      : register(t14);
Texture2D materialTexture8                      : register(t15);
Texture2D materialTexture9                      : register(t16);
Texture2D materialTexture10                     : register(t17);
Texture2D materialTexture11                     : register(t18);
Texture2D materialTexture12                     : register(t19);
TextureCube environmentTexture                  : register(t20); 

Texture2D worldPos_detailStrength_Pass          : register(t21);
Texture2D albedo_AO_Pass                        : register(t22);
Texture2D tangentNormal_metalness_Pass          : register(t23); 
Texture2D vertexNormal_Roughness_Pass           : register(t24); 
Texture2D emissive_emissiveStrength_Pass        : register(t25); 
Texture2D staticDepth_Pass                      : register(t26);
Texture2D depth_Pass                            : register(t27);
Texture2D shadowAtlasMap                        : register(t28);
Texture2D<uint> clusterIndexMap                 : register(t29);
Texture3D<uint4> clusterTexture                 : register(t30);
StructuredBuffer<ShadowCamera> shadowCameras    : register(t31);
Texture2D directionalShadowMap                  : register(t32);
Texture2D rotationalNoiseTexture                : register(t33);
StructuredBuffer<SH> SH_globalIrradiance        : register(t34);
Texture2D PBR_BRDFLut                           : register(t35);
TextureCubeArray ReflectionProbesTextures       : register(t36);
StructuredBuffer<ReflectionProbe> levelReflectionProbes : register(t37);
StructuredBuffer<SH_GridData> levelSHGrids : register(t38);
Texture2D blur_filterConstants                : register(t39);


SamplerState defaultSampler         : register(s0);
SamplerState pointSampler           : register(s1);
SamplerState trilinearWrapSampler   : register(s2);
SamplerComparisonState linearClampComparisonSampler   : register(s3);






