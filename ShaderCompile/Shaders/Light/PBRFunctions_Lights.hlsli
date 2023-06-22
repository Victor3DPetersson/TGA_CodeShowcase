#include "PBRFunctions_Probes_SH.hlsli"

#define FLT_EPSILON 1.192092896e-07f
#define nMipOffset 3
static const float PI = 3.14159265f;

float bias(float value, float b)
{
    return (b > 0.0) ? pow(abs(value), log(b) / log(0.5)) : 0.0;
}

float gain(float value, float g)
{
    return 0.5 * ((value < 0.5f) ? bias(2.0 * value, 1.0 - g) : (2.0 - bias(2.0 - 2.0 * value, 1.0 - g)));
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
#define DIELECTRIC_SPECULAR 0.04
float3 EvaluateAmbientPBR(float3 aPixelNormal, float3 aViewDir, float3 aBaseColor, float3 aRoughOccMet, float3 aIrradiance, uint3 someProbeIndexes, float3 someProbeWeigths, uint aNumberOfProbes)
{
    float3 lightDir = reflect(-aViewDir, aPixelNormal);
    float3 H = normalize(lightDir + aViewDir);
    float VoH = saturate(dot(aViewDir, H));
    float NoV = clamp(dot(aPixelNormal, aViewDir), 1e-5, 1.0);


    // From GLTF spec
    float3 diffuseColor = aBaseColor.rgb * (1.0 - DIELECTRIC_SPECULAR) * (1 - aRoughOccMet.b);
    float3 specularColor = lerp((float3)DIELECTRIC_SPECULAR, aBaseColor.xyz, aRoughOccMet.b);

    // Load env textures
    float2 f_ab = PBR_BRDFLut.Sample(defaultSampler, float2(NoV, aRoughOccMet.r)).xy;
    float lodLevel = aRoughOccMet.r * (gNumberOfMipsEnvironment);
    float3 radiance = 0;
    float3 probeIrradiance = 0;
    for(uint rProbe = 0; rProbe < aNumberOfProbes; rProbe++)
    {
        radiance += ReflectionProbesTextures.SampleLevel(defaultSampler, float4(lightDir, (float)someProbeIndexes[rProbe]), lodLevel).xyz * someProbeWeigths[rProbe] * levelReflectionProbes[someProbeIndexes[rProbe]].brightness;
        probeIrradiance += irradcoeffs(levelReflectionProbes[someProbeIndexes[rProbe]].irradianceLight, -aPixelNormal) * someProbeWeigths[rProbe];
    }
    //return probeIrradiance;
    //float3 FssEss = F0 * f_ab.x + f_ab.y;
    //return (FssEss * radiance + diffuseColor) * aIrradiance;
    float3 Fr = max(float3(aRoughOccMet.r, aRoughOccMet.r, aRoughOccMet.r), specularColor) - specularColor;
    float3 k_S = specularColor + Fr * pow((1.0 - NoV), 5.0);
    float3 FssEss = k_S * f_ab.x + f_ab.y;
    // Multiple scattering, from Fdez-Aguera
    float Ems = (1.0 - (f_ab.x + f_ab.y));
    float3 F_avg = specularColor + (1.0 - specularColor) / 21.0;
    float3 FmsEms = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);
    float3 k_D = diffuseColor * (1.0 - FssEss - FmsEms);
    return (FssEss * saturate(radiance / probeIrradiance) + ((FmsEms + k_D) * aRoughOccMet.g)) * aIrradiance;
}

float3 EvaluateAmbiance(TextureCube lysBurleyCube, float3 pN, float3 org_normal, float3 to_cam, float perceptualRoughness, float ao, float3 dfcol, float3 spccol)
{
    int numMips = GetNumMips(lysBurleyCube);
    const int nrBrdfMips = numMips - nMipOffset;
    float VdotN = clamp(dot(to_cam, pN), 0.0, 1.0f);
    const float3 vRorg = 2 * pN * VdotN - to_cam;

    float3 vR = GetSpecularDominantDir(pN, vRorg, RoughnessFromPerceptualRoughness(perceptualRoughness));
    float RdotNsat = saturate(dot(pN, vR));

    float l = BurleyToMip(perceptualRoughness, numMips, RdotNsat);

    float3 specRad = lysBurleyCube.SampleLevel(defaultSampler, vR, l).xyz;
    float3 diffRad = lysBurleyCube.SampleLevel(defaultSampler, pN, (float) (nrBrdfMips - 1)).xyz;

    float fT = 1.0 - RdotNsat;
    float fT5 = fT * fT;
    fT5 = fT5 * fT5 * fT;
    spccol = lerp(spccol, (float3) 1.0, fT5);

    float fFade = GetReductionInMicrofacets(perceptualRoughness);
    fFade *= EmpiricalSpecularAO(ao, perceptualRoughness);
    fFade *= ApproximateSpecularSelfOcclusion(vR, org_normal);

    float3 ambientDiffuse = ao * dfcol * diffRad;
    float3 ambientSpecular = fFade * spccol * specRad;
    return (ambientSpecular + ambientDiffuse + globalAmbientLightColor.rgb) * globalAmbientLightColor.a;
}

