#include "../CommonBuffers.hlsli"

struct VertexInput
{
	float4 myPosition	: SV_POSITION;
	float3 myNormal		: NORMAL;
	float3 myBinormal	: BINORMAL;
	float3 myTangent	: TANGENT;
	float4 myColor		: COLOR;
	float2 myUV			: TEXCOORD0;
	float2 myUV1		: TEXCOORD1;
	float2 myUV2		: TEXCOORD2;
	float2 myUV3		: TEXCOORD3;

	float4 myWorldPosition	: VPOS;
};

struct PixelOutput
{
	float4 myColor : SV_TARGET;
};

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

float Fresnel(float aScale, float someFresnelPower, float3 aCameraDirection, float3 aNormal)
{
	float fresnel = pow(1.0 + dot(aCameraDirection, aNormal), someFresnelPower);
	//fresnel = saturate(1 - fresnel);
	return aScale * fresnel;
}

PixelOutput main(VertexInput input)
{
	PixelOutput output;

	float simpleNoise = noise(input.myUV * 7);
	float2 frontUVPanning = input.myUV * 2.5f;
	frontUVPanning.y += totalTime * 0.25f + simpleNoise;

	float2 uvBubbles = input.myUV * 2.25f;
	uvBubbles.y += totalTime * 0.5f + noise(input.myUV * 3) * ((sin(totalTime * 0.5f) * 0.5f + 0.5f) * 0.25f);
	
	float2 uvDown = input.myUV;
	uvDown.y -= totalTime * 0.15f + noise(input.myUV * 1.5);
	uvDown.x += totalTime * 0.025f + noise(input.myUV * 2);

	float2 uvUPNoNoise = input.myUV;
	uvUPNoNoise .y += totalTime * 0.25f;
	float cloudsUp = materialTexture2.Sample(defaultSampler, uvUPNoNoise).g;

	float4 splat1Animated_faster = materialTexture1.Sample(defaultSampler, uvBubbles);

	float4 splat1Animated = materialTexture1.Sample(defaultSampler, frontUVPanning);
	float3 splat2Animated = materialTexture2.Sample(defaultSampler, frontUVPanning).rgb;

	float3 splat2Noise = materialTexture2.Sample(defaultSampler, uvDown).rgb;

	float4 splat1Still = materialTexture1.Sample(defaultSampler, input.myUV);
	float3 color = materialTexture3.Sample(defaultSampler, input.myUV);
	float alpha = input.myColor.r;

	float fresnel = Fresnel(1.75, 1.5, float3(renderCamera.toView._m20, renderCamera.toView._m21, renderCamera.toView._m22), input.myNormal );
	float waves = splat2Animated.r * pow(fresnel, 1.75);
	float glowyBits = saturate(pow(splat1Animated_faster.b * pow((1 - fresnel), 5.75), 3)) * 2; 

	float firePan = splat2Noise.b;

	output.myColor = 1;
	output.myColor.rgb = color;
	output.myColor.r += waves * 0.45f + abs(glowyBits * sin(totalTime) * 0.25f);
	output.myColor.g += waves * 1.75f + abs(glowyBits * sin(totalTime) * 5.9f) + (pow(firePan, 3) * 2 * (cloudsUp * 0.5f));
	output.myColor.b += waves * 0.55f + abs(glowyBits * sin(totalTime) * 0.8f);
	output.myColor.rgb *= pow(fresnel, 0.5) + 0.4f;
	output.myColor.rgb = lerp(output.myColor.rgb, effectData.color.rgb, effectData.tValue);
	output.myColor.a = lerp(alpha * pow(fresnel, 0.5), effectData.color.a, effectData.tValue);
	//output.myColor.rgb = pow(firePan, 3);
	return output;
}