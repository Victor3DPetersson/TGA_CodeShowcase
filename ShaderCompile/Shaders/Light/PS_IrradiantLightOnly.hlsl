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
    output.myColor.a = 1.0f;
    float depth = depth_Pass.Sample(defaultSampler, input.myUV).r;
    if(depth == 1)
    {
        discard;
    }
    float4 albedo_AO = albedo_AO_Pass.Sample(defaultSampler, input.myUV);
    float3 albedo = albedo_AO.rgb;
    float4 worldPositionDStrength = worldPos_detailStrength_Pass.Sample(defaultSampler, input.myUV).rgba;
    float4 emissive = emissive_emissiveStrength_Pass.Sample(defaultSampler, input.myUV);
    if(worldPositionDStrength.a == 10)
    {
        output.myColor = emissive;
        return output;
    }
    float3 worldPosition = worldPositionDStrength.rgb;
    float4 normal_metalness = tangentNormal_metalness_Pass.Sample(defaultSampler, input.myUV);
    float3 normal = (normal_metalness.rgb * 2) - 1;
    float roughness = max(vertexNormal_Roughness_Pass.Sample(defaultSampler, input.myUV).a, 0.0015f);
    float ssao = materialTexture1.Sample(defaultSampler, input.myUV).r;
    float metalness = normal_metalness.a;
    float ao = albedo_AO.a;
    float3 toEye = normalize(renderCameraPosition - worldPosition.xyz);
    float3 specularColor = lerp((float3) 0.04, albedo.rgb, metalness);
    float3 diffuseColor = albedo.rgb;//lerp((float3) 0.00, albedo.rgb, 1 - metalness);
    float NdV = saturate(dot(normal, toEye));
    float a = max(0.001f, roughness * roughness);
    //----------^ FETCHING PIXEL TEXTURE DATA ^---------//

    //Cluster Data
    float viewDepth = mul(renderCamera.toView, float4(worldPosition, 1.0f)).z;
    uint slice = (log10(viewDepth) * CLUSTER_DEPTH / renderCamera.clusterPreNumerator - renderCamera.clusterPreDenominator / renderCamera.clusterPreNumerator);
	unsigned int u = (input.myUV.x * CLUSTER_WIDTH);
	unsigned int v = ((1 - input.myUV.y) * CLUSTER_HEIGTH);
	uint4 clusterData = (uint4)clusterTexture.Load(uint4( u,  v, slice, 0));
    const int tileIndex = (slice * CLUSTER_WIDTH * CLUSTER_HEIGTH) + (v * CLUSTER_WIDTH) + u;


    uint3 uv = 0;
    float3 SHAmbience = 0;
    uint3 gridIndexes = 50;
    float3 gridWeigths = float3(1.f, 0, 0);
    SHGridWorldLocalPositions indexGridPositions;
    SHGridWorldLocalPositions localGridPositions;
    uint numberOfGrids = 0;
    [loop]for (unsigned int grid = clusterData.x + clusterData.y; grid < clusterData.x + clusterData.y + clusterData.z; grid++)
    {
        uv = uint3(tileIndex, grid, 0);
        uint gridIndex = (uint)clusterIndexMap.Load(uv);
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
            localGridPositions.pos[0] = gridPos;
            numberOfGrids = (uint)1;
            break;
        }
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

    // output.myColor.rgb = probeWeigths * 0.5f;
    // output.myColor.a = 1.0f;
    // return output;
    output.myColor.rgb = ambiance;
    output.myColor.a = 1.0f;
    return output;
    }