float3 EvaluateAmbianceBakedLight(TextureCube lysBurleyCube, float3 pN, float3 org_normal, float3 to_cam, float perceptualRoughness, float ao, float3 dfcol, float3 spccol)
{
    int numMips = GetNumMips(lysBurleyCube);
    const int nrBrdfMips = numMips - nMipOffset;
    float VdotN = clamp(dot(to_cam, pN), 0.0, 1.0f);
    const float3 vRorg = 2 * pN * VdotN - to_cam;

    float3 vR = GetSpecularDominantDir(pN, vRorg, RoughnessFromPerceptualRoughness(perceptualRoughness));
    float RdotNsat = saturate(dot(pN, vR));

    float l = BurleyToMip(perceptualRoughness, numMips, RdotNsat);

    float3 specRad = lysBurleyCube.SampleLevel(defaultSampler, vR, l).xyz;
    float3 diffRad = lysBurleyCube.SampleLevel(defaultSampler, pN, (float) (nrBrdfMips - 1)).xyz;

    float fT = 1.0 - RdotNsat;
    float fT5 = fT * fT;
    fT5 = fT5 * fT5 * fT;
    spccol = lerp(spccol, (float3) 1.0, fT5);

    float fFade = GetReductionInMicrofacets(perceptualRoughness);
    fFade *= EmpiricalSpecularAO(ao, perceptualRoughness);
    fFade *= ApproximateSpecularSelfOcclusion(vR, org_normal);

    float3 ambientDiffuse = ao * dfcol * diffRad;
    float3 ambientSpecular = fFade * spccol * specRad;
    return (ambientSpecular + ambientDiffuse);
}

// float3 EvaluateAmbianceMultiScattering(float3 pN,float3 to_cam, float perceptualRoughness, float ao, float3 dfcol, float aMetalness, float3 aIrradiance, uint3 someProbeIndices, float3 someProbeWeigths, uint aNumberOfProbes)
// {
//       return EvaluatePBR(pN, to_cam, dfcol, float3(perceptualRoughness, ao, aMetalness), aIrradiance, someProbeIndices, someProbeWeigths, aNumberOfProbes);
// }

float3 Fresnel_Schlick(float3 specularColor, float3 h, float3 v)
{
    return (specularColor + (1.0f - specularColor) * pow((1.0f - saturate(dot(v, h))), 5));
}

float3 Specular_F(float3 specularColor, float3 h, float3 v)
{
    return Fresnel_Schlick(specularColor, h, v);
}

float3 Diffuse(float3 pAlbedo)
{
    return pAlbedo / PI;
}

float NormalDistribution_GGX(float a, float NdH)
{
    float a2 = a * a;
    float NdH2 = NdH * NdH;

    float denominator = NdH2 * (a2 - 1.0f) + 1.0f;
    denominator *= denominator;
    denominator *= PI;

    return a2 / denominator;
}

float Specular_D(float a, float NdH)
{
    return NormalDistribution_GGX(a, NdH);
}

float Geometric_Smith_Schlick_GGX(float a, float NdV, float NdL)
{
    float k = a * 0.5f;
    float GV = NdV / (NdV * (1 - k) + k);
    float GL = NdL / (NdL * (1 - k) + k);

    return GV * GL;
}

float Specular_G(float a, float NdV, float NdL)
{
    return Geometric_Smith_Schlick_GGX(a, NdV, NdL);
}

float3 Specular(float3 specularColor, float3 h, float3 v, float a, float NdL, float NdV, float NdH)
{
    return ((Specular_D(a, NdH) * Specular_G(a, NdV, NdL)) * Specular_F(specularColor, v, h)) / (4.0f * NdL * NdV + 0.0001f);
}

