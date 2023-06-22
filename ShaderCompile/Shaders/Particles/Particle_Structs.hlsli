#include "../CommonBuffers.hlsli"

struct VertexInput
{
    float4 myColor              : COLOR;
    float3 myPosition           : POSITION;
    float3 myVelocity           : VELOCITY;
    float2 mySize               : SIZE;
    float myLifeTime            : LIFETIME;
    float myEndTime             : TIME;
    float myDistToCam           : DISTANCE;
    float myEmissiveStrength    : EMISSIVE;
    float myRotation            : ROTATION;
    float myRotationDir         : ROTATIONDIR;
    float myUVPanningSpeed      : PANNINGSPEED;
    unsigned int myCurrentColor : CURRENT_COLOR;
    float myRotationY           : ROTATIONY;
    float myRotationZ           : ROTATIONZ;
    float mySizeZ               : SIZEZ;
    uint myBillboardState        : BILLBOARDSTATE;
};

struct VertexToGeometry
{
    float4 myPosition : POSITION;
    float4 myColor : COLOR;
    float2 mySize : SIZE;
    float myEmissiveStrength : EMISSIVE;
    float myDistToCam           : DISTANCE;
    float myRotation            : ROTATION;
    float myDrag                : DRAG;
    float myUVPanningSpeed      : PANNINGSPEED;
    float2 myRotationXY         : ROTATIONXY;
    float mySizeZ               : SIZEZ;
    uint myBillboardState   : BILLBOARDSTATE;
};

struct GeometryToPixel
{
    float4 myPosition : SV_POSITION;
    float4 myColor : COLOR;
    float2 myUV : UV;
    float myEmissiveStrength : EMISSIVE;
    float myDistToCam           : DISTANCE;
    float myUVPanningSpeed      : PANNINGSPEED;
};

struct PixelOutput
{
    float4 myColor : SV_TARGET;
};

Texture2D ParticleTexture  : register(t8);
Texture2D ParticleTexture1 : register(t9);
Texture2D ParticleTexture2 : register(t10);
Texture2D DepthTexture : register(t27);
