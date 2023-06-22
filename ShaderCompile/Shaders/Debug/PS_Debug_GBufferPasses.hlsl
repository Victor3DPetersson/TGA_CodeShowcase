#include "../Light/PBRFunctions_Lights.hlsli"

struct VertexToPixel
{
    float4 myPosition : SV_POSITION;
    float2 myUV : UV;
};
struct PixelOutput
{
    float4 myColor : SV_TARGET;
};

cbuffer DebugBuffer : register(b9)
{
    float4 debugData;
}

float4 main(VertexToPixel input) : SV_TARGET
{
    float4 output = float4(0, 0, 0, 1);
    float debugVal = debugData.w;
    float depth = depth_Pass.Sample(defaultSampler, input.myUV).r;
    if(depth == 1)
    {
        discard;
    }
    float4 albedo_AO = albedo_AO_Pass.Sample(defaultSampler, input.myUV);
    float3 albedo = albedo_AO.rgb;
    float3 worldPosition = worldPos_detailStrength_Pass.Sample(defaultSampler, input.myUV).rgb;
    float4 normal_metalness = tangentNormal_metalness_Pass.Sample(defaultSampler, input.myUV);
    float3 normal = (normal_metalness.rgb * 2) - 1;
    float roughness = max(vertexNormal_Roughness_Pass.Sample(defaultSampler, input.myUV).a, 0.0015f);
    float ssao = materialTexture1.Sample(defaultSampler, input.myUV).r;
    float metalness = normal_metalness.a;
    float ao = albedo_AO.a;
    float4 emissive = emissive_emissiveStrength_Pass.Sample(defaultSampler, input.myUV);
    float3 toEye = normalize(renderCameraPosition - worldPosition.xyz);
    float3 specularColor = lerp((float3) 0.04, albedo.rgb, metalness);
    float3 diffuseColor = albedo.rgb;//lerp((float3) 0.00, albedo.rgb, 1 - metalness);
    float NdV = saturate(dot(normal, toEye));
    float a = max(0.001f, roughness * roughness);
    //----------^ FETCHING PIXEL TEXTURE DATA ^---------//
    if(debugVal == 0)
    {
        output.rgb = albedo.rgb;
    }
    else if(debugVal == 1)
    {
        output.rgb = normal * 0.5f + 0.5f;
    }
    else if(debugVal == 2)
    {
        output.rgb = vertexNormal_Roughness_Pass.Sample(defaultSampler, input.myUV).rgb * 0.5f + 0.5f;
    }
    else if(debugVal == 3)
    {
        output.r = roughness;
    }
    else if(debugVal == 4)
    {
        output.r = metalness;
    }
    else if(debugVal == 5)
    {
        output.r = ao * ssao;
    }
    else if(debugVal == 6)
    {
        output.rgb = emissive.rgb * emissive.a * 5.0f;
    }
    else if(debugVal == 7)
    {
        output.r = pow(depth, 50.f);
    }
    else if(debugVal == 8)
    {
        albedo = 0.35f;
        diffuseColor = 0.35f;
        //Cluster Data
        float viewDepth = mul(renderCamera.toView, float4(worldPosition, 1.0f)).z;
        uint slice = (log10(viewDepth) * CLUSTER_DEPTH / renderCamera.clusterPreNumerator - renderCamera.clusterPreDenominator / renderCamera.clusterPreNumerator);

        int u = (input.myUV.x * CLUSTER_WIDTH);
        int v = ((1 - input.myUV.y) * CLUSTER_HEIGTH);
        uint4 clusterData = (uint4)clusterTexture.Load(uint4( u,  v, slice, 0));
        int3 uv = 0;
        const int tileIndex = (slice * CLUSTER_WIDTH * CLUSTER_HEIGTH) + (v * CLUSTER_WIDTH) + u;
        float3 SHAmbience = 0;
        float3 finalGridPosition = 0;
        uint3 gridIndexes = 50;
        float3 gridWeigths = 1.f;
        SHGridWorldLocalPositions indexGridPositions;
        SHGridWorldLocalPositions localGridPositions;
        uint numberOfGrids = 0;
        //TODO change this to be based on the cluster of how many Grids affect this pixel
        [loop]for (int grid = clusterData.x + clusterData.y; grid < clusterData.x + clusterData.y + clusterData.z; grid++)
        {
            uv = int3(tileIndex, grid, 0);
            uint gridIndex = (uint)clusterIndexMap.Load(uv);
            //gridIndex >>= 16;
            //gridIndex &= 0xff;
            float3 gridScale = levelSHGrids[gridIndex].halfSize;
            float3 gridPos =  (worldPosition);
            gridPos = mul(levelSHGrids[gridIndex].toGrid, float4(gridPos, 1)).xyz; //Rotate in to the grid's space
            gridScale -= levelSHGrids[gridIndex].spacing * 0.5f;
            if (gridScale.x < gridPos.x) continue;
            if (-gridScale.x > gridPos.x) continue;
            if (gridScale.y < gridPos.y) continue;
            if (-gridScale.y > gridPos.y) continue;
            if (gridScale.z < gridPos.z) continue;
            if (-gridScale.z > gridPos.z) continue;

            float3 innerRange = gridScale * 0.05f;
            if (
            innerRange.x > gridPos.x && -innerRange.x < gridPos.x &&
            innerRange.y > gridPos.y && -innerRange.y < gridPos.y &&
            innerRange.z > gridPos.z && -innerRange.z < gridPos.z)
            {
                gridIndexes[0] = gridIndex;
                gridPos += ((levelSHGrids[gridIndex].halfSize) + levelSHGrids[gridIndex].spacing * 0.5f - levelSHGrids[gridIndex].spacing); //place the pixel in the correct position of the grid space
                localGridPositions.pos[0] = gridPos;
                numberOfGrids = (uint)1;
                break;
            }
            //float currentSize = ((gridScale.x * gridScale.x) + (gridScale.y * gridScale.y) + (gridScale.z * gridScale.z));
            //gridIndex = grid;
            if(numberOfGrids == 0)
            {
                gridIndexes[0] = gridIndex;
                localGridPositions.pos[0] = gridPos;
                gridPos += ((levelSHGrids[gridIndex].halfSize) + levelSHGrids[gridIndex].spacing * 0.5f - levelSHGrids[gridIndex].spacing); //place the pixel in the correct position of the grid space
                indexGridPositions.pos[0] = gridPos;
            }
            else if(numberOfGrids == 1)
            {
                gridIndexes[1] = gridIndex;
                localGridPositions.pos[1] = gridPos;
                gridPos += ((levelSHGrids[gridIndex].halfSize) + levelSHGrids[gridIndex].spacing * 0.5f - levelSHGrids[gridIndex].spacing); //place the pixel in the correct position of the grid space
                indexGridPositions.pos[1] = gridPos;
            }
                else if(numberOfGrids == 2)
            {
                gridIndexes[2] = gridIndex;
                localGridPositions.pos[2] = gridPos;
                gridPos += ((levelSHGrids[gridIndex].halfSize) + levelSHGrids[gridIndex].spacing * 0.5f - levelSHGrids[gridIndex].spacing); //place the pixel in the correct position of the grid space
                indexGridPositions.pos[2] = gridPos;
            }
            numberOfGrids++;
            if(numberOfGrids == NUMBOF_PERPIXEL)
            {
                break;
            }
        }
        if(numberOfGrids > 1)
        {
            gridWeigths = GetBlendMapFactorBox(numberOfGrids, worldPosition, gridIndexes, localGridPositions);
        }
        else if(numberOfGrids == 0){
            SHAmbience = globalAmbientLightColor.rgb * globalAmbientLightColor.a * 0.25f;
        }
        [unroll(NUMBOF_PERPIXEL)]for(uint shGrid = 0; shGrid < numberOfGrids; shGrid++)
        {
            SHAmbience += GetSHIrradiance(indexGridPositions.pos[shGrid], normal, levelSHGrids[gridIndexes[shGrid]]) * gridWeigths[shGrid] * levelSHGrids[gridIndexes[shGrid]].brightness;
        }
        float3 probeWeigths = 0;
        uint3 probeIndices = 0;
        uint numbOfAffectingProbes = 0;
        [loop]for (int probeIndex = clusterData.x + clusterData.y + clusterData.z; probeIndex < clusterData.x + clusterData.y + clusterData.z + clusterData.w; probeIndex++)
        {
            uv = int3(tileIndex, probeIndex, 0);
            uint probeFetchedIndex = (uint)clusterIndexMap.Load(uv);
            //probeFetchedIndex >>= 24;
            ReflectionProbe probe = levelReflectionProbes[probeFetchedIndex];
            float3 SphereCenter            = probe.position;
            float3 Direction               = worldPosition - SphereCenter;
            const float DistanceSquared    = dot(Direction, Direction);
            if (DistanceSquared <= probe.innerRadius * probe.innerRadius)
            {
                probeIndices[0] = probeFetchedIndex;
                probeWeigths[0] = 1.0f;
                numbOfAffectingProbes = (uint)1;
                break;
            }
            if (DistanceSquared <= probe.outerRadius * probe.outerRadius)
            {
                if(numbOfAffectingProbes == 0)
                {
                    probeIndices[0] = probeFetchedIndex;
                }
                else if(numbOfAffectingProbes == 1)
                {
                    probeIndices[1] = probeFetchedIndex;
                }
                else if(numbOfAffectingProbes == 2)
                {
                    probeIndices[2] = probeFetchedIndex;
                }
                numbOfAffectingProbes++;
                if(numbOfAffectingProbes == NUMBOF_PERPIXEL)
                {
                    break;
                }
            }
        }
        if(numbOfAffectingProbes > 1)
        {
            probeWeigths = GetBlendMapFactor(numbOfAffectingProbes, worldPosition, probeIndices);
        }
        else{
            numbOfAffectingProbes = 1;
            probeWeigths[0] = 1.0f;
        }
        float3 ambiance = (EvaluateAmbientPBR(
        normal,
        toEye,
        diffuseColor,
        float3(roughness, ao, metalness), 
        SHAmbience,
        probeIndices,
        probeWeigths,
        numbOfAffectingProbes
        )) * ssao;

        diffuseColor = lerp((float3) 0.00, albedo.rgb, 1 - metalness);
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

            const float shadowBias =  max(DIRECTIONALLIGHT_SIZE * (1.0f - NdL), DIRECTIONALLIGHT_SIZE);
            float viewDepth = (lightViewToLightProj.z / lightViewToLightProj.w) - shadowBias;
            float sampleDepth = shadowAtlasMap.Sample(pointSampler, projectedTexCoord).r;

            if(sampleDepth <= viewDepth)
            {
                shadow = CalcSoftShadow(projectedTexCoord, shadowBias, NdL, lightViewToLightProj.xyz / lightViewToLightProj.w, 1.0f / SHADOWMAP_size, 
                float2(0, 0),//MinUV
                float2(depthNormalizedTextureSize, depthNormalizedTextureSize),//MaxUV
                DIRECTIONALLIGHT_SIZE, worldToLightView.z, DIRECTIONALLIGHT_RESOLUTION, input.myUV);
                directionalLight *= shadow;
            }  
        }      
        float3 spotLightInfluence = 0;
        float3 pointLighInfluence = 0;

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
        output.rgb = (ambiance + directionalLight) * ssao + spotLightInfluence + pointLighInfluence + emissive.rgb * emissive.a * 5.f;
    }

    return output;
    }