float3 EvaluateDirectionalLight(float3 albedoColor, float3 aSpecularColor, float3 normal, float roughness, float4 lightColor, float3 lightDir, float3 viewDir, float NdV, float a)
{
    float NdL = saturate(dot(normal, lightDir));
    float lambert = NdL;
    float3 h = normalize(lightDir + viewDir);
    float NdH = saturate(dot(normal, h));

    float3 cDiff = albedoColor / PI;
    float3 cSpec = Specular(aSpecularColor, h, viewDir, a, NdL, NdV, NdH);
    return saturate(lightColor.rgb * lambert * lightColor.a * (cDiff * (1.0f - cSpec) + cSpec) * PI);
}
#define NUM_SAMPLES 32
#define ShadowCameraNear 1.0f

float2 VogelDiskSample(int sampleIndex, int samplesCount, float phi)
{
  float GoldenAngle = 2.4f;

  float r = sqrt(sampleIndex + 0.5f) / sqrt(samplesCount);
  float theta = (float)sampleIndex * GoldenAngle + phi;

  float sine, cosine;
  sincos(theta, sine, cosine);
  
  return float2(r * cosine, r * sine);
}

float InterleavedGradientNoise(float2 position_screen)
{
  float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
  return frac(magic.z * frac(dot(position_screen, magic.xy)));
}

float AvgBlockersDepthToPenumbra(float z_shadowMapView, float avgBlockersDepth)
{
  float penumbra = (z_shadowMapView - avgBlockersDepth) / avgBlockersDepth;
  penumbra *= penumbra;
  return saturate(80.0f * penumbra);
}

float calcSearchWidth(float receiverDepth, float aLightSize, float aViewZPost)
{
    return aLightSize * (worldDepthFromCameraZDepth(ShadowCameraNear, aLightSize, receiverDepth) - ShadowCameraNear) / aViewZPost;
}
 
float CalcBlockerDistance(float aBias, float3 aPixelposPost, float aTexelSize, float aRandomRotation, float aLightSize, float2 aMinUV, float2 aMaxUV, float aLightTargetDepth)
{
    float sumBlockerDistances = 0.0f;
    int numBlockerDistances = 0;
    float receiverDepth = aPixelposPost.z;
 
    int sw = int(calcSearchWidth(receiverDepth, aLightSize, aLightTargetDepth));
    [unroll(NUM_SAMPLES)]for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float2 offset = float2(
            //  NOISE_PoissonSamples[i].x * NOISE_PoissonSamples[i].y,
            //  NOISE_PoissonSamples[i].x * NOISE_PoissonSamples[i].y);
            VogelDiskSample(i, NUM_SAMPLES, aRandomRotation));
 
        float depth = shadowAtlasMap.Sample(defaultSampler, clamp(aPixelposPost.xy + offset * aTexelSize * sw, aMinUV, aMaxUV)).r;
        if (depth < receiverDepth - aBias)
        {
            ++numBlockerDistances;
            sumBlockerDistances += depth;
        }
    }
    if (numBlockerDistances > 0)
    {
        return sumBlockerDistances / numBlockerDistances;
    }
    else
    {
        return -1;
    }
}
float calcPCFKernelSize(float aBias, float3 aPixelposPost, float aTexelSize, float aRandomRotation, float aLightSize, float2 aMinUV, float2 aMaxUV, float aLightTargetDepth)
{
    float receiverDepth = aPixelposPost.z;
    float blockerDistance = CalcBlockerDistance(aBias, aPixelposPost, aTexelSize, aRandomRotation, aLightSize, aMinUV, aMaxUV, aLightTargetDepth);
    if (blockerDistance == -1)
    {
        return 1;
    }
 
    float penumbraWidth = (receiverDepth - blockerDistance) / blockerDistance;
    return penumbraWidth * aLightSize * ShadowCameraNear / receiverDepth;
}
float CalcSoftShadow(float2 inputUV, float aBias, float aNdL, float3 aPixelposPost, float aTexelSize, float2 aMinUV, float2 aMaxUV, float aLightSize, float aLightTargetDepth, float aAtlasSize, float2 aScreenPos)
{
    float shadow = 0;
    const float2 noiseScale = float2(frameRenderResolutionX / NOISE_RotationalTextureSize, frameRenderResolutionY / NOISE_RotationalTextureSize) * 0.1f;
	float randomRotation = InterleavedGradientNoise(aScreenPos * float2(frameRenderResolutionX , frameRenderResolutionY));
    float pcfKernelSize = ((float)aAtlasSize / (float)SHADOWMAP_tileSize) * calcPCFKernelSize(aBias, float3( inputUV.xy, aPixelposPost.z), aTexelSize, randomRotation, aLightSize, aMinUV, aMaxUV, aLightTargetDepth);
    shadow = 0.0f;
    [unroll(NUM_SAMPLES)]for(int i = 0; i < NUM_SAMPLES; i++)
    {
    float2 sampleUV = VogelDiskSample(i, NUM_SAMPLES, randomRotation * randomRotation);
    sampleUV = inputUV + sampleUV * pcfKernelSize;
    sampleUV = clamp(sampleUV, aMinUV, aMaxUV);
    shadow += shadowAtlasMap.SampleCmp(linearClampComparisonSampler, sampleUV, aPixelposPost.z - aBias).x;
    }
    shadow /= NUM_SAMPLES;

    return smoothstep(0, 0.85f, 1 - pow(saturate(shadow), 4));
}
float CalcSoftShadowNoVariedPCFKernel(float2 inputUV, float aBias, float aNdL, float3 aPixelposPost, float aTexelSize, float2 aMinUV, float2 aMaxUV, float aLightSize, float aLightTargetDepth, float aAtlasSize, float2 aScreenPos)
{
    float shadow = 0;
    const float2 noiseScale = float2(frameRenderResolutionX / NOISE_RotationalTextureSize, frameRenderResolutionY / NOISE_RotationalTextureSize) * 0.1f;
	float randomRotation = InterleavedGradientNoise(aScreenPos * float2(frameRenderResolutionX , frameRenderResolutionY));
    float pcfKernelSize = (float)aAtlasSize / (float)SHADOWMAP_tileSize * aLightSize;
    shadow = 0.0f;
    [unroll(NUM_SAMPLES)]for(int i = 0; i < NUM_SAMPLES; i++)
    {
    float2 sampleUV = VogelDiskSample(i, NUM_SAMPLES, randomRotation * randomRotation);
    sampleUV = inputUV + sampleUV * pcfKernelSize;
    sampleUV = clamp(sampleUV, aMinUV, aMaxUV);
    shadow += shadowAtlasMap.SampleCmp(linearClampComparisonSampler, sampleUV, aPixelposPost.z - aBias).x;
    }
    shadow /= NUM_SAMPLES;

    return smoothstep(0, 0.85f, 1 - pow(saturate(shadow), 4));
}

