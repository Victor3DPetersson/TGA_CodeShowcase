#include "../CommonBuffers.hlsli"
struct VertexInput
{
    float4 myColor          : COLOR;
    float2 myPosition       : POSITION;
    float2 mySize           : SIZE;
    float2 myUVOffsetTL     : TEXCOORD0;
    float2 myUVOffsetBR     : TEXCOORD1;
    float2 myPivotOffset    : TEXCOORD2;
    float myRotation        : ROTATION;
    float2 myData           : DATA;
    int myZIndex            : ZINDEX;
    int2 myPadding          : PADDING;
};

struct VertexToGeometry
{
    float4 myPosition       : POSITION;
    float4 myColor          : COLOR;
    float2 mySize           : SIZE;
    float2 myUVOffsetTL     : TEXCOORD0;
    float2 myUVOffsetBR     : TEXCOORD1;
    float2 myPivotOffset    : TEXCOORD2;
    float myRotation        : ROTATION;
    float2 myData           : DATA;
};

struct GeometryToPixel
{
    float4 myPosition   : SV_Position;
    float4 myColor      : COLOR;
    float2 myUV         : UV;
    float2 myData      : DATA;
};

struct PixelOutput
{
    float4 myColor      : SV_TARGET;
};

Texture2D uiImage : register(t8);

static const float FLT_EPSILON = 1.192092896e-07F;