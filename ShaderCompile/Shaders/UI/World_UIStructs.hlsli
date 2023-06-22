#include "../CommonBuffers.hlsli"

struct VertexInput
{
    float4 myColor          : COLOR;
    float3 myPosition       : POSITION;
    float2 mySize           : SIZE;
    float2 myUVOffsetTL     : TEXCOORD0;
    float2 myUVOffsetBR     : TEXCOORD1;
    float2 myPivotOffset    : TEXCOORD2;
    float myRotation        : ROTATION;
};

struct VertexToGeometry
{
    float4 myColor          : COLOR;
    float3 myPosition       : POSITION;
    float2 mySize           : SIZE;
    float2 myUVOffsetTL     : TEXCOORD0;
    float2 myUVOffsetBR     : TEXCOORD1;
    float2 myPivotOffset    : TEXCOORD2;
    float myRotation        : ROTATION;
};

struct GeometryToPixel
{
    float4 myPosition   : SV_Position;
    float4 myColor      : COLOR;
    float2 myUV         : UV;
    float2 myTrash      : TRASH;
};

struct PixelOutput
{
    float4 myColor      : SV_TARGET;
};

Texture2D uiImage : register(t8);