float EvaluateSpotLightShadowContribution(uint4 someShadowAtlasData, float4 aPixelPos, float aNdL, float aSpotLightSize, float2 aScreenPos)
{
    float shadowAmount = 1;
    float4 worldToLightView = mul(shadowCameras[someShadowAtlasData.w].toView, aPixelPos);
    float4 lightViewToLightProj = mul(shadowCameras[someShadowAtlasData.w].projection, worldToLightView);

    float2 projectedTexCoord;
    projectedTexCoord.x = ((lightViewToLightProj.x / lightViewToLightProj.w) * 0.50f + 0.5f);
    projectedTexCoord.y = ((lightViewToLightProj.y / lightViewToLightProj.w * -1.0f) * 0.50f + 0.5f);

    if(saturate(projectedTexCoord.x) == projectedTexCoord.x && saturate(projectedTexCoord.y) == projectedTexCoord.y)
    {
        float textureSize =  (float)someShadowAtlasData.z;
        float depthNormalizedTextureSize = textureSize / SHADOWMAP_size;
        projectedTexCoord *= depthNormalizedTextureSize;

        const float tileSizeNormalized = (float)SHADOWMAP_tileSize / (float)SHADOWMAP_size;
        projectedTexCoord.x += tileSizeNormalized * someShadowAtlasData.x;
        projectedTexCoord.y += tileSizeNormalized * someShadowAtlasData.y;

        const float shadowBias =  max(0.000005f * (1.0f - aNdL), 0.000005f);
        float viewDepth = (lightViewToLightProj.z / lightViewToLightProj.w) - shadowBias;
        float sampleDepth = shadowAtlasMap.Sample(pointSampler, projectedTexCoord).r;
        if(sampleDepth == 1)
        {
             return shadowAmount;
        }
        if(sampleDepth <= viewDepth)
        {
            shadowAmount = CalcSoftShadow(projectedTexCoord, shadowBias, aNdL, lightViewToLightProj.xyz / lightViewToLightProj.w, 1.0f / SHADOWMAP_size, 
             float2(tileSizeNormalized * someShadowAtlasData.x, tileSizeNormalized * someShadowAtlasData.y),//MinUV
             float2(tileSizeNormalized * someShadowAtlasData.x + depthNormalizedTextureSize, tileSizeNormalized * someShadowAtlasData.y + depthNormalizedTextureSize),//MaxUV
            aSpotLightSize, worldToLightView.z, someShadowAtlasData.z, aScreenPos);
        }        
        //return pow(sampleDepth, 2000);
    }
    //return 0; // commented out debug code
    return shadowAmount;
}

