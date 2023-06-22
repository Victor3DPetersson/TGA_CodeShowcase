#pragma once
#include "../CommonUtilities/CU/Utility/ShortString.h"
#include "../CommonUtilities/CU/Containers/GrowingArray.hpp"

#include <d3d11.h>
enum class MaterialTypes
{
	EPBR,
	EPBR_Transparent,
	EPBR_Anim,
	EPBRTransparent_Anim,
	EParticle_Default,
	EParticle_Glow,
	EDecal,
	ERenderTarget,
	ECount
};
enum class ShaderConfiguration
{
	VS,
	GS,
	PS,
	VS_PS,
	VS_GS,
	GS_PS,
	VS_GS_PS,
	COUNT
};

struct Material
{
	Material();	
	~Material();
	void ReleaseResources();
	bool operator<(const Material& aMaterial);

	static unsigned int myMaterialIDCounter;

	GUID myGUID;
	unsigned int myID;
	unsigned int myNumberOfTextures;
	unsigned int myPSEffectIndex;
	bool myIsCutOut = false;
	ShaderConfiguration myShaderConfig;
	MaterialTypes myMaterialType;
	ShortString myMaterialName;
	ID3D11VertexShader* myVertexShader;
	ID3D11PixelShader* myPixelShader;
	ID3D11GeometryShader* myGeometryShader;
	ID3D11ShaderResourceView* myTextures[12];
	ID3D11InputLayout* myInputLayout;
	D3D11_PRIMITIVE_TOPOLOGY myPrimitiveTopology;
};
