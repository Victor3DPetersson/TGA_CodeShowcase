#include "FullScreenStructs.hlsli"

#define SAMPLES 17
#define LOD 2
#define LODs 1 << SAMPLES
//   const int samples = 35,
//           LOD = 2,         // gaussian done on MIPmap at scale LOD
//           sLOD = 1 << LOD; // tile size = 2^LOD
float gaussian(float2 i) {
    return exp( -.5* dot(i/=SAMPLES * .25f,i) ) / ( 6.28 * (SAMPLES * .25f) * (SAMPLES * .25f) );
}
float4 blur(float2 U, float2 scale) {
    float4 O = 0;  
    int s = SAMPLES/LODs;
    
    for ( int i = 0; i < s*s; i++ ) {
        float2 d = float2(i%s, i/s)*float(LODs) - float(SAMPLES)/2.;
        O += gaussian(d) * materialTexture1.SampleLevel( defaultSampler, U + scale * d , float(LOD) );
    }
    return O / O.a;
}

PixelOutput main(VertexToPixel input)
{
	PixelOutput output;
	output.myColor = blur( input.myUV, 1.f / (float2(frameRenderResolutionX, frameRenderResolutionY) * 0.5f) );
	return output;
}