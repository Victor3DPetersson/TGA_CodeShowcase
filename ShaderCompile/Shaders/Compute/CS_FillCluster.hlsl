#include "../CommonBuffers.hlsli"


bool TestConeVsSphere(float3 aOrigin, float3 aSpotFwd, float aSpotLength, float aSpotAngle, float4 aTestSphere)
{
    const float3 V = aTestSphere.xyz - aOrigin;
    const float  VlenSq = dot(V,V);
    const float  V1len = dot(V, aSpotFwd);
    const float  distanceClosestPoint = cos(aSpotAngle * 0.5f) * sqrt(VlenSq - V1len * V1len) - V1len * sin(aSpotAngle * 0.5f);
    const bool angleCull = distanceClosestPoint > aTestSphere.w;
    const bool frontCull = V1len > aTestSphere.w + aSpotLength;
    const bool backCull = V1len < -aTestSphere.w;
    return !(angleCull || frontCull || backCull);
}

float sqDistPointAABB(float3 aPoint, cluster aTile)
{
    //float sqDist = 0.0;

    // if (aPoint.x < aTile.minPoint.x)
    // {
    //     sqDist += (aTile.minPoint.x - aPoint.x) * (aTile.minPoint.x - aPoint.x);
    // }
    // if (aPoint.x > aTile.maxPoint.x)
    // {
    //     sqDist += (aPoint.x - aTile.maxPoint.x) * (aPoint.x - aTile.maxPoint.x);
    // }

    // if (aPoint.y < aTile.minPoint.y)
    // {
    //     sqDist += (aTile.minPoint.y - aPoint.y) * (aTile.minPoint.y - aPoint.y);
    // }
    // if (aPoint.y > aTile.maxPoint.y)
    // {
    //     sqDist += (aPoint.y - aTile.maxPoint.y) * (aPoint.y - aTile.maxPoint.y);
    // }

    // if (aPoint.z < aTile.minPoint.z)
    // {
    //     sqDist += (aTile.minPoint.z - aPoint.z) * (aTile.minPoint.z - aPoint.z);
    // }
    // if (aPoint.z > aTile.maxPoint.z)
    // {
    //     sqDist += (aPoint.z - aTile.maxPoint.z) * (aPoint.z - aTile.maxPoint.z);
    // }
    float sqrtDist = 0.0;
    for (int i = 0; i < 3; ++i)
    {
        const float v = aPoint[i];
        if (v < aTile.minPoint[i])
        {
            sqrtDist += (aTile.minPoint[i] - v) * (aTile.minPoint[i] - v);
        }
        if (v > aTile.maxPoint[i])
        {
            sqrtDist += (v - aTile.maxPoint[i]) * (v - aTile.maxPoint[i]);
        }
    }
    return sqrtDist;
}

bool testSphereAABB(float aRadius, cluster aTile, float3 aPos)
{   
    float sqDist = sqDistPointAABB(aPos, aTile);
    float radiusSqr = (aRadius) * (aRadius);
    //radiusSqr = 1000.f * 1000.f;
    return sqDist <= radiusSqr;
}
float LengthSqr(float3 aV)
{
    return ((aV.x * aV.x) + (aV.y * aV.y) + (aV.z * aV.z));
}
float4 GetSphereSquaredFromVolume(cluster aTile)
{
    float3 size = (aTile.maxPoint - aTile.minPoint) * 0.5f;
    float3 middle = aTile.minPoint.xyz + size;
    return float4(middle, sqrt(LengthSqr(size)));
}
bool IntersectionAABB3DAABB3D(const float3 aMin, const float3 aMax, const float3 bMin, const float3 bMax)
{
 
    return !(
        (aMin.x >= bMax.x) || (aMax.x <= bMin.x) ||
        (aMin.y >= bMax.y) || (aMax.y <= bMin.y) ||
        (aMin.z >= bMax.z) || (aMax.z <= bMin.z));

}


//Input
StructuredBuffer<cluster> clusterData : register(t8);
StructuredBuffer<PointLight> gPoints : register(t0);
StructuredBuffer<SpotLight> gSpots : register(t2);

