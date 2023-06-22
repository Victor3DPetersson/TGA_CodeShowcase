#include "../CommonBuffers.hlsli"
float3 lineIntersectionToZPlane(float3 A, float3 B, float zDistance) {
	//Because this is a Z based normal this is fixed
	float3 normal = float3(0.0, 0.0, 1.0);

	float3 ab = B - A;

	//Computing the intersection length for the line and the plane
	float t = (zDistance - dot(normal, A)) / dot(normal, ab);

	//Computing the actual xyz position of the point along the line
	float3 result = A + t * ab;

	return result;
}
float4 screen2View(float4 screen){
    //Convert to NDC
    float2 texCoord = screen.xy / float2((float)frameRenderResolutionX, (float)frameRenderResolutionY).xy;

    //Convert to clipSpace
    float4 clip = float4(float2(texCoord.xy)* 2.0 - 1.0, screen.z, screen.w);

    //View space transform
    float4 view = mul(renderCamera.invProjection, clip);

    //Perspective projection
    view = view / view.w;

    return view;

    
}

//Output

RWStructuredBuffer<cluster> clusterData : register(u0);        // A linear list of AABB's with size = numclusters

//Each cluster has it's own thread ID in x, y and z
//We dispatch 16x9x24 threads, one thread per cluster
[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupID, uint groupIndex : SV_GroupIndex){
    //Eye position is zero in view space
    const float3 eyePos = 0.0f;

    //Per cluster variables
    float tileNear  = renderCamera.nearPlane * pow(renderCamera.farPlane/ renderCamera.nearPlane, (float)GTid.z / float(CLUSTER_DEPTH));
    float tileFar   = renderCamera.nearPlane * pow(renderCamera.farPlane/ renderCamera.nearPlane, ((float)GTid.z + 1) / float(CLUSTER_DEPTH));
    //Calculating the min and max point in screen space
    float4 maxPoint_sS = float4((GTid.x + 1) * ((float)frameRenderResolutionX / (float)CLUSTER_WIDTH), (GTid.y + 1) * ((float)frameRenderResolutionY / (float)CLUSTER_HEIGTH), 0, 1.0); // Top Right
    float4 minPoint_sS = float4(GTid.x * ((float)frameRenderResolutionX / (float)CLUSTER_WIDTH), GTid.y * ((float)frameRenderResolutionY / (float)CLUSTER_HEIGTH), 0, 1.0); // Bottom left

    //Pass min and max to view space
    float3 maxPoint_vS = screen2View(maxPoint_sS).xyz;
    float3 minPoint_vS = screen2View(minPoint_sS).xyz;

    float3 minPointNear = lineIntersectionToZPlane(eyePos, minPoint_vS, tileNear );
    float3 minPointFar  = lineIntersectionToZPlane(eyePos, minPoint_vS, tileFar );
    float3 maxPointNear = lineIntersectionToZPlane(eyePos, maxPoint_vS, tileNear );
    float3 maxPointFar  = lineIntersectionToZPlane(eyePos, maxPoint_vS, tileFar );

    float3 minPointAABB = min(min(minPointNear, minPointFar),min(maxPointNear, maxPointFar));
    float3 maxPointAABB = max(max(minPointNear, minPointFar),max(maxPointNear, maxPointFar));

    // minPointAABB.x -= abs(minPointAABB.x * 0.25f);
    // maxPointAABB.x += maxPointAABB.x * 0.25f;
    // minPointAABB.y -= abs(minPointAABB.y * 0.25f);
    // maxPointAABB.y += maxPointAABB.y * 0.25f;
    // minPointAABB.z -= tileFar * 0.25f;
    // maxPointAABB.z += tileFar * 0.25f;
    //Saving the AABB at the tile linear index
    uint tileIndex = (GTid.z * CLUSTER_WIDTH * CLUSTER_HEIGTH) + (GTid.y * CLUSTER_WIDTH) + GTid.x;
    clusterData[tileIndex].minPoint  = float4(minPointAABB , 0.0);
    clusterData[tileIndex].maxPoint  = float4(maxPointAABB , 0.0);
}
