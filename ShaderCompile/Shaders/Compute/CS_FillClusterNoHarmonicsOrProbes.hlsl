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
    float sqDist = 0.0;

    if (aPoint.x < aTile.minPoint.x)
    {
        sqDist += (aTile.minPoint.x - aPoint.x) * (aTile.minPoint.x - aPoint.x);
    }
    if (aPoint.x > aTile.maxPoint.x)
    {
        sqDist += (aPoint.x - aTile.maxPoint.x) * (aPoint.x - aTile.maxPoint.x);
    }

    if (aPoint.y < aTile.minPoint.y)
    {
        sqDist += (aTile.minPoint.y - aPoint.y) * (aTile.minPoint.y - aPoint.y);
    }
    if (aPoint.y > aTile.maxPoint.y)
    {
        sqDist += (aPoint.y - aTile.maxPoint.y) * (aPoint.y - aTile.maxPoint.y);
    }

    if (aPoint.z < aTile.minPoint.z)
    {
        sqDist += (aTile.minPoint.z - aPoint.z) * (aTile.minPoint.z - aPoint.z);
    }
    if (aPoint.z > aTile.maxPoint.z)
    {
        sqDist += (aPoint.z - aTile.maxPoint.z) * (aPoint.z - aTile.maxPoint.z);
    }
    return sqDist;
}

bool testSphereAABB(float aRadius, cluster aTile, float3 aPos)
{   
    float sqDist = sqDistPointAABB(aPos, aTile);
    float radiusSqr = (aRadius) * (aRadius);
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

    float4x4 toView = transpose(renderCamera.toView);
    cluster threadCluster = clusterData[tileIndex];
    for( uint pLight = 0; pLight < gPointLightCount; pLight++)
    {
        float4 lightPos = float4(gPoints[pLight].myPosition.xyz, 1.0f);
        lightPos = mul(lightPos, toView);
        if( testSphereAABB(gPoints[pLight].myRange, threadCluster, lightPos.xyz) ){
            globalIndexList[uint2(tileIndex, pointLightAmount)] = pLight;
            pointLightAmount += 1;
        }
    }
    float4 clusterSphere = GetSphereSquaredFromVolume(threadCluster);
    for(uint sLight = 0; sLight < gSpotLightCount; sLight++)
    {
        float4 lightDir = float4(gSpots[sLight].myDirection.xyz, 0);
        lightDir = mul(lightDir, toView);
        float4 lightPos = float4(gSpots[sLight].myPosition.xyz, 1);
        lightPos = mul(lightPos, toView);
        if(TestConeVsSphere(lightPos.xyz, lightDir.xyz, (gSpots[sLight].myRange), gSpots[sLight].myAngle, clusterSphere))
        {
            globalIndexList[uint2(tileIndex, pointLightAmount + spotLightAmount)] = sLight;
            spotLightAmount += 1;
        }
    }
    //Updating the light grid for each cluster
    lightGrid[uint3(GTid.xy, GTid.z)].rgba = uint4(pointLightAmount, spotLightAmount, shGridAmount,  reflectionProbeAmount).rgba;
}