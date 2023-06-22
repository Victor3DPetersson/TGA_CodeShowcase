#include "../CommonBuffers.hlsli"
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

static const float GaussianKernel[5] =
{ 0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f };
