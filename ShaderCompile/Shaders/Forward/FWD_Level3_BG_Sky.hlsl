struct VertexToPixel
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
	float4 cameraPosition : POSITION;
};

struct PixelOutput
{
	float4 myColor : SV_TARGET;
};

struct Camera{
     float4x4 toView;
    float4x4 fromView;
    float4x4 projection;
    float nearPlane;
    float farPlane;
    float padding1;
    float padding2;
};

cbuffer ConstantBuffer : register(b0)
{
	float WindowResolutionX;
    float WindowResolutionY;
    float deltaTime;
	float totalTime;
    float RenderResolutionX; //Render source res
    float RenderResolutionY;
   	unsigned int myNumberOfPointLights;
    unsigned int myNumberOfSpotLights;
}

cbuffer LightBuffer : register(b1)
{
	float4 myDLightColor;
	float4 myDLightDirection;
	float4 myAmbientLightColor;
}

cbuffer FrameBuffer : register(b2)
{
	Camera renderCamera;
	unsigned int myEnvironmentLightMipCount;
	float3 myCameraPosition;
}

cbuffer ObjectBuffer : register(b3)
{
	float4x4 OB_toWorld;
	float4x4 fromWorld_toOB;
	float2 OB_UVScale;
    float2 MapMidPoint;
}


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


Texture2D cloudTexture : register(t1);
Texture2D stripeTexture : register(t2);
SamplerState defaultSampler : register(s0);

PixelOutput main(VertexToPixel input) : SV_TARGET
{

	float2 uv = input.myUV * 6.0f;

    uv = uv * 0.450f;
    uv.y += uv.y + totalTime * 0.5f + (sin(totalTime * 0.05f) * 0.5f + 0.5f);	

	float2 uvPanningH = input.myUV + noise(uv);
	float2 uvPanningH2 = input.myUV * 4.5f - noise(uvPanningH) - totalTime * 0.1f;
    	
	uvPanningH.x += uvPanningH.x + totalTime * 0.25f;

	float cloud = cloudTexture.Sample(defaultSampler, uvPanningH).b;
	float plasma = cloudTexture.Sample(defaultSampler, uvPanningH2).r;

	float3 color = float3(0.325, 0.165, 0.91f) * 0.15f;

	PixelOutput output;
	output.myColor = float4(color.r + cloud * 0.5f * plasma, color.g, color.b + cloud * input.myUV.x * plasma, 1.0f);
	return output;
}