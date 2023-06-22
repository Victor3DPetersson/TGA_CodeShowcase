#include "FullScreenStructs.hlsli"
Texture2D fullscreenTextureMid 	: register(t8);
Texture2D fullscreenTextureNear	: register(t9);
Texture2D fullscreenTextureFar	: register(t10);
Texture2D<float4> fullscreenTextureMask	: register(t11);
PixelOutput main(VertexToPixel input)
{
	PixelOutput output;
	float4 val;

	float depth = depth_Pass.Sample(defaultSampler, input.myUV).a;
	float worldDepth = worldDepthFromCameraZDepth(renderCamera.nearPlane, renderCamera.farPlane, depth);
	float4 blend = 0;
	float t = 0;
	float4 midVal = fullscreenTextureMid.Sample(defaultSampler, input.myUV);
	float2 masks = fullscreenTextureMask.Sample(defaultSampler, input.myUV).xy;
	if(masks.x == 1)
	{
		blend = fullscreenTextureNear.Sample(defaultSampler, input.myUV);
		val.rgb = blend.rgb;
		val.a = 1;
		output.myColor = val;
		return output;
	}
	else if(masks.y == 1)
	{
		blend = fullscreenTextureFar.Sample(defaultSampler, input.myUV);
		val.rgb = blend.rgb;
		val.a = 1;
		output.myColor = val;
		return output;
	}
	t = saturate(masks.y);
	blend = fullscreenTextureFar.Sample(defaultSampler, input.myUV);
	//blend.rgb = blend.rgb / (blend.rgb / midVal.rgb);
	val.rgb = lerp(midVal.rgb, blend.rgb, pow(t, 2));
	val.a = 1;
	output.myColor = val;
	return output;
}