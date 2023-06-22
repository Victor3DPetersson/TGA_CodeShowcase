#include "../CommonBuffers.hlsli"
struct VertexInput
{
    float4 myColor : COLOR;
    float4 myPosFrom : POSITION;
    float4 myPosTo : POSITION1;
    float mySize : SIZE;
    float3 myTrash : TRASH;
};

struct DS_VertexInput
{
    float4 myPosition : POSITION;
	uint InstanceID		: SV_InstanceID;
};

struct VertexToGeometry
{
    float4 myColor      : Color;
    float4 myPosFrom    : POSITION;
    float4 myPosTo      : POSITION1;
    float mySize : SIZE;
};

struct GeometryToPixel
{
    float4 myPosition   : SV_POSITION;
    float4 myColor      : Color;
};

struct DS_VertexToPixel
{
    float4 myPosition : SV_POSITION;
    float4 myColor      : Color;
};

struct PixelOutput
{
    float4 myColor : SV_TARGET;
};

struct DebugSphereData
{
    float4x4 MVP;
    float4 color;
    float radius;
    float3 padding;
};

StructuredBuffer<DebugSphereData> myStructuredSphereData : register(t29);


