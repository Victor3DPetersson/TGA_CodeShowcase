#include "FullScreenStructs.hlsli"
Texture2D inputTexture : register( t8 );

cbuffer cbFixed
{
    static const int gBlurSize = 4;
    static const int gHBlurSize = gBlurSize / 2;
}

float main(VertexToPixel input) : SV_TARGET
{
    uint xDim = 0;
    uint yDim = 0;
	uint2 imageDim = 0;
    inputTexture.GetDimensions(xDim, yDim);
    imageDim = uint2(xDim, yDim);
	
	float2 texelSize = 1.0f / imageDim;

    float result = 0.0;
    float2 hlim = float(-gBlurSize) * 0.5 + 0.5f;
    for (int h = 0; h < gBlurSize; h++) {
        for (int w = 0; w < gBlurSize; ++w) {
            float2 offset = (hlim + float2(float(h), float(w))) * texelSize;
            offset.x = max(0, min(1.0f, input.myUV.x + offset.x));
           	offset.y = max(0, min(1.0f, input.myUV.y + offset.y));
            result += inputTexture.Sample(defaultSampler, offset, 0);
        }
    }
	return result / float(gBlurSize * gBlurSize);
}