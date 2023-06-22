#include "../CommonBuffers.hlsli"
#define NUMBOF_MAXREFLECTIONPROBES 32
#define NUMBOF_MAXSHGRIDS 32
#define NUMBOF_PERPIXEL 3

struct ReflectionProbeWeigths
{
    float weigths[NUMBOF_MAXREFLECTIONPROBES];
};
struct ReflectionProbeIndices
{
    uint indexes[NUMBOF_MAXREFLECTIONPROBES];
};
float GetInfluenceWeights(ReflectionProbe aProbe, float3 aPixelPos)
{
    float3 SphereCenter            = aProbe.position;
    float3 Direction               = aPixelPos - SphereCenter;
    //const float DistanceSquared    = dot(Direction, Direction);
    return float(length(Direction) - aProbe.innerRadius) / (aProbe.outerRadius - aProbe.innerRadius);
}
    // // Transform from World space to local box (without scaling, so we can test extend box)
    // Vector4 LocalPosition = InfluenceVolume.WorldToLocal(SourcePosition);
    // // Work in the upper left corner of the box.
    // Vector LocalDir = Vector(Abs(LocalPosition.X), Abs(LocalPosition.Y), Abs(LocalPosition.Z));
    // LocalDir = (LocalDir - BoxInnerRange) / (BoxOuterRange - BoxInnerRange);
    // // Take max of all axis
    // NDF = LocalDir.GetMax();
float GetBoxInfluenceWeights(float3 aLocalPosition, float3 aBoxSize, float aGridSpacing)
{
    float3 localDir = float3(abs(aLocalPosition.x), abs(aLocalPosition.y), abs(aLocalPosition.z));    
    float3 boxRange = aBoxSize - aGridSpacing * 0.5f;
    float3 innerRange = boxRange * 0.05;
    localDir = (localDir  - innerRange) / (boxRange - innerRange);
    // Take max of all axis
    return max(localDir.x, max(localDir.y, localDir.z));
}
float3 GetBlendFactor(uint Num, float3 aInfluenceVolume)
{
    // First calc sum of NDF and InvDNF to normalize value
    float3 output;
    float SumNDF            = 0.0f;
    float InvSumNDF         = 0.0f;
    float SumBlendFactor    = 0.0f;

    if(Num == 1)
    {
        return 1 - aInfluenceVolume[0];
    }
    // The algorithm is as follow
    // Primitive have a normalized distance function which is 0 at center and 1 at boundary
    // When blending multiple primitive, we want the following constraint to be respect:
    // A - 100% (full weight) at center of primitive whatever the number of primitive overlapping
    // B - 0% (zero weight) at boundary of primitive whatever the number of primitive overlapping
    // For this we calc two weight and modulate them.
    // Weight0 is calc with NDF and allow to respect constraint B
    // Weight1 is calc with inverse NDF, which is (1 - NDF) and allow to respect constraint A
    // What enforce the constraint is the special case of 0 which once multiply by another value is 0.
    // For Weight 0, the 0 will enforce that boundary is always at 0%, but center will not always be 100%
    // For Weight 1, the 0 will enforce that center is always at 100%, but boundary will not always be 0%
    // Modulate weight0 and weight1 then renormalizing will allow to respects A and B at the same time.
    // The in between is not linear but give a pleasant result.
    // In practice the algorithm fail to avoid popping when leaving inner range of a primitive
    // which is include in at least 2 other primitives.
    // As this is a rare case, we do with it.
    [unroll(NUMBOF_PERPIXEL)]for (int earlyIteration = 0; earlyIteration < Num; ++earlyIteration)
    {
        SumNDF       += aInfluenceVolume[earlyIteration];
        InvSumNDF    += (1.0f - aInfluenceVolume[earlyIteration]);
    }

    // Weight0 = normalized NDF, inverted to have 1 at center, 0 at boundary.
    // And as we invert, we need to divide by Num-1 to stay normalized (else sum is > 1). 
    // respect constraint B.
    // Weight1 = normalized inverted NDF, so we have 1 at center, 0 at boundary
    // and respect constraint A.
    [unroll(NUMBOF_PERPIXEL)]for (int averageWeigth = 0; averageWeigth < Num; ++averageWeigth)
    {
        output[averageWeigth] = (1.0f - (aInfluenceVolume[averageWeigth] / SumNDF)) / (Num - 1);
        output[averageWeigth] *= ((1.0f - aInfluenceVolume[averageWeigth]) / InvSumNDF);
        SumBlendFactor += output[averageWeigth];
      
    }

    // Normalize BlendFactor
    if (SumBlendFactor == 0.0f) // Possible with custom weight
    {
        SumBlendFactor = 1.0f;
    }

    float ConstVal = 1.0f / SumBlendFactor;
    [unroll(NUMBOF_PERPIXEL)]for (int normalizeProbe = 0; normalizeProbe < Num; ++normalizeProbe)
    {
        output[normalizeProbe] *= ConstVal;
    }
    return output;
}
float3 GetBlendMapFactor(int Num, float3 aPixelPos, uint3 aIndexList)
{
    float3 influenceVolume;
    [unroll(NUMBOF_PERPIXEL)]for (int probes = 0; probes < Num; ++probes)
    {
        influenceVolume[probes] = GetInfluenceWeights(levelReflectionProbes[aIndexList[probes]], aPixelPos);
    }
    return GetBlendFactor(Num, influenceVolume);
}
struct SHGridWorldLocalPositions
{
    float3 pos[NUMBOF_PERPIXEL];
};
float3 GetBlendMapFactorBox(int Num, float3 aPixelPos, uint3 aIndexList, SHGridWorldLocalPositions someLocalPositioins)
{
    float3 output;
    float3 influenceVolume;
    [unroll(NUMBOF_PERPIXEL)]for (int grid = 0; grid < Num; ++grid)
    {
        influenceVolume[grid] = GetBoxInfluenceWeights(someLocalPositioins.pos[grid], levelSHGrids[aIndexList[grid]].halfSize, levelSHGrids[aIndexList[grid]].spacing);
    }
    return GetBlendFactor(Num, influenceVolume);
}


