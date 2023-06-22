#include "FullScreenStructs.hlsli"


#define mod(x, y) (x - y * floor(x / y))

float hash(float n) { return frac(sin(n) * 1e4); }
float hash(float2 p) { return frac(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(float2 x) {
	float2 i = floor(x);
	float2 f = frac(x);

	// Four corners in 2D of a tile
	float a = hash(i);
	float b = hash(i + float2(1.0, 0.0));
	float c = hash(i + float2(0.0, 1.0));
	float d = hash(i + float2(1.0, 1.0));

	// Simple 2D lerp using smoothstep envelope between the values.
	// return float3(lerp(lerp(a, b, smoothstep(0.0, 1.0, f.x)),
	//			lerp(c, d, smoothstep(0.0, 1.0, f.x)),
	//			smoothstep(0.0, 1.0, f.y)));

	// Same code, with the clamps in smoothstep and common subexpressions
	// optimized away.
	float2 u = f * f * (3.0 - 2.0 * f);
	return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}


PixelOutput main(VertexToPixel input)
{

	PixelOutput returnValue;
	//float2 NRR = float2( WindowResolutionX / RenderResolutionX, WindowResolutionY / RenderResolutionY);

	float ratioX = max(windowResolutionX / windowResolutionY, ratio16_9);
	float ratioY = min(windowResolutionX / windowResolutionY, ratio16_9);
	float scaleFactor;
	if(ratioX > ratio16_9)
	{
		scaleFactor = windowResolutionY / frameRenderResolutionY;
	}
	else
	{
		scaleFactor = windowResolutionX / frameRenderResolutionX;
	}
	float2 NormalizedOffsetDist = float2((1 - (frameRenderResolutionX * scaleFactor) / windowResolutionX) * 0.5f,  (1 - (frameRenderResolutionY * scaleFactor) / windowResolutionY) * 0.5f);
	float2 NormalizedPixelPos = float2(input.myPosition.x / windowResolutionX, input.myPosition.y / windowResolutionY);
	float2 PixelOffsetDist = float2((windowResolutionX - (frameRenderResolutionX * scaleFactor)) * 0.5f, (windowResolutionY - (frameRenderResolutionY * scaleFactor)) * 0.5f);


	if(NormalizedPixelPos.x < NormalizedOffsetDist.x || NormalizedPixelPos.x > 1 - NormalizedOffsetDist.x ||
	NormalizedPixelPos.y < NormalizedOffsetDist.y || NormalizedPixelPos.y > 1 - NormalizedOffsetDist.y)
	{
		returnValue.myColor = float4(0, 0, 0, 1);
		return returnValue;
	}


	//float genNoise = 2.f*(noise(NormalizedPixelPos * 2000) -0.5f);



	float2 NormalizedUVCoord = float2(Map(NormalizedPixelPos.x, NormalizedOffsetDist.x, 1 - NormalizedOffsetDist.x, 0, 1),
	Map(NormalizedPixelPos.y, NormalizedOffsetDist.y, 1 - NormalizedOffsetDist.y, 0, 1));

    returnValue.myColor.rgb = materialTexture1.Sample(defaultSampler, NormalizedUVCoord).rgb;
	//returnValue.myColor.rgb = float3(0, 0, 0.5f);

	//returnValue.myColor.rgb +=  genNoise * pow(returnValue.myColor.rgb, 1.2f)/2.2f;

    returnValue.myColor.a = 1.0f;
	//returnValue.myColor = saturate(returnValue.myColor);
	return returnValue;
}