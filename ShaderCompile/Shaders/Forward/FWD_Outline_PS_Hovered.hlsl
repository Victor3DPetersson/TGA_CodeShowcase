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
}