float3 EvaluateSpotLight(float3 albedoColor, float3 specularColor, float3 normal, float NdV, float a,
float roughness, unsigned int aSpotLightIndex, float3 viewDir, float3 pixelPos, float2 aScreenPos)
{
    SpotLight spotLight = clusteredSpotLights[aSpotLightIndex];
    float3 toLight = spotLight.myPosition - pixelPos;
    float lightDistance = length(toLight);
    toLight = normalize(toLight);    

    float NdL = saturate(dot(normal, toLight));
    // uint4 shadowDatas = clusteredSpotLightsShadowData[aSpotLightIndex].shadowAtlasIndex;
    // float shadow = 1.0f;
    // if(shadowDatas.z != 0 /*&& shadowData.w == 0*/)
    // {
    //     shadow *= EvaluateSpotLightShadowContribution(shadowDatas, float4(pixelPos, 1), NdL, spotLight.myRange * 0.1f, aScreenPos);
    //     //return float3(EvaluateSpotLightShadowContribution(shadowData, float4(pixelPos, 1)), 0, 0);
    // }
    // return shadow;
    float lambert = NdL;
    float3 h = normalize(toLight + viewDir);
    float NdH = saturate(dot(normal, h));

    float3 cDiff = Diffuse(albedoColor);
    float3 cSpec = Specular(specularColor, h, viewDir, a, NdL, NdV, NdH);

    float cosOuterAngle = cos(spotLight.myAngle * 0.5f);
    float3 lightDirection = spotLight.myDirection;

    float theta = dot(toLight, normalize(-lightDirection));
    float epsilon = 1 - cosOuterAngle;
    float intensity = clamp((theta - cosOuterAngle) / epsilon, 0.0f, 1.0f);
    intensity *= intensity;

    float linearAttenuation = lightDistance / (spotLight.myRange);
    linearAttenuation = saturate(1.0f - linearAttenuation);
    float physicalAttenuation = saturate(1.0f / ((lightDistance * 0.01f) * (lightDistance * 0.01f)));
    float attenuation = linearAttenuation * physicalAttenuation * lambert;

    uint4 shadowData = clusteredSpotLightsShadowData[aSpotLightIndex].shadowAtlasIndex;
    if(shadowData.z != 0 /*&& shadowData.w == 0*/)
    {
        attenuation *= EvaluateSpotLightShadowContribution(shadowData, float4(pixelPos, 1), NdL, (spotLight.myAngle  / 0.017453f) * spotLight.size * 0.0001f, aScreenPos);
        //return float3(EvaluateSpotLightShadowContribution(shadowData, float4(pixelPos, 1)), 0, 0);
    }
    return saturate(spotLight.myColor.rgb * spotLight.myColor.a * attenuation * ((cDiff * (1.0 - cSpec) + cSpec) * PI)) * intensity * 10;
}

