#include "../CommonBuffers.hlsli"
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
	uint InstanceID		: SV_InstanceID;
};
float4 main(VertexToPixel input) : SV_TARGET
{
	float4 color = materialTexture1.Sample(defaultSampler, input.myUV);

	if(color.a == 0.f)
	{
		return color;
	}
	float scanSpeedAdd = 4.0f;

	// Change this value to change scanline size (> = smaller lines).
	float lineCut = 0.0004;

	// Reduce 'anaglyphIntensity' value to reduce eye stress.
	// Adding this two values should result in 1.0.

	float3 col_r = float3(0.0, 1.0, 1.0);
	float3 col_l = float3(1.0, 0.0, 0.0);

	float whiteIntensity = 0.18f;
	float anaglyphIntensity = 0.15;
    float2 uv = input.myUV;
    float2 uv_right = float2(uv.x + 0.01, uv.y + 0.01);
    float2 uv_left = float2(uv.x - 0.01, uv.y - 0.01);

    // Black screen.
    float3 col = 0.0;
    
    // Measure speed.
    float scanSpeed = (frac(totalTime) * 2.5 / 40.0) * scanSpeedAdd;
    
    // Generate scanlines.s
    float3 scanlines = 1.0 * abs(cos((uv.y + scanSpeed) * 100.0)) - lineCut;
    
    // Generate anaglyph scanlines.
    float3 scanlines_right = col_r * abs(cos((uv_right.y + scanSpeed) * 100.0)) - lineCut;
    float3 scanlines_left = col_l * abs(cos((uv_left.y + scanSpeed) * 100.0)) - lineCut;
    
    // First try; a strange mess.
    //vec3 scanlines = cos(cos(sqrt(uv.y)*tan(iTime / 10000.0) * 100.0 * 10.0) * vec3(1.0) * 100.0);
    
    col = smoothstep(0.1, 0.7, scanlines * whiteIntensity)
        + smoothstep(0.1, 0.7, scanlines_right * anaglyphIntensity)
        + smoothstep(0.1, 0.7, scanlines_left * anaglyphIntensity);
    
    // Deform test (WIP, thanks to 'ddoodm' for its Simple Fisheye Distortion!).
    float2 eyefishuv = (uv - 0.5) * 2.5;
    float deform = (1.0 - eyefishuv.y*eyefishuv.y) * 0.02 * eyefishuv.x;
    //deform = 0.0;
    
    // Add texture to visualize better the effect.
    
    // Add vignette effect.
    float bottomRight = pow(uv.x, uv.y * 100.0);
    float bottomLeft = pow(1.0 - uv.x, uv.y * 100.0);
    float topRight = pow(uv.x, (1.0 - uv.y) * 100.0);
    float topLeft = pow(uv.y, uv.x * 100.0);
    
    float screenForm = bottomRight
        + bottomLeft
        + topRight
        + topLeft;

    // Invert screenForm color.
    float3 col2 = smoothstep(0.0, 0.0001f, 1 - screenForm);
    
    // Output to screen.
    // Invert last 0.1 and 1.0 positions for image processing.
    float4 fragColor = color + float4((smoothstep(0.01, 0.25, col) * 0.1), 1.0);
    fragColor = float4(fragColor.rgb * 1, color.a);
	return fragColor;
}