//---------------SH MATH FUNCTIONS------------------------------//
float irradmat (float4x4 M, float3 v)
{
    float4 n = { v, 1 };
    return dot(n, mul(M , n));
}

float3 irradcoeffs ( SH aSH, float3 n) 
{
  //------------------------------------------------------------------
  // These are variables to hold x,y,z and squares and products
	float3 L00 = aSH.bands[0].rgb;
	float3 L1_1 = aSH.bands[1].rgb;
	float3 L10 = aSH.bands[2].rgb; 
	float3 L11 = aSH.bands[3].rgb; 
	float3 L2_2 = aSH.bands[4].rgb; 
	float3 L2_1 = aSH.bands[5].rgb; 
	float3 L20 = aSH.bands[6].rgb;
	float3 L21 = aSH.bands[7].rgb;
	float3 L22 = aSH.bands[8].rgb;

	float1 x2 ;
	float1  y2 ;
	float1 z2 ;
	float1 xy ;
	float1  yz ;
	float1  xz ;
	float1 x ;
	float1 y ;
	float1 z ;
	float3 col ;
  //------------------------------------------------------------------       
  // We now define the constants and assign values to x,y, and z 
	
	const float1 c1 = 0.429043 ;
	const float1 c2 = 0.511664 ;
	const float1 c3 = 0.743125 ;
	const float1 c4 = 0.886227 ;
	const float1 c5 = 0.247708 ;
	x = n[0] ; y = n[1] ; z = n[2] ;
  //------------------------------------------------------------------ 
  // We now compute the squares and products needed 

	x2 = x*x ; y2 = y*y ; z2 = z*z ;
	xy = x*y ; yz = y*z ; xz = x*z ;
  //------------------------------------------------------------------ 
  // Finally, we compute equation 13

	col = c1*L22*(x2-y2) + c3*L20*z2 + c4*L00 - c5*L20 
            + 2*c1*(L2_2*xy + L21*xz + L2_1*yz) 
            + 2*c2*(L11*x+L1_1*y+L10*z) ;

	return col ;
}
//using matrixes for the math instead of the constant coefficients
 float4 irrad (float4x4 Mr, float4x4 Mg, float4x4 Mb, float3 worldNormal)
{

     return ( irradmat(Mr, worldNormal), 
 	     irradmat(Mg, worldNormal), 
 	     irradmat(Mb, worldNormal) , 1.0 );
}


void Barycentric(float3 p, float3 a, float3 b, float3 c, out float u, out float v, out float w)
{
    float3 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0f - v - w;
}