float EvaluatePointLightShadowContribution(uint4 someShadowAtlasData, float4 aPixelPos, unsigned int aPointDirection, float aNdL, float aPointLightSize, float2 aScreenPos)
{
    float shadowAmount = 1;
    float4 worldToLightView = mul(shadowCameras[someShadowAtlasData.w + aPointDirection].toView, aPixelPos);
    float4 lightViewToLightProj = mul(shadowCameras[someShadowAtlasData.w + aPointDirection].projection, worldToLightView);

    float2 projectedTexCoord;
    projectedTexCoord.x = ((lightViewToLightProj.x / lightViewToLightProj.w) * 0.50f + 0.5f);
    projectedTexCoord.y = ((lightViewToLightProj.y / lightViewToLightProj.w * -1.0f) * 0.50f + 0.5f);

    if(saturate(projectedTexCoord.x) == projectedTexCoord.x && saturate(projectedTexCoord.y) == projectedTexCoord.y)
    {
        float textureSize =  (float)someShadowAtlasData.z;
        //float depthTexTileSizeNormalized = (textureSize / SHADOWMAP_size) * 0.999f;
        float2 depthNormalizedTextureSize = (textureSize / SHADOWMAP_size);
        float pointIndexOffset = (aPointDirection * (textureSize / SHADOWMAP_tileSize));
        projectedTexCoord *= depthNormalizedTextureSize;

        float tileSizeNormalized = ((float)SHADOWMAP_tileSize / (float)SHADOWMAP_size);
        projectedTexCoord.x += tileSizeNormalized * (someShadowAtlasData.x + pointIndexOffset);
        projectedTexCoord.y += tileSizeNormalized * someShadowAtlasData.y;

        const float shadowBias = max(0.0005f * (1.0f - aNdL), 0.0005f); 
        float shadow = 0.0f;
        float viewDepth = (lightViewToLightProj.z / lightViewToLightProj.w) - shadowBias;
        float sampleDepth = shadowAtlasMap.Sample(pointSampler, projectedTexCoord).r;
        if(sampleDepth == 1)
        {
              return shadowAmount;
        }
        if(sampleDepth < viewDepth)
        {
            shadowAmount = 0;
            shadowAmount = CalcSoftShadow(projectedTexCoord, shadowBias, aNdL, lightViewToLightProj.xyz / lightViewToLightProj.w, 1.0f / SHADOWMAP_size, 
             float2(tileSizeNormalized * (someShadowAtlasData.x + pointIndexOffset), tileSizeNormalized * someShadowAtlasData.y),//MinUV
             float2(tileSizeNormalized * (someShadowAtlasData.x + pointIndexOffset), tileSizeNormalized * someShadowAtlasData.y) + depthNormalizedTextureSize,//MaxUV
            aPointLightSize, worldToLightView.z, someShadowAtlasData.z, aScreenPos);
        }        
        //return pow(sampleDepth, 200);
    }
    //return 0; // commented out debug code
    return shadowAmount;
}

float3 EvaluatePointLight
    (
    float3 albedoColor, float3 specularColor, float3 normal, float roughness, uint aPointLightIndex,
    float NdV, float a, 
    float3 viewDir, float3 pixelPos, float2 aScreenPos)
{
    PointLight pointLight = clusteredPointLights[aPointLightIndex];
    float3 lightDir = pointLight.myPosition - pixelPos;
    float lightDistance = length(lightDir);
    lightDir = normalize(lightDir);
    float NdL = saturate(dot(normal, lightDir));
    float lambert = NdL;
    float3 h = normalize(lightDir + viewDir);
    float NdH = saturate(dot(normal, h));

    float3 cDiff = Diffuse(albedoColor);
    float3 cSpec = Specular(specularColor, h, viewDir, a, NdL, NdV, NdH);

    float linearAttenuation = lightDistance / (pointLight.myRange);
    linearAttenuation = saturate(1.0f - linearAttenuation);
    float physicalAttenuation = saturate(1.0f / ((lightDistance * 0.01f) * (lightDistance * 0.01f)));
    float attenuation = lambert * linearAttenuation * physicalAttenuation;

    uint4 shadowData = clusteredPointLightsShadowData[aPointLightIndex].shadowAtlasIndex;
    if(shadowData.z != 0 /*&& shadowData.w == 0*/)
    {
        unsigned int pointLightDirection = 0;
        if(abs(lightDir.x) > abs(lightDir.y) && abs(lightDir.x) > abs(lightDir.z))
        {
             if(lightDir.x >= 0)
            {
                pointLightDirection = 1;
            }
        }else if(abs(lightDir.y) > abs(lightDir.x) && abs(lightDir.y) > abs(lightDir.z))
        {
            if(lightDir.y <= 0)
            {
                pointLightDirection = 2;
            }
            else
            {
                pointLightDirection = 3;
            }
        }
        else
        {
            if(lightDir.z <= 0)
            {
                pointLightDirection = 4;
            }
            else
            {
                pointLightDirection = 5;
            }
        }
        attenuation *= EvaluatePointLightShadowContribution(shadowData, float4(pixelPos, 1), pointLightDirection, NdL, pointLight.size * 0.001, aScreenPos);
        //return float3(EvaluateSpotLightShadowContribution(shadowData, float4(pixelPos, 1)), 0, 0);
    }

    return saturate(pointLight.myColor.rgb * pointLight.myColor.a * attenuation * ((cDiff * (1.0 - cSpec) + cSpec) * PI));
}
