#include "HelperFunctions.hlsli"
#include "DirectionalLight.hlsli"

float3 EvaluatePointLight(float3 albedoColor, float3 specularColor, float3 normal, float roughness, PointLight pointLight, float3 viewDir, float3 pixelPos)
{
	float3 lightDir = pointLight.myPosition - pixelPos;
	float lightDistance = length(lightDir) / 1000;
	lightDir = normalize(lightDir);
	if(lightDistance > pointLight.myRange/ 100)
	return 0;

	float NdL = saturate(dot(normal, lightDir));
	float lambert = NdL;
	float NdV = saturate(dot(normal, viewDir));
	float3 h = normalize(lightDir + viewDir);
	float NdH = saturate(dot(normal, h));	
	float a = max(0.001f, roughness * roughness);

	float3 cDiff = Diffuse(albedoColor);
	float3 cSpec = Specular(specularColor, h, viewDir, a, NdL, NdV, NdH);

	float linearAttenuation = lightDistance / (pointLight.myRange / 100);
	linearAttenuation = 1.0f - linearAttenuation;
	linearAttenuation = saturate(linearAttenuation);
	float physicalAttenuation = saturate(1.0f / (lightDistance * lightDistance));
	float attenuation = lambert * linearAttenuation * physicalAttenuation;
	return saturate(pointLight.myColor.rgb * pointLight.myColor.a * attenuation * ((cDiff * (1.0 - cSpec) + cSpec) * PI));
}

float3 EvaluateSpotLight(float3 albedoColor, float3 specularColor, float3 normal,
float roughness, SpotLight spotLight, float3 viewDir, float3 pixelPos )
{
	float3 toLight = spotLight.myPosition - pixelPos;
	float lightDistance = length(toLight) / 1000;
	toLight = normalize(toLight);
	//if(lightDistance < (spotLight.myRange / 100))
	//return 0;

	float NdL = saturate(dot(normal, toLight));
	float lambert = NdL;
	float NdV = saturate(dot(normal, viewDir));
	float3 h = normalize(toLight + viewDir);
	float NdH = saturate(dot(normal, h));	
	float a = max(0.001f, roughness * roughness);

	float3 cDiff = Diffuse(albedoColor);
	float3 cSpec = Specular(specularColor, h, viewDir, a, NdL, NdV, NdH);

	float cosOuterAngle = cos(spotLight.myAngle);
	float cosInnerAngle = cos(0);
	float3 lightDirection = spotLight.myDirection;

	float theta = dot(toLight, normalize(-lightDirection));
	float epsilon = cosInnerAngle - cosOuterAngle;
	float intensity = clamp((theta - cosOuterAngle) / epsilon, 0.0f, 1.0f);
	intensity *= intensity;

	float linearAttenuation = lightDistance / (spotLight.myRange / 100);
	linearAttenuation = 1.0f - linearAttenuation;
	linearAttenuation = saturate(linearAttenuation);
	float physicalAttenuation = saturate(1.0f / (lightDistance * lightDistance));
	float attenuation = lambert * linearAttenuation * physicalAttenuation;

	float finalAttenuation = lambert * intensity * attenuation;


	return saturate(spotLight.myColor.rgb * spotLight.myColor.a * lambert * finalAttenuation * ((cDiff * (1.0 - cSpec) + cSpec) * PI));
}