//Output
RWTexture3D<uint4> lightGrid : register(u0); // Array containing offset and number of lights in a cluster
RWTexture2D<uint> globalIndexList : register(u1); // List of active lights in the scene

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{ 
    uint tileIndex = (GTid.z * CLUSTER_WIDTH * CLUSTER_HEIGTH) + (GTid.y * CLUSTER_WIDTH) + GTid.x;

    //Local thread variables
    uint pointLightAmount = 0;
    uint spotLightAmount = 0;
    uint shGridAmount = 0;
    uint reflectionProbeAmount = 0;

    float4x4 toView = (renderCamera.toView);
    cluster threadCluster = clusterData[tileIndex];
    for( uint pLight = 0; pLight < gPointLightCount; pLight++)
    {
        float4 lightPos = float4(gPoints[pLight].myPosition.xyz, 1.0f);
        lightPos = mul(toView, lightPos);
        if( testSphereAABB(gPoints[pLight].myRange * 1.15f, threadCluster, lightPos.xyz) ){
            globalIndexList[uint2(tileIndex, pointLightAmount)] = pLight;
            pointLightAmount += 1;
        }
    }
    float4 clusterSphere = GetSphereSquaredFromVolume(threadCluster);
    for(uint sLight = 0; sLight < gSpotLightCount; sLight++)
    {
        float4 lightDir = float4(gSpots[sLight].myDirection.xyz, 0);
        lightDir = mul(toView, lightDir);
        float4 lightPos = float4(gSpots[sLight].myPosition.xyz, 1);
        lightPos = mul(toView, lightPos);
        if(TestConeVsSphere(lightPos.xyz, lightDir.xyz, (gSpots[sLight].myRange), gSpots[sLight].myAngle, clusterSphere))
        {
            globalIndexList[uint2(tileIndex, pointLightAmount + spotLightAmount)] = sLight;
            spotLightAmount += 1;
        }
    }
 
    const float3 clusterSize = threadCluster.minPoint + ((threadCluster.maxPoint - threadCluster.minPoint) * 0.5f);
    const float3 clusterMid = threadCluster.maxPoint - threadCluster.minPoint;
    const float4 clusterSphereWS = float4(mul(renderCamera.fromView, float4(clusterSphere.xyz, 1)).xyz, clusterSphere.w);
    float sortedDistances[16] = {999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f}; 
    uint shGridStartIndex = pointLightAmount + spotLightAmount;
    for(uint shGrid = 0; shGrid < gLevelSHGridAmount; shGrid++)
    {        
        float3 gridScale = levelSHGrids[shGrid].halfSize;
        gridScale -= levelSHGrids[shGrid].spacing * 0.5f;

        float4 clusterSphereGridSpace = clusterSphereWS;
        clusterSphereGridSpace.xyz = mul(levelSHGrids[shGrid].toGrid, float4(clusterSphereGridSpace.xyz, 1)).xyz;
        cluster gridAABB;
        gridAABB.minPoint = float4(-gridScale, 1);
        gridAABB.maxPoint = float4(gridScale, 1);
        if(testSphereAABB(clusterSphereGridSpace.w, gridAABB, clusterSphereGridSpace.xyz))
        {
            float distanceSquaredFromCluster = (clusterSphereGridSpace.x * clusterSphereGridSpace.x + clusterSphereGridSpace.y * clusterSphereGridSpace.y + clusterSphereGridSpace.z * clusterSphereGridSpace.z);
            uint indexToInsert = shGrid;
            shGridAmount++;// = min(shGridAmount + 1, 3);
            for(uint distSort = shGridStartIndex; distSort < shGridStartIndex + shGridAmount; distSort++)
            {
                if(sortedDistances[distSort - shGridStartIndex] > distanceSquaredFromCluster)
                {
                    //Cacheing indices and ranges
                    float tempDistance = sortedDistances[distSort - shGridStartIndex];
                    uint tempIndex = (uint)globalIndexList[uint2(tileIndex, distSort)];// >> 16;
                    //Filling clusterData
                    sortedDistances[distSort - shGridStartIndex] = distanceSquaredFromCluster;
                    //globalIndexList[uint2(tileIndex, distSort)] &= 0xffff;
                    //globalIndexList[uint2(tileIndex, distSort)] |= (indexToInsert << 16);
                    globalIndexList[uint2(tileIndex, distSort)] = (indexToInsert);
                    //filling Temp values for next iteration
                    distanceSquaredFromCluster = tempDistance;
                    indexToInsert = tempIndex;
                }
            }
        }
    }
    float probeSortedDistances[16] = {999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f, 999999999.f}; 
    uint probeStartIndex = pointLightAmount + spotLightAmount + shGridAmount;
    for(uint rProbe = 0; rProbe < gLevelRProbeAmount; rProbe++)
    {
        ReflectionProbe probe = levelReflectionProbes[rProbe];
        float4 probePos = float4(probe.position.xyz, 1.0f);
        probePos = mul(toView, probePos);
        if(testSphereAABB(probe.outerRadius * 1.25, threadCluster, probePos.xyz))
        {
            float distanceFromCluster = length(probePos.xyz - clusterMid);
            uint indexToInsert = rProbe;
            //reflectionProbeAmount = min(reflectionProbeAmount + 1, 3);
            reflectionProbeAmount++;
            for(uint distSort = probeStartIndex; distSort < probeStartIndex + reflectionProbeAmount; distSort++)
            {
                if(probeSortedDistances[distSort - probeStartIndex] > distanceFromCluster)
                {
                    //Cacheing indices and ranges
                    float tempDistance = probeSortedDistances[distSort - probeStartIndex];
                    uint tempIndex = (uint)globalIndexList[uint2(tileIndex, distSort)];// >> 24;
                    //Filling clusterData
                    probeSortedDistances[distSort - probeStartIndex] = distanceFromCluster;
                    //globalIndexList[uint2(tileIndex, distSort)] &= 0xffffff;
                    //globalIndexList[uint2(tileIndex, distSort)] |= (indexToInsert << 24);
                    globalIndexList[uint2(tileIndex, distSort)] = indexToInsert;
                    //filling Temp values for next iteration
                    distanceFromCluster = tempDistance;
                    indexToInsert = tempIndex;
                }
            }
        }
    }


    //Updating the light grid for each cluster
    lightGrid[uint3(GTid.xy, GTid.z)].rgba = uint4(pointLightAmount, spotLightAmount, shGridAmount,  reflectionProbeAmount).rgba;
}