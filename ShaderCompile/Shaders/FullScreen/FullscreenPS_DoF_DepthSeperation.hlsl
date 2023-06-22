#include "FullScreenStructs.hlsli"

struct DepthSplitOutput
{
    float4 far : SV_Target0;
    float4 near : SV_Target1;
	float4 mask : SV_Target2;
};
Texture2D TextureToSplit : register(t8);
DepthSplitOutput main(VertexToPixel input)
{
	DepthSplitOutput output;
	float4 val = TextureToSplit.Sample(defaultSampler, input.myUV);
	float depth = depth_Pass.Sample(defaultSampler, input.myUV).r;
	float worldDepth = worldDepthFromCameraZDepth(renderCamera.nearPlane, renderCamera.farPlane, depth);

	output.mask = float4(0, 0, 0, 0);
	float nearDistance = 0.f;
	if(worldDepth < nearDistance)
	{
		output.near = float4(val.rgb, 1);
		output.far = 0;
		output.mask = float4(1, (worldDepth / nearDistance),  smoothstep(0, 0.5f, (worldDepth / nearDistance)), 0.5f);
	}else if(worldDepth > DoF_focusDistance + 250)
	{
		output.near = 0;
		output.far = float4(val.rgb, 1);
		output.mask = float4(0, 1,  1, clamp((worldDepth - DoF_focusDistance * 1.5f) / 250, 0, 0.750f));
	}
	else
	{
		output.mask = float4( 0, (worldDepth - DoF_focusDistance) / 250, 0, 0);
		output.near = float4(val.rgb, depth);
		output.far = float4(val.rgb, depth);
	}
   
	return output;
}