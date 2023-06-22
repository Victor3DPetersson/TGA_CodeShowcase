#include "Forward_ShaderStructs.hlsli"

#define FLT_EPSILON 1.192092896e-07f
#define nMipOffset 3

PixelOutput PixelShader_Material(VertexToPixel input)
{
	PixelOutput output;
	float3 material;
	output.myColor.rgba = MRESTexture.Sample(defaultSampler, input.myUV.xy);
	return output;
}

PixelOutput PixelShader_Metalness(VertexToPixel input)
{
	PixelOutput output;
	PixelOutput material = PixelShader_Material(input);
	output.myColor.rgb = material.myColor.rrr;
	output.myColor.a = 1.0f;
	return output;
}

PixelOutput PixelShader_PerceptualRoughness(VertexToPixel input)
{
	PixelOutput output;
	PixelOutput material = PixelShader_Material(input);
	output.myColor.rgb = material.myColor.ggg;
	output.myColor.a = 1.0f;
	return output;
}

PixelOutput PixelShader_Emissive(VertexToPixel input)
{
	PixelOutput output;
	PixelOutput material = PixelShader_Material(input);
	output.myColor.rgb = material.myColor.bbb;
	output.myColor.a = 1.0f;
	return output;
}

PixelOutput PixelShader_Normal(VertexToPixel input)
{
	float3 normal = float3(normalTexture.Sample(defaultSampler, input.myUV.xy).wy, 0);

	normal = 2.0f * normal - 1.0f;
	normal.z = sqrt(1 - saturate(normal.x * normal.x + normal.y * normal.y));

	float3x3 TBN = float3x3(normalize(input.myTangent),	normalize(input.myBinormal), normalize(input.myNormal));
	TBN = transpose(TBN);
	float3 pixelNormal = mul(TBN, normal);
	pixelNormal = normalize(pixelNormal);

	PixelOutput output;
	output.myColor.rgb = pixelNormal;
	output.myColor.a = 1.0f;
	return output;
}

PixelOutput PixelShader_AmbientOcclusion(VertexToPixel input)
{
	float ao = normalTexture.Sample(defaultSampler, input.myUV.xy).b;
	PixelOutput output;
	output.myColor.rgb = ao.xxx;
	output.myColor.a = 1.0f;
	return output;
}

PixelOutput PixelShader_Albedo(VertexToPixel input)
{
	PixelOutput output;
	float4 albedo = colorTexture.Sample(defaultSampler, input.myUV.xy).rgba;
	output.myColor.rgba = albedo;
	return output;
}

float bias(float value, float b)
{
	return (b > 0.0) ? pow(abs(value), log(b) / log(0.5)) : 0.0;
}

float gain(float value, float g)
{
	return 0.5 * ((value < 0.5f) ? bias(2.0 * value, 1.0 - g) : (2.0 - bias(2.0 - 2.0 * value, 1.0 -g)));
}

float RoughnessFromPerceptualRoughness(float fPerceptualRoughness)
{
	return fPerceptualRoughness * fPerceptualRoughness;
}

float PerceptualRoughnessFromRoughness(float fRoughness)
{
	return sqrt(max(0.0, fRoughness));
}

float SpecularPowerFromPerceptualRoughness(float fPerceptualRoughness)
{
	float fRoughness = RoughnessFromPerceptualRoughness(fPerceptualRoughness);
	return (2.0 / max(FLT_EPSILON, fRoughness * fRoughness)) - 2.0;
}

float PerceptualRoughnessFromSpecularPower(float fSpecPower)
{
	float fRoughness = sqrt(2.0 / (fSpecPower + 2.0));
	return PerceptualRoughnessFromRoughness(fRoughness);
}

int GetNumMips(TextureCube cubeTex)
{
	int iWidth = 0, iHeght = 0, numMips = 0;
	cubeTex.GetDimensions(0, iWidth, iHeght, numMips);
	return numMips;
}

float BurleyToMip(float fPerceptualRoughness, int nMips, float NdotR)
{
	float fSpecPower = SpecularPowerFromPerceptualRoughness(fPerceptualRoughness);
	fSpecPower /= (4 * max(NdotR, FLT_EPSILON));
	float fScale = PerceptualRoughnessFromSpecularPower(fSpecPower);
	return fScale * (nMips - 1 - nMipOffset);
}

float3 GetSpecularDominantDir(float3 pN, float3 vR, float fRealRoughness)
{
	float fInvRealRough = saturate(1 - fRealRoughness);
	float lerpFactor = fInvRealRough * (sqrt(fInvRealRough) + fRealRoughness);
	return lerp(pN, vR, lerpFactor);
}

float GetReductionInMicrofacets(float perceptualRoughness)
{
	float roughness = RoughnessFromPerceptualRoughness(perceptualRoughness);
	return 1.0 / (roughness * roughness + 1.0);
}

float EmpiricalSpecularAO(float ao, float perceptualRoughness)
{
	float fSmooth = 1 - perceptualRoughness;
	float fSpecAo = gain(ao, 0.5 + max(0.0, fSmooth * 0.04));

	return min(1.0, fSpecAo + lerp(0.0, 0.5, fSmooth * fSmooth * fSmooth * fSmooth));
}

float ApproximateSpecularSelfOcclusion(float3 vR, float3 vertNormalNormalized)
{
	const float fFadeParam = 1.3;
	float rimmask = clamp(1 + fFadeParam * dot(vR, vertNormalNormalized), 0.0, 1.0);
	rimmask *= rimmask;

	return rimmask;
}

float3 EvaluateAmbiance(TextureCube lysBurleyCube, float3 pN, float3 org_normal, float3 to_cam, float perceptualRoughness, float metalness, float3 albedo, float ao, float3 dfcol, float3 spccol)
{
	int numMips = GetNumMips(lysBurleyCube);
	const int nrBrdfMips = numMips - nMipOffset;
	float VdotN = clamp(dot(to_cam, pN), 0.0, 1.0f);
	const float3 vRorg = 2 * pN * VdotN - to_cam;

	float3 vR = GetSpecularDominantDir(pN, vRorg, RoughnessFromPerceptualRoughness(perceptualRoughness));
	float RdotNsat = saturate(dot(pN, vR));

	float l = BurleyToMip(perceptualRoughness, numMips, RdotNsat);

	float3 specRad = lysBurleyCube.SampleLevel(defaultSampler, vR, l).xyz;
	float3 diffRad = lysBurleyCube.SampleLevel(defaultSampler, pN, (float)(nrBrdfMips - 1)).xyz;

	float fT = 1.0 - RdotNsat;
	float fT5 = fT * fT; fT5 = fT5 * fT5 * fT;
	spccol = lerp(spccol, (float3)1.0, fT5);

	float fFade = GetReductionInMicrofacets(perceptualRoughness);
	fFade *= EmpiricalSpecularAO(ao, perceptualRoughness);
	fFade *= ApproximateSpecularSelfOcclusion(vR, org_normal);

	float3 ambientDiffuse = ao * dfcol * diffRad;
	float3 ambientSpecular = fFade * spccol * specRad;
	return ambientSpecular + ambientDiffuse + ambientColor.rgb * ambientColor.a;
}