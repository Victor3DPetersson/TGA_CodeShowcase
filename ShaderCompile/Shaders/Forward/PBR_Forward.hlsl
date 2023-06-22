#include "Lights.hlsli"

PixelOutput main(VertexToPixel input) : SV_TARGET
{
	PixelOutput output;

	float3 toEye = normalize(myCameraPosition - input.myWorldPosition.xyz);
	float4 albedo = PixelShader_Albedo(input).myColor.rgba;
	if(albedo.a < 0.15f)
	{
		discard;
	}
	float3 normal = normalTexture.Sample(defaultSampler, input.myUV).wyz;
    float ambientOcclusion = normal.b;

 	normal = 2.0f * normal - 1.0f;
    normal.z = sqrt(1 - saturate(normal.x * normal.x + normal.y * normal.y));
    normal = normalize(normal);
    
    float3x3 TBN = float3x3(
    normalize(input.myTangent.xyz),
    normalize(input.myBinormal.xyz),
    normalize(input.myNormal.xyz)    
    );
    
    TBN = transpose(TBN);
    float3 pixelNormal = (normalize(mul(TBN, normal)));


	float3 material = float3(normalTexture.Sample(defaultSampler, input.myUV.xy).wy, 0);
	float metalness = PixelShader_Metalness(input).myColor.r;
	float perceptualroughness = PixelShader_PerceptualRoughness(input).myColor.r;
	float3 emissivedata = EmissiveTexture.Sample(defaultSampler, input.myUV).rgb;

	float3 specularcolor = lerp((float3) 0.04, albedo.rgb, metalness);
	float3 diffusecolor = lerp((float3) 0.00,  albedo.rgb, 1 - metalness);

	float3 ambience = EvaluateAmbiance(
		environmentTexture, pixelNormal, input.myNormal.xyz,
		toEye, perceptualroughness, metalness,  albedo.rgb,
		ambientOcclusion, diffusecolor, specularcolor
	);

	float3 directionallight = EvaluateDirectionalLight(
		diffusecolor, specularcolor, pixelNormal, perceptualroughness,
		directionalColor.xyzw, directionalDirection.xyz, toEye
	);

	float3 pointLightInfluence = 0;
	//for(unsigned int i = 0; i < OB_NumPointLights; i++)
	//{
	//	pointLightInfluence += EvaluatePointLight(
	//		diffusecolor, specularcolor, normal, perceptualroughness, 
	//		myPointLights[i], toEye, input.myWorldPosition.xyz
	//		);
	//}
	float3 spotLightInfluence = 0;
	//for(unsigned int i = 0; i < OB_NumSpotLights; i++)
	//{
	//	spotLightInfluence += EvaluateSpotLight(
	//		diffusecolor, specularcolor, normal, perceptualroughness, 
	//		mySpotLights[i], toEye, input.myWorldPosition.xyz
	//	);
	//}
	float3 emissive = albedo.rgb * emissivedata;
	float3 radiance = ambience + directionallight + emissive + pointLightInfluence + spotLightInfluence;

	output.myColor.rgb = radiance.rgb;
	output.myColor.a = 1.0f;
	return output;
}