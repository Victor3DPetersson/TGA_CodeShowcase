#include "CommonBuffers.hlsli"

struct GBufferOutput
{
    float4 myWorldPosDetailStrength : SV_Target0;//Position XYZ : DetailNormal Strength W
    float4 myAlbedoAO				: SV_Target1;//Albedo RGB : AO W
    float4 myNormalMetalness		: SV_Target2;//Normal XYZ : Metalness W
    float4 myVertexNormalRoughness  : SV_Target3;//VertexNormal XYZ : Roughness W
    float4 myEmissive           	: SV_Target4;//EMISSIVE RGB : EMISSIVE STRENGTH W
    float myDepth               	: SV_Target5;
};

struct VertexToPixel
{
	float4 myPosition	: SV_POSITION;
	float3 myNormal		: NORMAL;
	float3 myBinormal	: BINORMAL;
	float3 myTangent	: TANGENT;
	float4 myColor		: COLOR;
	float2 myUV			: TEXCOORD0;
	float2 myUV1		: TEXCOORD1;
	float2 myUV2		: TEXCOORD2;
	float2 myUV3		: TEXCOORD3;

	float4 myWorldPosition	: VPOS;
	uint InstanceID		: SV_InstanceID;
};

GBufferOutput main(VertexToPixel input)
{
    GBufferOutput output;
	//Lerping Color between TValue and color
	//Also the sampling values are the default, Albedo, Normal, Material, Emissive

	ModelEffect objectEffectData;
	if(effectData.gBufferPSEffectIndex % 2 == 0)
	{
		objectEffectData = EffectBuffer[input.InstanceID];
	} 
	else
	{
		objectEffectData = effectData;
	}

	float4 worldPosDStrength = 0;
	float4 albedoAO = 1;
	float4 normalMetalness = 0;
	float4 vertexNormalRoughness = 1;
	float4 emissive = 0;
	float depth = 1;

	if(objectEffectData.gBufferPSEffectIndex  == 0 || objectEffectData.gBufferPSEffectIndex  == 1) // No Coloring fo model, just standard lit
	{
    	float3 albedo = materialTexture1.Sample(trilinearWrapSampler, input.myUV).rgb;
		float3 normal = materialTexture2.Sample(trilinearWrapSampler, input.myUV).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;
		float4 material = materialTexture3.Sample(trilinearWrapSampler, input.myUV).rgba;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, input.myUV);
		depth = input.myPosition.z;
	}
	else if(objectEffectData.gBufferPSEffectIndex  == 2 || objectEffectData.gBufferPSEffectIndex  == 3) // Multiplying albedo with Effect Color
	{
    	float3 albedo = materialTexture1.Sample(trilinearWrapSampler, input.myUV).rgb;
		float3 effectColor = objectEffectData.color.rgb;
		albedo = albedo * lerp(1, effectColor, objectEffectData.tValue);

		float3 normal = materialTexture2.Sample(trilinearWrapSampler, input.myUV).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;
		float4 material = materialTexture3.Sample(trilinearWrapSampler, input.myUV).rgba;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, input.myUV);
		depth = input.myPosition.z;
	}
	else if(objectEffectData.gBufferPSEffectIndex == 4 || objectEffectData.gBufferPSEffectIndex  == 5) // Emissive is albedo
	{
		float3 normal = materialTexture2.Sample(trilinearWrapSampler, input.myUV).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;
		float4 material = materialTexture3.Sample(trilinearWrapSampler, input.myUV).rgba;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		emissive = materialTexture4.Sample(trilinearWrapSampler, input.myUV);
		albedoAO = float4(emissive.rgb, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		depth = input.myPosition.z;
	}
	else if(objectEffectData.gBufferPSEffectIndex == 6 || objectEffectData.gBufferPSEffectIndex  == 7) // Transparent CutOut with lerp
	{
		float4 albedo = materialTexture1.Sample(trilinearWrapSampler, input.myUV).rgba;
		if(albedo.a < 0.05f)
		{
			discard;
		}
		float3 effectColor = objectEffectData.color.rgb;
		albedo = float4(albedo.rgb * lerp(1, effectColor, objectEffectData.tValue), albedo.a);

		float3 normal = materialTexture2.Sample(trilinearWrapSampler, input.myUV).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;
		float4 material = materialTexture3.Sample(trilinearWrapSampler, input.myUV).rgba;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo.rgb, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, input.myUV);
		depth = input.myPosition.z;
	}
	if (objectEffectData.gBufferPSEffectIndex == 8 || objectEffectData.gBufferPSEffectIndex == 9) // Effect color coloring Emissive
	{
		float3 albedo = materialTexture1.Sample(trilinearWrapSampler, input.myUV).rgb;
		float3 effectColor = objectEffectData.color.rgb;

		float3 normal = materialTexture2.Sample(trilinearWrapSampler, input.myUV).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f) + 0.5f;
		float4 material = materialTexture3.Sample(trilinearWrapSampler, input.myUV).rgba;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, input.myUV);
		emissive = float4(effectColor.xyz * length(emissive.xyz), emissive.w);
		depth = input.myPosition.z;
	}
	if (objectEffectData.gBufferPSEffectIndex == 10 || objectEffectData.gBufferPSEffectIndex == 11) // Effect character shading
	{
		float4 material = materialTexture3.Sample(trilinearWrapSampler, input.myUV).rgba;
    	float3 albedo = materialTexture1.Sample(trilinearWrapSampler, input.myUV).rgb;
		float3 effectColor = objectEffectData.color.rgb;
		albedo = albedo * lerp(1, lerp(effectColor, 1, material.b), objectEffectData.tValue);

		float3 normal = materialTexture2.Sample(trilinearWrapSampler, input.myUV).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, input.myUV);
		depth = input.myPosition.z;
	}
	if (objectEffectData.gBufferPSEffectIndex == 12 || objectEffectData.gBufferPSEffectIndex == 13) // Panning UV's Vertical
	{
		float2 uv = input.myUV;
		uv.y +=  totalTime * 0.50f;//effectData.tValue
		float3 albedo = materialTexture1.Sample(trilinearWrapSampler, uv).rgb;
		float3 normal = materialTexture2.Sample(trilinearWrapSampler, uv).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;
		float4 material = materialTexture3.Sample(trilinearWrapSampler, uv).rgba;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, uv);
		depth = input.myPosition.z;
	}

	if (objectEffectData.gBufferPSEffectIndex == 14 || objectEffectData.gBufferPSEffectIndex == 15) // Panning UV's Vertical Controlled by Code
	{
		float2 uv = input.myUV;
		uv.y +=  totalTime * effectData.tValue;//
		float3 albedo = materialTexture1.Sample(trilinearWrapSampler, uv).rgb;
		float3 normal = materialTexture2.Sample(trilinearWrapSampler, uv).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;
		float4 material = materialTexture3.Sample(trilinearWrapSampler, uv).rgba;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, uv);
		depth = input.myPosition.z;
	}
	if (objectEffectData.gBufferPSEffectIndex == 16 || objectEffectData.gBufferPSEffectIndex == 17) // Background material where Mesh Ignores Lighting
	{
		float2 uv = input.myUV * 4.0f;
		uv.y +=  totalTime * 0.01f;
		uv.x -=  totalTime * 0.005f;
		worldPosDStrength = float4(input.myWorldPosition.xyz, 10);
		albedoAO = float4(0, 0 , 0, 0);
		normalMetalness = float4(input.myNormal.xyz, 0);
		vertexNormalRoughness = float4(input.myNormal.xyz, 0);
		emissive = materialTexture4.Sample(trilinearWrapSampler, uv);
		emissive = emissive * emissive + emissive * emissive;
		emissive += emissive;
		depth = input.myPosition.z;
	}
		if (objectEffectData.gBufferPSEffectIndex == 140 || objectEffectData.gBufferPSEffectIndex == 141) // Mesh Particle Standard
	{
		float4 material = materialTexture3.Sample(trilinearWrapSampler, input.myUV).rgba;
    	float3 albedo = materialTexture1.Sample(trilinearWrapSampler, input.myUV).rgb;
		float4 effectColor = objectEffectData.color.rgba;
		albedo = albedo * effectColor.rgb * effectColor.a;

		float3 normal = materialTexture2.Sample(trilinearWrapSampler, input.myUV).wyz;
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
		float3 pixelNormal = (normalize(mul(TBN, normal)) * 0.5f ) + 0.5f;

		worldPosDStrength = float4(input.myWorldPosition.xyz, material.a);
		albedoAO = float4(albedo, ambientOcclusion);
		normalMetalness = float4(pixelNormal, material.r);
		vertexNormalRoughness = float4(input.myNormal.xyz, material.g);
		emissive = materialTexture4.Sample(trilinearWrapSampler, input.myUV) * effectColor.a;
		depth = input.myPosition.z;
	}
	//Add more effects here if you wanna use them, I might add the effect data to the materials

	output.myWorldPosDetailStrength = worldPosDStrength;
	output.myEmissive = emissive;
	output.myAlbedoAO = albedoAO;
	output.myNormalMetalness = normalMetalness;
	output.myVertexNormalRoughness = vertexNormalRoughness;
	output.myDepth = depth;
	return output;
}