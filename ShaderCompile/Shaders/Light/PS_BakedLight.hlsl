#include "PBRFunctions_Lights.hlsli"

struct VertexToPixel
{
    float4 myPosition : SV_POSITION;
    float2 myUV : UV;
};
struct PixelOutput
{
    float4 myColor : SV_TARGET;
};

PixelOutput main(VertexToPixel input)
{
    PixelOutput output;
    output.myColor = 0;
    float depth = depth_Pass.Sample(defaultSampler, input.myUV).r;
    if(depth == 1)
    {
        output.myColor = 0;
        return output;
    }
    float4 worldPositionDStrength = worldPos_detailStrength_Pass.Sample(defaultSampler, input.myUV).rgba;
    float4 emissive = emissive_emissiveStrength_Pass.Sample(defaultSampler, input.myUV);
    if(worldPositionDStrength.a == 10)
    {
        output.myColor = emissive;
        return output;
    }
    float3 worldPosition = worldPositionDStrength.rgb;
    float4 albedo_AO = albedo_AO_Pass.Sample(defaultSampler, input.myUV);
    float3 albedo = albedo_AO.rgb;
    float4 normal_metalness = tangentNormal_metalness_Pass.Sample(defaultSampler, input.myUV);
    float3 normal = (normal_metalness.rgb * 2) - 1;
    float4 vertexNormal_Roughness = vertexNormal_Roughness_Pass.Sample(defaultSampler, input.myUV);
    float3 vertexNormal = vertexNormal_Roughness.rgb;
    float ssao = materialTexture1.Sample(defaultSampler, input.myUV).r;
    
    // output.myColor.rgb = float3(ssao, 0, 0);
    // output.myColor.a = 1.0f;
    // return output;
    float metalness = normal_metalness.a;
    float roughness = vertexNormal_Roughness.a;
    float ao = albedo_AO.a;
    
    
    float3 toEye = normalize(renderCameraPosition - worldPosition.xyz);
    
    float3 specularColor = lerp((float3) 0.04, albedo.rgb, metalness);

    float3 diffuseColor = lerp((float3) 0.00, albedo.rgb, 1 - metalness);

    float NdV = saturate(dot(normal, toEye));
    float a = max(0.001f, roughness * roughness);
    float3 ambiance = EvaluateAmbiance(
    environmentTexture,
    normal,
    vertexNormal,
    toEye,
    roughness,
    ao,
    diffuseColor,
    specularColor
    ) * ssao;
    
    float3 directionalLight = EvaluateDirectionalLight(
    diffuseColor,
    specularColor,
    normal,     
    roughness,
    directionalLightColor,
    directionalLightDirection.rgb,
    toEye.xyz,
    NdV, 
    a
    );

    float4 worldToLightView = mul(shadowCameras[0].toView, float4(worldPosition, 1));
    float4 lightViewToLightProj = mul(shadowCameras[0].projection, worldToLightView);

    float2 projectedTexCoord;
    projectedTexCoord.x = ((lightViewToLightProj.x / lightViewToLightProj.w) * 0.50f + 0.5f);
    projectedTexCoord.y = ((lightViewToLightProj.y / lightViewToLightProj.w * -1.0f) * 0.50f + 0.5f);

    if(saturate(projectedTexCoord.x) == projectedTexCoord.x && saturate(projectedTexCoord.y) == projectedTexCoord.y)
    {
        float shadow = 1.f;
        float textureSize =  DIRECTIONALLIGHT_RESOLUTION;
        float depthNormalizedTextureSize = textureSize / SHADOWMAP_size;
        projectedTexCoord *= depthNormalizedTextureSize;

        const float tileSizeNormalized = (float)SHADOWMAP_tileSize / (float)SHADOWMAP_size;
        float3 toLight = directionalLightDirection.rgb - worldPosition;
        toLight = normalize(toLight);    
        float NdL = saturate(dot(normal, toLight));

        const float shadowBias =  max(DIRECTIONALLIGHT_SIZE * 20 * (1.0f - NdL), DIRECTIONALLIGHT_SIZE * 20);
        float viewDepth = (lightViewToLightProj.z / lightViewToLightProj.w) - shadowBias;
        float sampleDepth = shadowAtlasMap.Sample(pointSampler, projectedTexCoord).r;

        if(sampleDepth <= viewDepth)
        {
            shadow = CalcSoftShadowNoVariedPCFKernel(projectedTexCoord, shadowBias, NdL, lightViewToLightProj.xyz / lightViewToLightProj.w, 1.0f / SHADOWMAP_size, 
             float2(0, 0),//MinUV
             float2(depthNormalizedTextureSize, depthNormalizedTextureSize),//MaxUV
            DIRECTIONALLIGHT_SIZE, worldToLightView.z, DIRECTIONALLIGHT_RESOLUTION, input.myUV);
            directionalLight *= shadow;
        }  
    }     
    
    float3 spotLightInfluence = 0;
    float3 pointLighInfluence = 0;

    //float worldDepth = worldDepthFromCameraZDepth(renderCamera.nearPlane, renderCamera.farPlane, depth);
    float viewDepth = mul(renderCamera.toView, float4(worldPosition, 1.0f)).z;
    uint slice = (log10(viewDepth) * CLUSTER_DEPTH / renderCamera.clusterPreNumerator - renderCamera.clusterPreDenominator / renderCamera.clusterPreNumerator);
	unsigned int u = (input.myUV.x * CLUSTER_WIDTH);
	unsigned int v = ((1 - input.myUV.y) * CLUSTER_HEIGTH);
	uint4 clusterData = clusterTexture.Load(uint4( u,  v, slice, 0));
    int3 uv = 0;
    int tileIndex = (slice * CLUSTER_WIDTH * CLUSTER_HEIGTH) + (v * CLUSTER_WIDTH) + u;
    [loop]for(int pL = 0; pL < (int)clusterData.r; pL++)
    {
        uv = int3(tileIndex, pL, 0);
        uint pointLightIndex = (uint)clusterIndexMap.Load(uv);
        //pointLightIndex &= 0xffff;
        pointLighInfluence += EvaluatePointLight(albedo.rgb, specularColor, normal, roughness, (uint)pointLightIndex, NdV, a, toEye, worldPosition, input.myUV);
    }
    [loop]for(int sL = 0; sL < (int)clusterData.g; sL++)
    {
        uv = int3(tileIndex, (clusterData.r + sL), 0);
        uint spotLightIndex = (uint)clusterIndexMap.Load(uv);
        //spotLightIndex &= 0xffff;
        spotLightInfluence += EvaluateSpotLight(albedo.rgb, specularColor, normal, NdV, a, roughness, (uint)spotLightIndex, toEye, worldPosition, input.myUV);
    }
    output.myColor.rgb = lerp((ambiance + directionalLight) + spotLightInfluence + pointLighInfluence, emissive.rgb * (emissive.a * 40.f), (emissive.a));
    output.myColor.a = 1.0f;
    //output.myColor.rgb = float3(ssao, 0, 0);
    return output;
    }