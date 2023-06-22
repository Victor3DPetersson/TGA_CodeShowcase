struct FrameGeometryToPixel
{
    float4 myPosition   : SV_Position;
    float4 myColor      : COLOR;
    float2 myUV         : UV;
    float2 myData      : DATA;
    float2 mySize       : SIZE;
    float2 myImgPos     : IMGPOS;
    float4 OGUV         : OGUV;
};
