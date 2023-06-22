#include "Particle_Structs.hlsli"

PixelOutput main(GeometryToPixel input)
{
PixelOutput output;
	float4 TextureColor = ParticleTexture.Sample(defaultSampler, input.myUV);
	if(TextureColor.a == 0) { discard; }
	output.myColor.rgb = TextureColor.rgb * input.myColor.rgb * input.myEmissiveStrength;
	output.myColor.rgb *= input.myColor.a * TextureColor.a;
	output.myColor.a = TextureColor.a * input.myColor.a;
	return output;
}