//---------------SH GRID FUNCTIONS------------------------------//

float interpolate1D(float v1, float v2, float x){
    return v1*(1-x) + v2*x;
}
float interpolate2D(float v1, float v2, float v3, float v4, float x, float y){

    float s = interpolate1D(v1, v2, x);
    float t = interpolate1D(v3, v4, x);
    return interpolate1D(s, t, y);
}
float interpolate3D(float v1,float v2,float v3,float v4,float v5,float v6,float v7,float v8, float3 aPos)
{
    float s = interpolate2D(v1, v2, v3, v4, aPos.x, aPos.y);
    float t = interpolate2D(v5, v6, v7, v8, aPos.x, aPos.y);
    return interpolate1D(s, t, aPos.z);
}
float3 InterpolatePositionInGrid(float3 aPos, float3 v1, float3 v2, float3 v3, float3 v4, float3 v5, float3 v6, float3 v7, float3 v8)
{
    return float3(
        interpolate3D(v1.x, v2.x, v3.x, v4.x, v5.x, v6.x, v7.x, v8.x, aPos),
        interpolate3D(v1.y, v2.y, v3.y, v4.y, v5.y, v6.y, v7.y, v8.y, aPos),
        interpolate3D(v1.z, v2.z, v3.z, v4.z, v5.z, v6.z, v7.z, v8.z, aPos)
    );
}

float3 SHSum(SH aSH)
{
    return aSH.bands[0] + aSH.bands[1] + aSH.bands[2] + aSH.bands[3] + aSH.bands[4] + aSH.bands[5] + aSH.bands[6];
}

float3 GetSHIrradiance(float3 aGridPos, float3 aNormal, SH_GridData aGrid)
{
    const int w = int((aGrid.halfSize.x * 2) / aGrid.spacing);
	const int h = int((aGrid.halfSize.y * 2) / aGrid.spacing);
	const int d = int((aGrid.halfSize.z * 2) / aGrid.spacing);
    const int currentW = clamp(int((float)aGridPos.x / aGrid.spacing),0, w - 1);
	const int currentH = clamp(int((float)aGridPos.y / aGrid.spacing), 0, h - 1);
	const int currentD = clamp(int((float)aGridPos.z / aGrid.spacing), 0, d - 1);

    float3 SHPos1 = irradcoeffs(SH_globalIrradiance[aGrid.offset + (currentD * w * h) + (currentH * w) + currentW], aNormal * -1);
	//W + 1
	float3 SHPos2 = irradcoeffs(SH_globalIrradiance[aGrid.offset + (currentD * w * h) + (currentH * w) + (currentW + 1)], aNormal * -1);
	//H + 1
	float3 SHPos3 = irradcoeffs(SH_globalIrradiance[aGrid.offset + (currentD * w * h) + ((currentH + 1) * w) + (currentW)], aNormal * -1);
	//H + 1 W + 1 
	float3 SHPos4 = irradcoeffs(SH_globalIrradiance[aGrid.offset + (currentD * w * h) + ((currentH + 1) * w) + (currentW + 1)], aNormal * -1);
	//D + 1
	float3 SHPos5 = irradcoeffs(SH_globalIrradiance[aGrid.offset + ((currentD + 1) * w * h) + ((currentH) * w) + (currentW)], aNormal * -1);
	//D + 1 W + 1
	float3 SHPos6 = irradcoeffs(SH_globalIrradiance[aGrid.offset + ((currentD + 1) * w * h) + ((currentH)*w) + (currentW + 1)], aNormal * -1);
	//D + 1 H + 1
	float3 SHPos7 = irradcoeffs(SH_globalIrradiance[aGrid.offset + ((currentD + 1) * w * h) + ((currentH + 1)*w) + (currentW)], aNormal * -1);
	//D + 1 H + 1 W + 1
    float3 SHPos8 = irradcoeffs(SH_globalIrradiance[aGrid.offset + ((currentD + 1) * w * h) + ((currentH + 1) * w) + (currentW + 1)], aNormal * -1);

	return InterpolatePositionInGrid(((aGridPos /  aGrid.spacing)) - float3(currentW, currentH, currentD), SHPos1, SHPos2, SHPos3, SHPos4, SHPos5, SHPos6, SHPos7, SHPos8);
}