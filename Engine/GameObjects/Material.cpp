#include "stdafx.h"
#include "GameObjects/Material.h"
#include "EngineInterface.h"
#include "Managers\TextureManager.h"
#include "Managers\Managers.h"

unsigned int Material::myMaterialIDCounter = 0;

Material::Material()
{
	myID = myMaterialIDCounter++;
	myNumberOfTextures = 0;
	myPSEffectIndex = 0;
	myGUID = NIL_UUID;
	myShaderConfig = ShaderConfiguration::COUNT;
	myMaterialType = MaterialTypes::ECount;
	myVertexShader = nullptr;
	myPixelShader = nullptr;
	myGeometryShader = nullptr;
	myVertexShader = nullptr;
	myInputLayout = nullptr;
	myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	for (unsigned int i = 0; i < 12; i++)
	{
		myTextures[i] = nullptr;
	}
}

Material::~Material()
{
	myVertexShader = nullptr;
	myPixelShader = nullptr;
	myGeometryShader = nullptr;
	myVertexShader = nullptr;
	myInputLayout = nullptr;
	myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	for (unsigned int i = 0; i < 12; i++)
	{
		myTextures[i] = nullptr;
	}
}

void Material::ReleaseResources()
{
	SAFE_RELEASE(myVertexShader);
	SAFE_RELEASE(myPixelShader);
	SAFE_RELEASE(myGeometryShader);
	SAFE_RELEASE(myInputLayout);
	for (unsigned int i = 0; i < 12; i++)
	{
		EngineInterface::GetManagers()->myTextureManager.ReturnTexture(myTextures[i]);
	}
	myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

bool Material::operator<(const Material& aMaterial)
{
	if (myID < aMaterial.myID)
	{
		return true;
	}
	return false;
}
