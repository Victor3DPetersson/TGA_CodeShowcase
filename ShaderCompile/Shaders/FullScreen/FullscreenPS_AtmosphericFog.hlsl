#include "FullScreenStructs.hlsli"

static const float PI = 3.14159265f;
float easeInOutSine(float x)  
{
	return -(cos(PI * x) - 1) / 2;
}

#define mod(x, y) (x - y * floor(x / y))

float hash(float n) { return frac(sin(n) * 1e4); }
float hash(float2 p) { return frac(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(float x) {
	float i = floor(x);
	float f = frac(x);
	float u = f * f * (3.0 - 2.0 * f);
	return lerp(hash(i), hash(i + 1.0), u);
}

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

#define ACESccMidGray 0.4135884f
struct ParamsLogC
{
    float cut;
    float a, b, c, d, e, f;
};

static const ParamsLogC LogC =
{
    0.011361, // cut
    5.555556, // a
    0.047996, // b
    0.244161, // c
    0.386036, // d
    5.301883, // e
    0.092819  // f
};

float LinearToLogC_Precise(half x)
{
    float o;
    if (x > LogC.cut)
        o = LogC.c * log10(LogC.a * x + LogC.b) + LogC.d;
    else
        o = LogC.e * x + LogC.f;
    return o;
}

float3 LinearToLogC(float3 x)
{
#if USE_PRECISE_LOGC
    return float3(
        LinearToLogC_Precise(x.x),
        LinearToLogC_Precise(x.y),
        LinearToLogC_Precise(x.z)
    );
#else
    return LogC.c * log10(LogC.a * x + LogC.b) + LogC.d;
#endif
}

float LogCToLinear_Precise(float x)
{
    float o;
    if (x > LogC.e * LogC.cut + LogC.f)
        o = (pow(10.0, (x - LogC.d) / LogC.c) - LogC.b) / LogC.a;
    else
        o = (x - LogC.f) / LogC.e;
    return o;
}

float3 LogCToLinear(float3 x)
{
#if USE_PRECISE_LOGC
    return float3(
        LogCToLinear_Precise(x.x),
        LogCToLinear_Precise(x.y),
        LogCToLinear_Precise(x.z)
    );
#else
    return (pow(10.0, (x - LogC.d) / LogC.c) - LogC.b) / LogC.a;
#endif
}

float3 RgbToHsv(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = EPSILON;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HsvToRgb(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float RotateHue(float value, float low, float hi)
{
    return (value < low)
            ? value + hi
            : (value > hi)
                ? value - hi
                : value;
}

float Luminance(float3 linearRgb)
{
    return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750));
}

float3 SoftLight(float3 aColorBackground, float3 aColorSource)
{
	if(Luminance(aColorSource) < 0.5)
	{
		return  2 * aColorBackground * aColorSource + aColorBackground * aColorBackground * (1 - 2 * aColorSource);
	}
	else
	{
		return 2 * aColorBackground * (1 - aColorSource) + sqrt(aColorBackground) * (2 * aColorSource - 1);
	}
}

float3 ColorGradingShadowsMidtonesHighlights (float3 color) {
	float luminance = Luminance(color);
	float shadowsWeight = 1.0 - smoothstep(pp_SMH_shadowStart, pp_SMH_shadowEnd, luminance);
	float highlightsWeight = smoothstep(pp_SMH_highlightStart, pp_SMH_highlightEnd, luminance);
	float midtonesWeight = 1.0 - shadowsWeight - highlightsWeight;
	return
		color * pp_SMH_shadows * shadowsWeight +
		color * pp_SMH_midtones * midtonesWeight +
		color * pp_SMH_highlights * highlightsWeight;
}

float3 ColorGradingContrast(float3 aColor)
{
	aColor = LinearToLogC(aColor);
	aColor = (aColor - ACESccMidGray) * (pp_contrast * 0.01 + 1.f) + ACESccMidGray;
	aColor = LogCToLinear(aColor);
	return aColor;
}

float3 ColorGrading(float3 aInputcolor)
{

	float exposure = pow(pp_postExposure, 2.0f);
	float hueShift = pp_hueShift * (1.f / 360.f);
	float saturation = pp_saturation * 0.01 + 1.f;
	float splitColorBalance = pp_splitToningBalance * 0.01f;

	float3 adjustedColor = aInputcolor;
	adjustedColor *= exposure; //exposure
	adjustedColor = ColorGradingContrast(adjustedColor);

	adjustedColor *= pp_colorFilter;//color filter
	adjustedColor = max(adjustedColor, 0);

	float colorLuminance = Luminance(saturate(adjustedColor));
	float t = saturate(colorLuminance * splitColorBalance); // Split Toning
	//adjustedColor = pow(adjustedColor, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
	float3 shadows = lerp( 0.5f, pp_splitToneShadowTint, t);
	float3 highlights = lerp( 0.5f, pp_splitToneHighLightTint, t);
	if(colorLuminance <= 0.5f)
	{
		adjustedColor = SoftLight(adjustedColor, shadows);
	}
	else{
		adjustedColor = SoftLight(adjustedColor, highlights);
	}
	//adjustedColor = pow(adjustedColor, float3(2.2, 2.2, 2.2));

	adjustedColor = mul(float3x3(pp_channelMixerR, pp_channelMixerG, pp_channelMixerB), adjustedColor);//Channel mixing
	adjustedColor = max(adjustedColor, 0.0f);

	adjustedColor = ColorGradingShadowsMidtonesHighlights(adjustedColor);

	adjustedColor = RgbToHsv(adjustedColor); //Hue Shift
	float hue = adjustedColor.x + hueShift;
	adjustedColor.x = RotateHue(hue, 0.0, 1.0);
	adjustedColor = HsvToRgb(adjustedColor);

	float luminance = Luminance(adjustedColor);
	adjustedColor = (adjustedColor - luminance) * saturation + luminance; // Saturation


	return adjustedColor;
}

PixelOutput main(VertexToPixel input)
{
	PixelOutput output;

	float z = depth_Pass.Sample(defaultSampler, input.myUV).r;
	float4 previousPassColor = materialTexture2.Sample(defaultSampler, input.myUV);
	if(z == 1) {
		previousPassColor.rgb = ColorGrading(previousPassColor.rgb);
		output.myColor = previousPassColor + float4(globalAmbientLightColor.rgb, 1.0f);
		return output;
		}
		
	float near = renderCamera.nearPlane;
	float far = renderCamera.farPlane;

	float worldDepth = -(near * far) / (z * far - near * z - far);
	float remapped = saturate(min(1, (worldDepth - pp_FOG_nearDistance) / (pp_FOG_farDistance - pp_FOG_nearDistance)));
	float3 color = lerp(previousPassColor.rgb, (pp_FOG_color.rgb * pp_FOG_exponent) * saturate(0.95f + noise(input.myUV)), remapped);

	color = ColorGrading(color);

	output.myColor.rgb = color;
	output.myColor.a = 1;
	return output;
}