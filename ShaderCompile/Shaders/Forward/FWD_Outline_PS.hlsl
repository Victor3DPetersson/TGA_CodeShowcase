#include "../CommonBuffers.hlsli"

struct VertexToPixel
{
	float4 myPosition	: SV_POSITION;
	float4 myColor		: COLOR;
	uint InstanceID		: SV_InstanceID;
};

struct PixelOutput
{
	float4 myColor : SV_TARGET;
};

PixelOutput main(VertexToPixel input) : SV_TARGET
{
	PixelOutput output;

	if(effectData.gBufferPSEffectIndex == 0)
	{
		output.myColor = EffectBuffer[input.InstanceID].color;
	}
	else if(effectData.gBufferPSEffectIndex == 1)
	{
		output.myColor = effectData.color;
	}
	else if (effectData.gBufferPSEffectIndex == 8)
	{
		output.myColor = EffectBuffer[input.InstanceID].outlineColor;
	}
	else if (effectData.gBufferPSEffectIndex == 9)
	{
		output.myColor = effectData.outlineColor;
	}
	output.myColor.a = 1;
	return output;

	float3 inputPos = input.myPosition.xyz;
	float2 pixelScreenPos = float2(inputPos.x / frameRenderResolutionX, inputPos.y / frameRenderResolutionY);
	float zDepth =  depth_Pass.Sample(defaultSampler, pixelScreenPos.xy).r;
	float4 color;
	if(effectData.gBufferPSEffectIndex == 0)
	{
		color = EffectBuffer[input.InstanceID].color;
	}
	else if(effectData.gBufferPSEffectIndex == 1)
	{
		color = effectData.color;
	}
	else if (effectData.gBufferPSEffectIndex == 8)
	{
		output.myColor = EffectBuffer[input.InstanceID].outlineColor;
	}
	else if (effectData.gBufferPSEffectIndex == 9)
	{
		output.myColor = effectData.outlineColor;
	}

	if(zDepth < inputPos.z - 0.002f)
	{
		float worldDepth = -(renderCamera.nearPlane * renderCamera.farPlane) / (inputPos.z * renderCamera.farPlane - renderCamera.nearPlane * inputPos.z - renderCamera.farPlane);
		output.myColor = color * saturate(1 - (worldDepth / 4000));
		return output;
	}
	discard;
	return output;
}