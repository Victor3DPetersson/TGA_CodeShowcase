#include "../CommonBuffers.hlsli"


Texture2D inputTexture : register( t8 );
RWTexture2D<float> outputTexture : register( u0 );

cbuffer cbFixed
{
    static const int gBlurSize = 4;
    static const int gHBlurSize = gBlurSize / 2;
}

#define N 20
#define CacheSize (N * N + (gBlurSize * N) * 2 + (N * N))
groupshared float gCache[CacheSize];
[numthreads(N, N, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GROUPTHREADID)
{
    
    //uint2 texCoord = DTid.xy;
    //int xBound = max(DTid.x - gBlurSize, 0);
    uint xDim = 0;
    uint yDim = 0;
    uint2 imageDim = 0;
    inputTexture.GetDimensions(xDim, yDim);
    imageDim = uint2(xDim, yDim);
    int2 cachePos = GTid + gHBlurSize;
    if(GTid.x == 0 && GTid.y == 0)
    {
        int2 xy = int2(max(DTid.x - gHBlurSize, 0), max(DTid.y - gHBlurSize, 0));
        for(int h = 0; h < gHBlurSize; h++)
        {
            for(int w = 0; w < gHBlurSize; w++)
            {
                gCache[w + (h * N)] = inputTexture[float2(xy.x + w, xy.y + h)];
            }
        }
    }
    if(GTid.x == N - 1 && GTid.y == 0)
    {
        int2 xy = int2(DTid.x, max(DTid.y - gHBlurSize, 0));
        for(int h = 0; h < gHBlurSize; h++)
        {
            for(int w = 0; w < gHBlurSize; w++)
            {
                gCache[(w + N + gHBlurSize) + (h * N)] = inputTexture[float2(min(xy.x + w, imageDim.x - 1), xy.y + h)];
            }
        }
    }
    if(GTid.x == 0 && GTid.y == N - 1)
    {
        int2 xy = int2(max(DTid.x - gHBlurSize, 0), DTid.y);
        for(int h = 0; h < gHBlurSize; h++)
        {
            for(int w = 0; w < gHBlurSize; w++)
            {
                gCache[(w) + (h + N + gHBlurSize) * N] = inputTexture[float2(xy.x + w, min(xy.y + h, imageDim.y - 1))];
            }
        }
    }
    if(GTid.x == N - 1 && GTid.y == N - 1)
    {
        int2 xy = int2(DTid.x, DTid.y);
        for(int h = 0; h < gHBlurSize; h++)
        {
            for(int w = 0; w < gHBlurSize; w++)
            {
                gCache[(w + N + gHBlurSize) + (h + N + gHBlurSize) * N] = inputTexture[float2(min(xy.x + w, imageDim.x - 1), min(xy.y + h, imageDim.y - 1))];
            }
        }
    }

    if(GTid.x == 0)
    {
        int x = max(DTid.x - gHBlurSize, 0);
        for(int w = 0; w < gHBlurSize; w++)
        {
            gCache[w + (cachePos.y * N)] = inputTexture[int2(x + w, DTid.y)];
        }
    }
    if(GTid.y == 0)
    {
        int y = max(DTid.y - gHBlurSize, 0);
        for(int h = 0; h < gHBlurSize; h++)
        {
            gCache[cachePos.x + (h * N)] = inputTexture[int2(DTid.x, y + h)];
        }
    }
    if(GTid.x == N - 1)
    {
        for(int w = 0; w < gHBlurSize; w++)
        {
            gCache[cachePos.x + 1 + w + cachePos.y * N] = inputTexture[int2(min(DTid.x + w + 1, imageDim.x - 1), DTid.y)];
        }
    }
    if(GTid.y == N - 1)
    {
        int y = min(DTid.y + gBlurSize, imageDim.y - 1);
        for(int h = 0; h < gHBlurSize; h++)
        {
            gCache[cachePos.x + (y + 1 + h) * N] = inputTexture[int2(DTid.x, min(DTid.y + h + 1, imageDim.y - 1))];
        }
    }

    gCache[(cachePos.x) + (cachePos.y) * N] = inputTexture[min(DTid.xy, imageDim.xy - 1)];
    GroupMemoryBarrierWithGroupSync();

    float2 texelSize = 1.0f / imageDim;
    float result = 0.0;
    float2 hlim = float(-gBlurSize) * 0.5 + 0.5f;
    float2 texCoord = DTid.xy;
    for (int h = 0; h < gBlurSize; h++) {
        for (int w = 0; w < gBlurSize; ++w) {
            //float2 offset = (hlim + float2(float(h), float(w))) * texelSize;
            //offset.x = max(0, min(1.0f, texCoord.x + offset.x));
           // offset.y = max(0, min(1.0f, texCoord.y + offset.y));
            //result += inputTexture.SampleLevel(defaultSampler, offset, 0);
            //result += inputTexture[DTid.xy + offset];
            result += gCache[cachePos.x - gHBlurSize + w + (cachePos.y - gHBlurSize + h) * N];
        }
    }
 
   outputTexture[DTid.xy] = result / float(gBlurSize * gBlurSize);

}