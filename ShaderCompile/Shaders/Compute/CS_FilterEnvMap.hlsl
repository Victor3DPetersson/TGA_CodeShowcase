static const float PI = 3.14159265f;

#define THREADS 8
#define NUM_SAMPLES 64u

// Taken from https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/genbrdflut.frag
// Based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float2 hammersley(uint i, uint N)
{
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return float2(float(i) /float(N), rdi);
}

// Based on Karis 2014
float3 importanceSampleGGX(float2 Xi, float roughness, float3 N)
{
    float a = roughness * roughness;
    // Sample in spherical coordinates
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    // Construct tangent space vector
    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    // Tangent to world space
    float3 upVector = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangentX = normalize(cross(upVector, N));
    float3 tangentY = cross(N, tangentX);
    return tangentX * H.x + tangentY * H.y + N * H.z;
}

// u_params.x == roughness
// u_params.y == mipLevel
// u_params.z == imageSize
cbuffer FilterData : register(b3)
{
    float4 u_params;
};
TextureCube inputTexture : register (t8);
RWTexture2DArray<float4> outputTexture : register (u0);
SamplerState defaultSampler         : register(s0);

float D_GGX(float linearRoughness, float NoH) {
    // Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
    float oneMinusNoHSquared = 1.0 - NoH * NoH;

    float a = NoH * linearRoughness;
    float k = linearRoughness / (oneMinusNoHSquared + a * a);
    float d = k * k * (1.0 / PI);
    return saturate(d);
}

// From Karis, 2014
float3 prefilterEnvMap(float roughness, float3 R, float imgSize)
{
    // Isotropic approximation: we lose stretchy reflections :(
    float3 N = R;
    float3 V = R;

    float3 prefilteredColor = 0;
    float totalWeight = 0.0;
    for (uint i = 0u; i < NUM_SAMPLES; i++) {
        float2 Xi = hammersley(i, NUM_SAMPLES);
        float3 H = importanceSampleGGX(Xi, roughness, N);
        float VoH = dot(V, H);
        float NoH = VoH; // Since N = V in our approximation
        // Use microfacet normal H to find L
        float3 L = 2.0 * VoH * H - V;
        float NoL = saturate(dot(N, L));
        // Clamp 0 <= NoH <= 1
        NoH = saturate(NoH);

        if (NoL > 0.0) {
            // Based off https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
            // Typically you'd have the following:
            // float pdf = D_GGX(NoH, roughness) * NoH / (4.0 * VoH);
            // but since V = N => VoH == NoH
            float pdf = D_GGX(NoH, roughness) / 4.0 + 0.001;
            // Solid angle of current sample -- bigger for less likely samples
            float omegaS = 1.0 / (float(NUM_SAMPLES) * pdf);
            // Solid angle of texel
            float omegaP = 4.0 * PI / (6.0 * imgSize * imgSize);
            // Mip level is determined by the ratio of our sample's solid angle to a texel's solid angle
            float mipLevel = max(0.5 * log2(omegaS / omegaP), 0.0);
            prefilteredColor += inputTexture.SampleLevel(defaultSampler, L, mipLevel).rgb * NoL;
            totalWeight += NoL;
        }
    }
    return prefilteredColor / totalWeight;
}

float3 toWorldCoords( int3 aGlobalId, float mipImageSize)
{
    float3 direction = 0;
    float mipImageHalfSize = (u_params.z * 0.5f);
    float2 uvCoord = 0;
    uvCoord.x =  aGlobalId.x - mipImageHalfSize;
    uvCoord.y =  aGlobalId.y - mipImageHalfSize;
    if(aGlobalId.z == 0)
    {
        direction.x = mipImageHalfSize;
        direction.y = -uvCoord.y;
        direction.z = -uvCoord.x;
    }
    else if(aGlobalId.z == 1)
    {
        direction.x = -mipImageHalfSize;
        direction.y = -uvCoord.y;
        direction.z = uvCoord.x;
    }//Y UP
     else if(aGlobalId.z == 2)
    {
        direction.y = mipImageHalfSize;
        direction.x = uvCoord.x;
        direction.z = uvCoord.y;
    }//Y DOWN
     else if(aGlobalId.z == 3)
    {
        direction.y = -mipImageHalfSize;
        direction.x = uvCoord.x;
        direction.z = -uvCoord.y;
    }//Z 
     else if(aGlobalId.z == 4)
    {
        direction.z = mipImageHalfSize;
        direction.x = uvCoord.x;
        direction.y = -uvCoord.y;
    }
    else
    {
        direction.z = -mipImageHalfSize;
        direction.x = -uvCoord.x;
        direction.y = -uvCoord.y;
    }
    return normalize(direction);
}

// Notice the 6 in the Z component of our NUM_THREADS call
// This allows us to index the faces using gl_GlobalInvocationID.z
[numthreads(THREADS, THREADS, 6)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float mipLevel = u_params.y;
    float imgSize = u_params.z;
    float mipImageSize = u_params.z;
    int3 globalId = int3(DTid.xyz);

    if (globalId.x >= mipImageSize || globalId.y >= mipImageSize)
    {
        return;
    }

    float3 R = normalize(toWorldCoords(globalId, mipImageSize));

    // Don't need to integrate for roughness == 0, since it's a perfect reflector
    if (u_params.x == 0.0) {
        float3 color = inputTexture.SampleLevel(defaultSampler, R, 0);
        outputTexture[globalId] = float4(color, 1.0);
        return;
    }

    float3 color = prefilterEnvMap(u_params.x, R, imgSize);
    // We access our target cubemap as a 2D texture array, where z is the face index
    outputTexture[globalId.xyz] = float4(color, 1.0);
}