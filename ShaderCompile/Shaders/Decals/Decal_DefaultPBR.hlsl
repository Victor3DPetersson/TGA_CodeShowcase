#include "../CommonBuffers.hlsli"

struct VertexToPixel
{
	float4 position	: SV_POSITION;
	float3 normal		: NORMAL;
	float3 binormal	: BINORMAL;
	float3 tangent	: TANGENT;
	float4 color		: COLOR;
	float2 UV			: TEXCOORD0;
	float2 UV1		: TEXCOORD1;
	float2 UV2		: TEXCOORD2;
	float2 UV3		: TEXCOORD3;
	float4 worldPosition	: VPOS;
};

struct DecalOutputs
{
	float4 albedo_ao        : SV_Target0;
    float4 tangentNormal_metalness : SV_Target1;
    float4 vertexNormal_roughness         : SV_Target2;
    float4 emissiveStrength         : SV_Target3;
};
Texture2D StaticDepthTexture : register(t26);

DecalOutputs main(VertexToPixel input) : SV_TARGET
{
	DecalOutputs output;
	// output.albedo_roughness = float4(1, 0, 0, 1);
	// output.tangentNormal_metalness = float4(0, 0, 0, 0);
	// output.emissiveStrength = float4(0, 0, 0, 0);
	// return output;
	float2 NormalizedPixelPos = float2(input.position.x / frameRenderResolutionX, input.position.y / frameRenderResolutionY);
	//NormalizedPixelPos.y = 1 - NormalizedPixelPos.y;
	float worldZ = StaticDepthTexture.Sample(pointSampler, NormalizedPixelPos).r;
	if(worldZ == 1) discard;
	float3 pixelWorldPos = worldPos_detailStrength_Pass.Sample(defaultSampler, NormalizedPixelPos).rgb;
	float3 pixelVertexNormal = vertexNormal_Roughness_Pass.Sample(defaultSampler, NormalizedPixelPos).rgb;
	float3 boxViewDir = normalize(float3(decalFromOB_toWorld._m10, decalFromOB_toWorld._m11, decalFromOB_toWorld._m12));
	float diff = dot(boxViewDir, pixelVertexNormal);
	if(diff < 0.05f) discard;
	float sX = length(float3(decalFromOB_toWorld._m00, decalFromOB_toWorld._m01, decalFromOB_toWorld._m02)) * 100;
	float sY = length(float3(decalFromOB_toWorld._m10, decalFromOB_toWorld._m11, decalFromOB_toWorld._m12)) * 100;
	float sZ = length(float3(decalFromOB_toWorld._m20, decalFromOB_toWorld._m21, decalFromOB_toWorld._m22)) * 100;

	float4 boxPixel_DecalSpacePos = mul(decalFromWorld_toOB, float4(pixelWorldPos, 1));
	clip(0.5f * abs(sX) - abs(boxPixel_DecalSpacePos.x));
	clip(0.5f * abs(sY) - abs(boxPixel_DecalSpacePos.y));
	clip(0.5f * abs(sZ) - abs(boxPixel_DecalSpacePos.z));

	float2 texCoord = ((boxPixel_DecalSpacePos.xz / float2(abs(sX), abs(sZ))) + 0.5f );

	// float3 decalTangentNormal = normalTexture.Sample(defaultSampler, texCoord).wyz;
    // float ambientOcclusion = decalTangentNormal.b;
    
    // decalTangentNormal = 2.0f * decalTangentNormal - 1.0f;
    // decalTangentNormal.z = sqrt(1 - saturate(decalTangentNormal.x * decalTangentNormal.x + decalTangentNormal.y * decalTangentNormal.y));
    // decalTangentNormal = normalize(decalTangentNormal);


	// //Getting the normals of the SampledPixel
	// float3 ddxWp = ddx(pixelWorldPos);
	// float3 ddyWp = ddy(pixelWorldPos);
	// float3 normal = normalize(cross(ddyWp, ddxWp));

	// float3 binormal = normalize(ddxWp);
	// float3 tangent = normalize(ddyWp);

	// float3x3 tangentToView;
	// tangentToView[0] = mul(tangent, renderCamera.toView);
	// tangentToView[1] = mul(binormal, renderCamera.toView);
	// tangentToView[2] = mul(normal, renderCamera.toView);

	// float3 pixelNormal = (normalize(mul(tangentToView, decalTangentNormal)) * 0.5f ) + 0.5f;
	// normal = mul(normal, tangentToView);

	// decalTangentNormal = mul(decalTangentNormal, normal);

	float4 color = materialTexture1.Sample(defaultSampler, texCoord);
	if(color.a < 0.1f){discard;}
	// //float4 normal = normalTexture.Sample(defaultSampler, texCoord).rgb;
	float3 MRES = materialTexture3.Sample(defaultSampler, texCoord);
	float4 Emissive = materialTexture4.Sample(defaultSampler, texCoord) ;

	//float3 viewRay = myCameraPosition - 
	output.albedo_ao = (color.rgba);
	output.vertexNormal_roughness = float4(0, 0, 0, MRES.g);
	output.tangentNormal_metalness = float4(0, 0, 0, MRES.r);
	output.emissiveStrength = Emissive.rgba;
	return output;
}