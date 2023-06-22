#include "stdafx.h"
#include "MaterialManager.h"

#include <d3d11.h>
#include "TextureManager.h"

#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <filesystem>

#include "Core\Rendering/Resources\FullScreenTexture_Factory.h"
#include "Core\Rendering/Resources\FullScreenTexture.h"
#include "Core\Rendering/Resources\GBuffer.h"
#include "EngineInterface.h"

bool Engine::MaterialManager::Init(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, TextureManager* aTextureManager)
{
	myDevice = aDevice;
	myContext = aContext;
	myTextureManager = aTextureManager;
	CreateDefaultMaterials();
	ReloadAllMaterials();
	myShouldReloadAllMaterials = false;
	myShouldReloadMaterial = false;
	v2ui resolution = EngineInterface::GetRenderResolution();

	for (size_t i = 0; i < (size_t)RenderRes::RenderRes_Count; i++)
	{
		v2f renderResolution;
		switch ((RenderRes)i)
		{
		case RenderRes::RenderRes_x16y9:
			renderResolution = resolution;
			break;
		case RenderRes::RenderRes_x16y9quarter:
			renderResolution = v2f(resolution.x / 4.f, resolution.y / 4.f);
			break;
		//case ERenderTargetTextureType::E4_3_2048:
		//	renderResolution = v2f(1440, 1440 * 0.75f);
		//	break;
		case RenderRes::RenderRes_xy512:
			renderResolution = v2f(512, 512);
			break;
		default:
			break;
		}
		myRenderTargetDepthTextures[i] = CreateDepthTexture(renderResolution, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, myDevice, myContext, EDepthStencilSRV::CREATE, EDepthStencilFlag::BOTH);
		myRenderTargetIntermediateTextures[i] = CreateFullScreenTexture(renderResolution, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext);
		myRenderTargetsGBuffer[i] = CreateGBuffer(renderResolution, myDevice, myContext);
	}
	for (unsigned short i = 0; i < 8; i++)
	{
		myRenderTargets[i][(int)RenderRes::RenderRes_x16y9] = CreateFullScreenTexture({ resolution.x, resolution.y }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext);
		//myRenderTargets[i][(int)ERenderTargetTextureType::E16_9half] = CreateFullScreenTexture({ resolution.x * 0.5, resolution.y * 0.5 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext);
		myRenderTargets[i][(int)RenderRes::RenderRes_x16y9quarter] = CreateFullScreenTexture({ resolution.x / 4U, resolution.y / 4U }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext);
		//myRenderTargets[i][(int)ERenderTargetTextureType::E1_1_1024] = CreateFullScreenTexture({ 1024, 1024 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext);
		//myRenderTargets[i][(int)ERenderTargetTextureType::E4_3_2048] = CreateFullScreenTexture({ 1440, (unsigned int)(1440 * 0.75f) }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext);
		myRenderTargets[i][(int)RenderRes::RenderRes_xy512] = CreateFullScreenTexture({ 512, 512 }, DXGI_FORMAT_R16G16B16A16_FLOAT, myDevice, myContext);
	}


	return false;
}

Material* Engine::MaterialManager::GetGratPlat(const ShortString& aGratKey, MaterialTypes aMaterialType)
{
	//auto t = myLoadedMaterials[(unsigned int)aMaterialType].find(aGratKey.GetString());
	//if (t == myLoadedMaterials[(unsigned int)aMaterialType].end())
	//{
	//	//return &myLoadedMaterials[(unsigned int)aMaterialType][aGratKey.GetString()];
	//	return &myDefaultMaterials[(unsigned int)aMaterialType];
	//}
	//return &myLoadedMaterials[(unsigned int)aMaterialType][aGratKey.GetString()];
	GUID* id = myLoadedStringsToGUID[(unsigned int)aMaterialType].Get(aGratKey.GetString());
	if (id)
	{
		Material* mat = myLoadedMaterials[(unsigned int)aMaterialType].Get(*id);
		if (mat)
		{
			return mat;
		}
	}
	return &myDefaultMaterials[(unsigned int)aMaterialType];
}

Material* Engine::MaterialManager::GetGratPlat(const GUID& aGratKey, MaterialTypes aMaterialType)
{
	Material* mat = myLoadedMaterials[(unsigned int)aMaterialType].Get(aGratKey);
	if (mat)
	{
		return mat;
	}
	return &myDefaultMaterials[(unsigned int)aMaterialType];
}

GUID Engine::MaterialManager::GetGUID(const FixedString256& aGratKey, MaterialTypes aMaterialType)
{
	GUID* id = myLoadedStringsToGUID[(unsigned int)aMaterialType].Get(aGratKey);
	if (id)
	{
		return *id;
	}
	return NIL_UUID;
}

void Engine::MaterialManager::ReloadMaterials()
{
	myShouldReloadAllMaterials = true;
}

void Engine::MaterialManager::ReloadMaterial(const ShortString& aGratKey, MaterialTypes aMaterialType)
{
	std::pair<ShortString, MaterialTypes> materialToReload = {aGratKey, aMaterialType};
	myMaterialsToReload.Enqueue(materialToReload);
	myShouldReloadMaterial = true;
}

void Engine::MaterialManager::ReleaseMaterials()
{
	for (size_t i = 0; i < (unsigned int)MaterialTypes::ECount; i++)
	{
		myLoadedMaterials[i].ForEach([](GUID&, Material& cmd, CU::Dictionary<GUID, Material>&)
		{
			cmd.ReleaseResources();
		});
		//for (auto mat : myLoadedMaterials[i])
		//{
		//	mat.second.ReleaseResources();
		//}
		myDefaultMaterials[i].ReleaseResources();
	}
}

void Engine::MaterialManager::Update()
{
	if (myShouldReloadAllMaterials)
	{
		ReloadAllMaterials();
		myShouldReloadAllMaterials = false;
	}
	if (myShouldReloadMaterial)
	{
		for (unsigned int i = 0; i < myMaterialsToReload.Size(); i++)
		{
			std::pair<ShortString, MaterialTypes> mat = myMaterialsToReload.Dequeue();

			Material material;
			if (LoadMaterialFromMaterialName(mat.first, mat.second, material))
			{
				GUID id = *myLoadedStringsToGUID[(unsigned int)mat.second].Get(material.myMaterialName.GetString());
				Material& loadedMat = *myLoadedMaterials[(unsigned int)mat.second][id];
				loadedMat = material;
				//myLoadedMaterials[(unsigned int)mat.second][material.myMaterialName.GetString()].ReleaseResources();
				//myLoadedMaterials[(unsigned int)mat.second][material.myMaterialName.GetString()] = material;
			}
		}
		myShouldReloadMaterial = false;
	}
}

void Engine::MaterialManager::ReloadAllMaterials()
{
	for (unsigned int  i = 0; i < (unsigned int)MaterialTypes::ECount; i++)
	{
		ReloadMaterialsOfType((MaterialTypes)i);
	}
	return;
}

void Engine::MaterialManager::ReloadMaterialsOfType(MaterialTypes aMaterialType)
{
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::wstring outpathAbs = currentPath;
	switch (aMaterialType)
	{
	case MaterialTypes::EPBR:
		outpathAbs.append(L"/Content/Materials/PBR/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::EPBR].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::EPBR].Insert(material.myGUID, material);
				}
			}
		}
		break;
	case MaterialTypes::EPBR_Transparent:
		outpathAbs.append(L"/Content/Materials/PBR_Transparent/");
		if (std::filesystem::exists(outpathAbs) == false) { break; }
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Transparent].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Transparent].Insert(material.myGUID, material);
					//myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Transparent][material.myMaterialName.GetString()].ReleaseResources();
					//myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Transparent][material.myMaterialName.GetString()] = material;
				}
			}
		}
		break;
	case MaterialTypes::EPBR_Anim:
		outpathAbs.append(L"/Content/Materials/PBR_Anim/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Anim].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Anim].Insert(material.myGUID, material);
					//myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Anim][material.myMaterialName.GetString()].ReleaseResources();
					//myLoadedMaterials[(unsigned int)MaterialTypes::EPBR_Anim][material.myMaterialName.GetString()] = material;
				}
			}
		}
		break;
	case MaterialTypes::EPBRTransparent_Anim:
		outpathAbs.append(L"/Content/Materials/PBR_Transparent_Anim/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::EPBRTransparent_Anim].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::EPBRTransparent_Anim].Insert(material.myGUID, material);
					//myLoadedMaterials[(unsigned int)MaterialTypes::EPBRTransparent_Anim][material.myMaterialName.GetString()].ReleaseResources();
					//myLoadedMaterials[(unsigned int)MaterialTypes::EPBRTransparent_Anim][material.myMaterialName.GetString()] = material;
				}
			}
		}
		break;
	case MaterialTypes::EParticle_Default:
		outpathAbs.append(L"/Content/Materials/Particle_Default/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Default].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Default].Insert(material.myGUID, material);
					//myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Default][material.myMaterialName.GetString()].ReleaseResources();
					//myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Default][material.myMaterialName.GetString()] = material;
				}
			}
		}
		break;
	case MaterialTypes::EParticle_Glow:
		outpathAbs.append(L"/Content/Materials/Particle_Glow/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Glow].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Glow].Insert(material.myGUID, material);
					//myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Glow][material.myMaterialName.GetString()].ReleaseResources();
					//myLoadedMaterials[(unsigned int)MaterialTypes::EParticle_Glow][material.myMaterialName.GetString()] = material;
				}
			}
		}
		break;
	case MaterialTypes::EDecal:
		outpathAbs.append(L"/Content/Materials/Decal/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::EDecal].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::EDecal].Insert(material.myGUID, material);
					//myLoadedMaterials[(unsigned int)MaterialTypes::EDecal][material.myMaterialName.GetString()].ReleaseResources();
					//myLoadedMaterials[(unsigned int)MaterialTypes::EDecal][material.myMaterialName.GetString()] = material;
				}
			}
		}
		break;
	case MaterialTypes::ERenderTarget:
		outpathAbs.append(L"/Content/Materials/RenderTarget/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
		{
			if (dirEntry.is_regular_file())
			{
				Material material;
				if (LoadMaterial(material, dirEntry.path()))
				{
					Material* loadedMat = myLoadedMaterials[(unsigned int)MaterialTypes::ERenderTarget].Get(material.myGUID);
					if (loadedMat)
					{
						loadedMat->ReleaseResources();
					}
					myLoadedMaterials[(unsigned int)MaterialTypes::ERenderTarget].Insert(material.myGUID, material);
					//myLoadedMaterials[(unsigned int)MaterialTypes::ERenderTarget][material.myMaterialName.GetString()].ReleaseResources();
					//myLoadedMaterials[(unsigned int)MaterialTypes::ERenderTarget][material.myMaterialName.GetString()] = material;
				}
			}
		}
		break;
	case MaterialTypes::ECount:
		break;
	default:
		break;
	}
	return;
}

bool Engine::MaterialManager::LoadMaterialFromMaterialName(const ShortString& aGratKey, MaterialTypes aMaterialType, Material& aMaterialToFill)
{
	std::filesystem::path currentPath = std::filesystem::current_path();

	std::wstring outpathAbs = currentPath;
	switch (aMaterialType)
	{
	case MaterialTypes::EPBR:
		outpathAbs.append(L"/Content/Materials/PBR/");
		break;
	case MaterialTypes::EPBR_Transparent:
		outpathAbs.append(L"/Content/Materials/PBR_Transparent/");
		break;
	case MaterialTypes::EPBR_Anim:
		outpathAbs.append(L"/Content/Materials/PBR_Anim/");
		break;
	case MaterialTypes::EPBRTransparent_Anim:
		outpathAbs.append(L"/Content/Materials/PBR_Transparent_Anim/");
		break;
	case MaterialTypes::EParticle_Default:
		outpathAbs.append(L"/Content/Materials/Particle_Default/");
		break;
	case MaterialTypes::EParticle_Glow:
		outpathAbs.append(L"/Content/Materials/Particle_Glow/");
		break;
	case MaterialTypes::EDecal:
		outpathAbs.append(L"/Content/Materials/Decal/");
		break;
	case MaterialTypes::ERenderTarget:
		outpathAbs.append(L"/Content/Materials/RenderTarget/");
		break;
	default:
		break;
	}
	bool foundMat = false;
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(outpathAbs))
	{
		std::string filename = dirEntry.path().filename().replace_extension().string();
		if (dirEntry.is_regular_file() && strcmp(filename.c_str(), aGratKey.GetString()) == 0)
		{
			outpathAbs = dirEntry.path().wstring().c_str();
			foundMat = true;
			break;
		}
	}
	if (foundMat == false)
	{
		return false;
	}
	return LoadMaterial(aMaterialToFill, outpathAbs);
}

bool Engine::MaterialManager::LoadMaterial(Material& aMaterialToFill, std::wstring aMaterialPath)
{
	std::ifstream iMD;
	iMD.open(aMaterialPath, std::ios::in | std::ios::binary);
	
	if (iMD)
	{
		unsigned int materialVersionIndex;
		iMD.read((char*)&materialVersionIndex, sizeof(materialVersionIndex));
		if (materialVersionIndex == 1)
		{
			struct ints
			{
				unsigned int shaderConfiguration;
				unsigned int mateiralType;
				unsigned int PrimitiveTopology;
				unsigned int numberOfShaders;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInTexture[4];
				unsigned int numberOfCharactersInShader[3];
				unsigned int numberOfCharactersInName;
			} intsToRead;
			struct strings
			{
				char myMaterialName[128];
				char myTextures[4][128];
				char myShaders[3][128];
			} stringsToRead;

			iMD.read((char*)&intsToRead, sizeof(ints));
			iMD.read((char*)&stringsToRead, sizeof(strings));

			iMD.close();

			aMaterialToFill.myShaderConfig = (ShaderConfiguration)intsToRead.shaderConfiguration;
			aMaterialToFill.myMaterialType = (MaterialTypes)intsToRead.mateiralType;
			aMaterialToFill.myPrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)intsToRead.PrimitiveTopology;
			aMaterialToFill.myNumberOfTextures = intsToRead.numberOfTextures;
			memcpy(&aMaterialToFill.myMaterialName, &stringsToRead.myMaterialName, intsToRead.numberOfCharactersInName);
			for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
			{
				aMaterialToFill.myTextures[i] = myTextureManager->GetTextureObject(stringsToRead.myTextures[i], (ETextureTypes)i);
			}
			aMaterialToFill.myVertexShader = nullptr;
			aMaterialToFill.myGeometryShader = nullptr;
			aMaterialToFill.myPixelShader = nullptr;

			std::string VSData;
			switch (aMaterialToFill.myShaderConfig)
			{
			case ShaderConfiguration::VS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				break;
			case ShaderConfiguration::GS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::PS:
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::GS_PS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::COUNT:
				break;
			default:
				break;
			}
			if (!VSData.empty())
			{
				ID3D11InputLayout* inputLayout = GetInputLayout(aMaterialToFill.myMaterialType, VSData);
				if (inputLayout)
				{
					aMaterialToFill.myInputLayout = inputLayout;
				}
				else
				{
					return false;
				}
			}
			return true;
		}
		if (materialVersionIndex == 2)
		{
			struct ints
			{
				unsigned int shaderConfiguration;
				unsigned int mateiralType;
				unsigned int PrimitiveTopology;
				unsigned int numberOfShaders;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInTexture[8];
				unsigned int numberOfCharactersInShader[3];
				unsigned int numberOfCharactersInName;
			} intsToRead;
			struct strings
			{
				char myMaterialName[128];
				char myTextures[8][128];
				char myShaders[3][128];
			} stringsToRead;

			iMD.read((char*)&intsToRead, sizeof(ints));
			iMD.read((char*)&stringsToRead, sizeof(strings));

			iMD.close();

			aMaterialToFill.myShaderConfig = (ShaderConfiguration)intsToRead.shaderConfiguration;
			aMaterialToFill.myMaterialType = (MaterialTypes)intsToRead.mateiralType;
			aMaterialToFill.myPrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)intsToRead.PrimitiveTopology;
			aMaterialToFill.myNumberOfTextures = intsToRead.numberOfTextures;
			memcpy(&aMaterialToFill.myMaterialName, &stringsToRead.myMaterialName, intsToRead.numberOfCharactersInName);
			for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
			{
				aMaterialToFill.myTextures[i] = myTextureManager->GetTextureObject(stringsToRead.myTextures[i], (ETextureTypes)i);
			}
			aMaterialToFill.myVertexShader = nullptr;
			aMaterialToFill.myGeometryShader = nullptr;
			aMaterialToFill.myPixelShader = nullptr;

			std::string VSData;
			switch (aMaterialToFill.myShaderConfig)
			{
			case ShaderConfiguration::VS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				break;
			case ShaderConfiguration::GS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::PS:
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::GS_PS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::COUNT:
				break;
			default:
				break;
			}
			if (!VSData.empty())
			{
				ID3D11InputLayout* inputLayout = GetInputLayout(aMaterialToFill.myMaterialType, VSData);
				if (inputLayout)
				{
					aMaterialToFill.myInputLayout = inputLayout;
				}
				else
				{
					return false;
				}
			}
		}
		if (materialVersionIndex == 3)
		{
			struct ints
			{
				unsigned int shaderConfiguration;
				unsigned int mateiralType;
				unsigned int PrimitiveTopology;
				unsigned int numberOfShaders;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInTexture[16];
				unsigned int numberOfCharactersInShader[3];
				unsigned int numberOfCharactersInName;
			} intsToRead;
			struct strings
			{
				char myMaterialName[128];
				char myTextures[16][128];
				char myShaders[3][128];
			} stringsToRead;

			iMD.read((char*)&intsToRead, sizeof(ints));
			iMD.read((char*)&stringsToRead, sizeof(strings));

			iMD.close();

			aMaterialToFill.myShaderConfig = (ShaderConfiguration)intsToRead.shaderConfiguration;
			aMaterialToFill.myMaterialType = (MaterialTypes)intsToRead.mateiralType;
			aMaterialToFill.myPrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)intsToRead.PrimitiveTopology;
			aMaterialToFill.myNumberOfTextures = intsToRead.numberOfTextures;
			memcpy(&aMaterialToFill.myMaterialName, &stringsToRead.myMaterialName, intsToRead.numberOfCharactersInName);
			for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
			{
				aMaterialToFill.myTextures[i] = myTextureManager->GetTextureObject(stringsToRead.myTextures[i], (ETextureTypes)i);
			}
			aMaterialToFill.myVertexShader = nullptr;
			aMaterialToFill.myGeometryShader = nullptr;
			aMaterialToFill.myPixelShader = nullptr;

			std::string VSData;
			switch (aMaterialToFill.myShaderConfig)
			{
			case ShaderConfiguration::VS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				break;
			case ShaderConfiguration::GS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::PS:
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::GS_PS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::COUNT:
				break;
			default:
				break;
			}
			if (!VSData.empty())
			{
				ID3D11InputLayout* inputLayout = GetInputLayout(aMaterialToFill.myMaterialType, VSData);
				if (inputLayout)
				{
					aMaterialToFill.myInputLayout = inputLayout;
				}
				else
				{
					return false;
				}
			}
		}
		if (materialVersionIndex == 4)
		{
			struct ints
			{
				unsigned int shaderConfiguration;
				unsigned int mateiralType;
				unsigned int PrimitiveTopology;
				unsigned int numberOfShaders;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInTexture[16];
				unsigned int numberOfCharactersInShader[3];
				unsigned int numberOfCharactersInName;
				unsigned int myRenderCutOut;
			} intsToRead;
			struct strings
			{
				char myMaterialName[128];
				char myTextures[16][128];
				char myShaders[3][128];
			} stringsToRead;

			iMD.read((char*)&intsToRead, sizeof(ints));
			iMD.read((char*)&stringsToRead, sizeof(strings));

			iMD.close();

			if (intsToRead.myRenderCutOut == 1)
			{
				aMaterialToFill.myIsCutOut = true;
			}
			else
			{
				aMaterialToFill.myIsCutOut = false;
			}

			aMaterialToFill.myShaderConfig = (ShaderConfiguration)intsToRead.shaderConfiguration;
			aMaterialToFill.myMaterialType = (MaterialTypes)intsToRead.mateiralType;
			aMaterialToFill.myPrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)intsToRead.PrimitiveTopology;
			aMaterialToFill.myNumberOfTextures = intsToRead.numberOfTextures;
			memcpy(&aMaterialToFill.myMaterialName, &stringsToRead.myMaterialName, intsToRead.numberOfCharactersInName);
			for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
			{
				aMaterialToFill.myTextures[i] = myTextureManager->GetTextureObject(stringsToRead.myTextures[i], (ETextureTypes)i);
			}
			aMaterialToFill.myVertexShader = nullptr;
			aMaterialToFill.myGeometryShader = nullptr;
			aMaterialToFill.myPixelShader = nullptr;

			std::string VSData;
			switch (aMaterialToFill.myShaderConfig)
			{
			case ShaderConfiguration::VS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				break;
			case ShaderConfiguration::GS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::PS:
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				break;
			case ShaderConfiguration::GS_PS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::VS_GS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.myShaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.myShaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.myShaders[2]);
				break;
			case ShaderConfiguration::COUNT:
				break;
			default:
				break;
			}
			if (!VSData.empty())
			{
				ID3D11InputLayout* inputLayout = GetInputLayout(aMaterialToFill.myMaterialType, VSData);
				if (inputLayout)
				{
					aMaterialToFill.myInputLayout = inputLayout;
				}
				else
				{
					return false;
				}
			}
		}
		if (materialVersionIndex == 5)
		{
			struct ints
			{
				unsigned int shaderConfiguration;
				unsigned int materialType;
				unsigned int primitiveTopology;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInName;
				unsigned int renderCutout;
			} intsToRead;
			struct strings
			{
				char materialName[128];
				char textures[MAX_TEXTURE_AMOUNT][128];
				char shaders[3][128];
			} stringsToRead;
			MaterialExportStruct exportStruct;
			iMD.read((char*)&intsToRead, sizeof(ints));
			iMD.read((char*)&stringsToRead, sizeof(strings));

			iMD.close();
			memcpy(&exportStruct, &stringsToRead, sizeof(MaterialExportStruct));
			if (intsToRead.renderCutout == 1)
			{
				aMaterialToFill.myIsCutOut = true;
			}
			else
			{
				aMaterialToFill.myIsCutOut = false;
			}
			aMaterialToFill.myShaderConfig = (ShaderConfiguration)intsToRead.shaderConfiguration;
			aMaterialToFill.myMaterialType = (MaterialTypes)intsToRead.materialType;
			aMaterialToFill.myPrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)intsToRead.primitiveTopology;
			aMaterialToFill.myNumberOfTextures = intsToRead.numberOfTextures;
			memcpy(&aMaterialToFill.myMaterialName, &stringsToRead.materialName, intsToRead.numberOfCharactersInName);
			for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
			{
				aMaterialToFill.myTextures[i] = myTextureManager->GetTextureObject(stringsToRead.textures[i], (ETextureTypes)i);
			}
			aMaterialToFill.myVertexShader = nullptr;
			aMaterialToFill.myGeometryShader = nullptr;
			aMaterialToFill.myPixelShader = nullptr;

			std::string VSData;
			switch (aMaterialToFill.myShaderConfig)
			{
			case ShaderConfiguration::VS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				break;
			case ShaderConfiguration::GS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				break;
			case ShaderConfiguration::PS:
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_GS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				break;
			case ShaderConfiguration::GS_PS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_GS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::COUNT:
				break;
			default:
				break;
			}
			if (!VSData.empty())
			{
				ID3D11InputLayout* inputLayout = GetInputLayout(aMaterialToFill.myMaterialType, VSData);
				if (inputLayout)
				{
					aMaterialToFill.myInputLayout = inputLayout;
				}
				else
				{
					return false;
				}
			}
			UuidCreate(&aMaterialToFill.myGUID);
			ExportMaterial(aMaterialToFill, exportStruct);
		}
		if (materialVersionIndex == 6)
		{
			struct ints
			{
				unsigned int shaderConfiguration;
				unsigned int materialType;
				unsigned int primitiveTopology;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInName;
				unsigned int renderCutout;
			} intsToRead;
			struct strings
			{
				char materialName[128];
				char textures[MAX_TEXTURE_AMOUNT][128];
				char shaders[3][128];
			} stringsToRead;

			iMD.read((char*)&aMaterialToFill.myGUID, sizeof(GUID));
			iMD.read((char*)&intsToRead, sizeof(ints));
			iMD.read((char*)&stringsToRead, sizeof(strings));

			iMD.close();

			if (intsToRead.renderCutout == 1)
			{
				aMaterialToFill.myIsCutOut = true;
			}
			else
			{
				aMaterialToFill.myIsCutOut = false;
			}

			aMaterialToFill.myShaderConfig = (ShaderConfiguration)intsToRead.shaderConfiguration;
			aMaterialToFill.myMaterialType = (MaterialTypes)intsToRead.materialType;
			aMaterialToFill.myPrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)intsToRead.primitiveTopology;
			aMaterialToFill.myNumberOfTextures = intsToRead.numberOfTextures;
			memcpy(&aMaterialToFill.myMaterialName, &stringsToRead.materialName, intsToRead.numberOfCharactersInName);
			for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
			{
				aMaterialToFill.myTextures[i] = myTextureManager->GetTextureObject(stringsToRead.textures[i], (ETextureTypes)i);
			}
			aMaterialToFill.myVertexShader = nullptr;
			aMaterialToFill.myGeometryShader = nullptr;
			aMaterialToFill.myPixelShader = nullptr;

			std::string VSData;
			switch (aMaterialToFill.myShaderConfig)
			{
			case ShaderConfiguration::VS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				break;
			case ShaderConfiguration::GS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				break;
			case ShaderConfiguration::PS:
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_GS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				break;
			case ShaderConfiguration::GS_PS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_GS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::COUNT:
				break;
			default:
				break;
			}
			if (!VSData.empty())
			{
				ID3D11InputLayout* inputLayout = GetInputLayout(aMaterialToFill.myMaterialType, VSData);
				if (inputLayout)
				{
					aMaterialToFill.myInputLayout = inputLayout;
				}
				else
				{
					return false;
				}
			}
		}
		if (materialVersionIndex == 7)
		{
			struct ints
			{
				unsigned int shaderConfiguration;
				unsigned int materialType;
				unsigned int primitiveTopology;
				unsigned int numberOfTextures;
				unsigned int numberOfCharactersInName;
				unsigned int renderCutout;
				unsigned int deferredEffect;
			} intsToRead;
			struct strings
			{
				char materialName[128];
				char textures[MAX_TEXTURE_AMOUNT][128];
				char shaders[3][128];
			} stringsToRead;

			iMD.read((char*)&aMaterialToFill.myGUID, sizeof(GUID));
			iMD.read((char*)&intsToRead, sizeof(ints));
			iMD.read((char*)&stringsToRead, sizeof(strings));

			iMD.close();

			if (intsToRead.renderCutout == 1)
			{
				aMaterialToFill.myIsCutOut = true;
			}
			else
			{
				aMaterialToFill.myIsCutOut = false;
			}

			aMaterialToFill.myShaderConfig = (ShaderConfiguration)intsToRead.shaderConfiguration;
			aMaterialToFill.myMaterialType = (MaterialTypes)intsToRead.materialType;
			aMaterialToFill.myPrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)intsToRead.primitiveTopology;
			aMaterialToFill.myNumberOfTextures = intsToRead.numberOfTextures;
			aMaterialToFill.myPSEffectIndex = intsToRead.deferredEffect;
			memcpy(&aMaterialToFill.myMaterialName, &stringsToRead.materialName, intsToRead.numberOfCharactersInName);
			for (unsigned int i = 0; i < intsToRead.numberOfTextures; i++)
			{
				aMaterialToFill.myTextures[i] = myTextureManager->GetTextureObject(stringsToRead.textures[i], (ETextureTypes)i);
			}
			aMaterialToFill.myVertexShader = nullptr;
			aMaterialToFill.myGeometryShader = nullptr;
			aMaterialToFill.myPixelShader = nullptr;

			std::string VSData;
			switch (aMaterialToFill.myShaderConfig)
			{
			case ShaderConfiguration::VS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				break;
			case ShaderConfiguration::GS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				break;
			case ShaderConfiguration::PS:
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_GS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				break;
			case ShaderConfiguration::GS_PS:
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::VS_GS_PS:
				aMaterialToFill.myVertexShader = LoadVS(stringsToRead.shaders[0], VSData);
				aMaterialToFill.myGeometryShader = LoadGS(stringsToRead.shaders[1]);
				aMaterialToFill.myPixelShader = LoadPS(stringsToRead.shaders[2]);
				break;
			case ShaderConfiguration::COUNT:
				break;
			default:
				break;
			}
			if (!VSData.empty())
			{
				ID3D11InputLayout* inputLayout = GetInputLayout(aMaterialToFill.myMaterialType, VSData);
				if (inputLayout)
				{
					aMaterialToFill.myInputLayout = inputLayout;
				}
				else
				{
					return false;
				}
			}
		}

		myLoadedStringsToGUID[(unsigned int)aMaterialToFill.myMaterialType].Insert(aMaterialToFill.myMaterialName.GetString(), aMaterialToFill.myGUID);
		return true;
	}
	return false;
}

void Engine::MaterialManager::CreateDefaultMaterials()
{
	for (size_t i = 0; i < (unsigned int)MaterialTypes::ECount; i++)
	{
		Material* mat = &myDefaultMaterials[i];
		GUID guid = NIL_UUID;
		std::string vsData;
		switch ((MaterialTypes)i)
		{
		case MaterialTypes::EPBR:
			mat->myMaterialType = MaterialTypes::EPBR;
			mat->myMaterialName = "Default_PBR";
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myIsCutOut = false;
			mat->myShaderConfig = ShaderConfiguration::VS_PS;

			mat->myVertexShader = LoadVS("Content/Shaders/Deferred_GBuffer_VS.cso", vsData);
			mat->myInputLayout = GetInputLayout(MaterialTypes::EPBR, vsData);
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			mat->myGeometryShader = nullptr;

			mat->myPixelShader = LoadPS("Content/Shaders/Deferred_GBuffer_PS.cso");

			mat->myNumberOfTextures = 4;
			mat->myTextures[0] = myTextureManager->GetTextureObject("", ETextureTypes::eColor);
			mat->myTextures[1] = myTextureManager->GetTextureObject("", ETextureTypes::eNormal);
			mat->myTextures[2] = myTextureManager->GetTextureObject("", ETextureTypes::eMaterial);
			mat->myTextures[3] = myTextureManager->GetTextureObject("", ETextureTypes::eEmissive);


			break;
		case MaterialTypes::EPBR_Transparent:
			mat->myMaterialType = MaterialTypes::EPBR_Transparent;
			mat->myMaterialName = "Default_PBR_Transparent";

			mat->myIsCutOut = true;
			mat->myShaderConfig = ShaderConfiguration::VS_PS;
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myVertexShader = LoadVS("Content/Shaders/VertexShader_Forward.cso", vsData);
			mat->myInputLayout = GetInputLayout(MaterialTypes::EPBR_Transparent, vsData);
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			mat->myGeometryShader = nullptr;

			mat->myPixelShader = LoadPS("Content/Shaders/PBR_Forward.cso");

			mat->myNumberOfTextures = 4;
			mat->myTextures[0] = myTextureManager->GetTextureObject("", ETextureTypes::eColor);
			mat->myTextures[1] = myTextureManager->GetTextureObject("", ETextureTypes::eNormal);
			mat->myTextures[2] = myTextureManager->GetTextureObject("", ETextureTypes::eMaterial);
			mat->myTextures[3] = myTextureManager->GetTextureObject("", ETextureTypes::eEmissive);
			break;
		case MaterialTypes::EPBR_Anim:
			mat->myMaterialType = MaterialTypes::EPBR_Anim;
			mat->myMaterialName = "Default_PBR_Animation";

			mat->myIsCutOut = false;
			mat->myShaderConfig = ShaderConfiguration::VS_PS;
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myVertexShader = LoadVS("Content/Shaders/GBuffer_VS_Animated.cso", vsData);
			mat->myInputLayout = GetInputLayout(MaterialTypes::EPBR_Anim, vsData);
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			mat->myGeometryShader = nullptr;

			mat->myPixelShader = LoadPS("Content/Shaders/Deferred_GBuffer_PS.cso");

			mat->myNumberOfTextures = 4;
			mat->myTextures[0] = myTextureManager->GetTextureObject("", ETextureTypes::eColor);
			mat->myTextures[1] = myTextureManager->GetTextureObject("", ETextureTypes::eNormal);
			mat->myTextures[2] = myTextureManager->GetTextureObject("", ETextureTypes::eMaterial);
			mat->myTextures[3] = myTextureManager->GetTextureObject("", ETextureTypes::eEmissive);
			break;
		case MaterialTypes::EPBRTransparent_Anim:
			mat->myMaterialType = MaterialTypes::EPBRTransparent_Anim;
			mat->myMaterialName = "Default_PBR_Animation_Transparent";

			mat->myIsCutOut = true;
			mat->myShaderConfig = ShaderConfiguration::VS_PS;
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myVertexShader = LoadVS("Content/Shaders/GBuffer_VS_Animated.cso", vsData);
			mat->myInputLayout = GetInputLayout(MaterialTypes::EPBRTransparent_Anim, vsData);
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			mat->myGeometryShader = nullptr;

			mat->myPixelShader = LoadPS("Content/Shaders/PBR_Forward.cso");

			mat->myNumberOfTextures = 4;
			mat->myTextures[0] = myTextureManager->GetTextureObject("", ETextureTypes::eColor);
			mat->myTextures[1] = myTextureManager->GetTextureObject("", ETextureTypes::eNormal);
			mat->myTextures[2] = myTextureManager->GetTextureObject("", ETextureTypes::eMaterial);
			mat->myTextures[3] = myTextureManager->GetTextureObject("", ETextureTypes::eEmissive);
			break;
		case MaterialTypes::EParticle_Default:
			mat->myMaterialType = MaterialTypes::EParticle_Default;
			mat->myMaterialName = "Default_Particle";

			mat->myIsCutOut = true;
			mat->myShaderConfig = ShaderConfiguration::VS_GS_PS;
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myVertexShader = LoadVS("Content/Shaders/Particle_VS.cso", vsData);
			mat->myInputLayout = GetInputLayout(MaterialTypes::EParticle_Default, vsData);
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

			mat->myGeometryShader = LoadGS("Content/Shaders/Particle_GS.cso");
			mat->myPixelShader = LoadPS("Content/Shaders/Particle_PS.cso");

			mat->myNumberOfTextures = 1;
			mat->myTextures[0] = myTextureManager->GetTextureObject("Content/Textures/default_particle_texture.dds", ETextureTypes::eColor);
			break;
		case MaterialTypes::EParticle_Glow:
			mat->myMaterialType = MaterialTypes::EParticle_Glow;
			mat->myMaterialName = "Default_Particle_Glow";

			mat->myIsCutOut = true;
			mat->myShaderConfig = ShaderConfiguration::VS_GS_PS;
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myVertexShader = LoadVS("Content/Shaders/Particle_VS.cso", vsData);
			mat->myInputLayout = GetInputLayout(MaterialTypes::EParticle_Glow, vsData);
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

			mat->myGeometryShader = LoadGS("Content/Shaders/Particle_GS.cso");
			mat->myPixelShader = LoadPS("Content/Shaders/Particle_PS.cso");

			mat->myNumberOfTextures = 1;
			mat->myTextures[0] = myTextureManager->GetTextureObject("Content/Textures/default_particle_texture.dds", ETextureTypes::eColor);
			break;
		case MaterialTypes::EDecal:
			mat->myMaterialType = MaterialTypes::EDecal;
			mat->myMaterialName = "Default_Decal";

			mat->myIsCutOut = false;
			mat->myShaderConfig = ShaderConfiguration::PS;
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myVertexShader = nullptr;
			mat->myInputLayout = nullptr;
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

			mat->myGeometryShader = nullptr;
			mat->myPixelShader = LoadPS("Content/Shaders/Decal_DefaultPBR.cso");

			mat->myNumberOfTextures = 4;
			mat->myTextures[0] = myTextureManager->GetTextureObject("", ETextureTypes::eColor);
			mat->myTextures[1] = myTextureManager->GetTextureObject("", ETextureTypes::eNormal);
			mat->myTextures[2] = myTextureManager->GetTextureObject("", ETextureTypes::eMaterial);
			mat->myTextures[3] = myTextureManager->GetTextureObject("", ETextureTypes::eEmissive);
			break;
		case MaterialTypes::ERenderTarget:
			mat->myMaterialType = MaterialTypes::ERenderTarget;
			mat->myMaterialName = "Default_RT";
			mat->myIsCutOut = false;
			mat->myShaderConfig = ShaderConfiguration::VS_PS;
			guid.Data1 = (unsigned long)i;
			mat->myGUID = guid;
			mat->myVertexShader = LoadVS("Content/Shaders/Deferred_GBuffer_VS.cso", vsData);
			mat->myInputLayout = GetInputLayout(MaterialTypes::EPBR, vsData);
			mat->myPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			mat->myGeometryShader = nullptr;

			mat->myPixelShader = LoadPS("Content/Shaders/PS_RenderTarget_Basic.cso");

			mat->myNumberOfTextures = 4;
			mat->myTextures[0] = myTextureManager->GetTextureObject("", ETextureTypes::eColor);
			mat->myTextures[1] = myTextureManager->GetTextureObject("", ETextureTypes::eNormal);
			mat->myTextures[2] = myTextureManager->GetTextureObject("", ETextureTypes::eMaterial);
			mat->myTextures[3] = myTextureManager->GetTextureObject("", ETextureTypes::eEmissive);
			break;
		default:
			break;
		}
		//myLoadedMaterials[i].Insert(mat->myGUID, *mat);
		//myLoadedStringsToGUID[i].Insert(mat->myMaterialName.GetString(), mat->myGUID);
	}
}

ID3D11VertexShader* Engine::MaterialManager::LoadVS(ShortString aPath, std::string& someVSData)
{
	HRESULT result;

	std::ifstream file;
	file.open(aPath.GetString(), std::ios::binary);
	someVSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11VertexShader* VS;
	result = myDevice->CreateVertexShader(someVSData.data(), someVSData.size(), nullptr, &VS);
	if (FAILED(result))
	{
		assert(false && "Could not load Shader");
		return nullptr;
	}
	file.close();
	return VS;
}

ID3D11GeometryShader* Engine::MaterialManager::LoadGS(ShortString aPath)
{
	HRESULT result;

	std::ifstream file;
	file.open(aPath.GetString(), std::ios::binary);
	std::string someGSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11GeometryShader* GS;
	result = myDevice->CreateGeometryShader(someGSData.data(), someGSData.size(), nullptr, &GS);
	if (FAILED(result))
	{
		assert(false && "Could not load Shader");
		return nullptr;
	}
	file.close();
	return GS;
}

ID3D11PixelShader* Engine::MaterialManager::LoadPS(ShortString aPath)
{
	HRESULT result;

	std::ifstream file;
	file.open(aPath.GetString(), std::ios::binary);
	std::string somePSData = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	ID3D11PixelShader* PS;
	result = myDevice->CreatePixelShader(somePSData.data(), somePSData.size(), nullptr, &PS);
	if (FAILED(result))
	{
		assert(false && "Could not load Shader");
		return nullptr;
	}
	file.close();
	return PS;
}

constexpr D3D11_INPUT_ELEMENT_DESC solidLayout[] =
{
	{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BINORMAL",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv1
	{ "TEXCOORD",	1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv2
	{ "TEXCOORD",	2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv3
	{ "TEXCOORD",	3, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv3
};

constexpr D3D11_INPUT_ELEMENT_DESC animationLayout[] =
{
	{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BINORMAL",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv1
	{ "TEXCOORD",	1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv2
	{ "TEXCOORD",	2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv3
	{ "TEXCOORD",	3, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },//uv3
	{ "BONEIDS",	0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{ "BONEWEIGHTS",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

constexpr D3D11_INPUT_ELEMENT_DESC particleLayout[] =
{
	{ "COLOR",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "VELOCITY",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "SIZE",0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "LIFETIME",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TIME",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "DISTANCE",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "EMISSIVE",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "ROTATION",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "ROTATIONDIR",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "PANNINGSPEED",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "CURRENT_COLOR",0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "ROTATIONY",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "ROTATIONZ",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "SIZEZ",0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BILLBOARDSTATE",0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};


ID3D11InputLayout* Engine::MaterialManager::GetInputLayout(MaterialTypes aMaterialType, std::string aVSData)
{
	HRESULT result;

	ID3D11InputLayout* InputLayout = nullptr;

	switch (aMaterialType)
	{
	case MaterialTypes::EPBR:
		result = myDevice->CreateInputLayout(solidLayout, sizeof(solidLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), aVSData.data(), aVSData.size(), &InputLayout);
		if (FAILED(result))
		{
			return nullptr;
		}
		break;
	case MaterialTypes::EPBR_Transparent:

		result = myDevice->CreateInputLayout(solidLayout, sizeof(solidLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), aVSData.data(), aVSData.size(), &InputLayout);
		if (FAILED(result))
		{
			return nullptr;
		}
		break;

	case MaterialTypes::EPBR_Anim:

		result = myDevice->CreateInputLayout(animationLayout, sizeof(animationLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), aVSData.data(), aVSData.size(), &InputLayout);
		if (FAILED(result))
		{
			return nullptr;
		}
		break;
	case MaterialTypes::EPBRTransparent_Anim:

		result = myDevice->CreateInputLayout(animationLayout, sizeof(animationLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), aVSData.data(), aVSData.size(), &InputLayout);
		if (FAILED(result))
		{
			return nullptr;
		}
		break;
	case MaterialTypes::EParticle_Default:

		result = myDevice->CreateInputLayout(particleLayout, sizeof(particleLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), aVSData.data(), aVSData.size(), &InputLayout);
		if (FAILED(result))
		{
			return nullptr;
		}
		break;
	case MaterialTypes::EParticle_Glow:
		result = myDevice->CreateInputLayout(particleLayout, sizeof(particleLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), aVSData.data(), aVSData.size(), &InputLayout);
		if (FAILED(result))
		{
			return nullptr;
		}
		break;
	case MaterialTypes::ECount:
		break;
	case MaterialTypes::ERenderTarget:
		result = myDevice->CreateInputLayout(solidLayout, sizeof(solidLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC), aVSData.data(), aVSData.size(), &InputLayout);
		if (FAILED(result))
		{
			return nullptr;
		}
		break;
	default:
		break;
	}

	return InputLayout;

}

constexpr unsigned int matVersionNumb = 6;
void Engine::MaterialManager::ExportMaterial(Material& aMatToExport, MaterialExportStruct aExportStruct)
{
	struct MaterialExportStruct
	{
		unsigned int materialVersionIndex;
		GUID id;
		unsigned int shaderConfiguration;
		unsigned int materialType;
		unsigned int primitiveTopology;
		unsigned int numberOfTextures;
		unsigned int numberOfCharactersInName;
		unsigned int renderCutout;
		char myMaterialName[128];
		char myTextures[MAX_TEXTURE_AMOUNT][128];
		char myShaders[3][128];
	};

	MaterialExportStruct exportStruct;
	exportStruct.materialVersionIndex = matVersionNumb;
	exportStruct.shaderConfiguration = (unsigned int)aMatToExport.myShaderConfig;
	exportStruct.materialType = (unsigned int)aMatToExport.myMaterialType;
	exportStruct.primitiveTopology = (unsigned int)aMatToExport.myPrimitiveTopology;
	exportStruct.numberOfTextures = (unsigned int)aMatToExport.myNumberOfTextures;
	exportStruct.numberOfCharactersInName = (unsigned int)0;
	size_t nameLength = strlen(aMatToExport.myMaterialName.Data());
	for (unsigned short c = 0; c < 128; c++)
	{
		if (c < nameLength)
		{
			exportStruct.numberOfCharactersInName++;
			exportStruct.myMaterialName[c] = aMatToExport.myMaterialName[c];
		}
		else
		{
			exportStruct.myMaterialName[c] = 0;
		}
	}
	for (unsigned int i = 0; i < MAX_TEXTURE_AMOUNT; i++)
	{
		size_t numbCharsInTexture = strlen(aExportStruct.texturePaths[i]);
		for (unsigned short c = 0; c < 128; c++)
		{
			if (i < exportStruct.numberOfTextures)
			{
				if (c < numbCharsInTexture)
				{
					exportStruct.myTextures[i][c] = aExportStruct.texturePaths[i][c];
				}
				else
				{
					exportStruct.myTextures[i][c] = 0;
				}
			}
			else
			{
				exportStruct.myTextures[i][c] = 0;
			}
		}
	}
	if (aMatToExport.myIsCutOut)
	{
		exportStruct.renderCutout = 1;
	}
	else
	{
		exportStruct.renderCutout = 0;
	}
	/// Koden här inne är as äcklig, men man behöver aldrig peta på den igen, Den går över mina strings baserat på vilken Shader config man kör 
	/// och sparar rätt shader paths bsaerat på vilken config det är
	std::string shaderType;
	switch (aMatToExport.myMaterialType)
	{
	case MaterialTypes::EPBR:
		shaderType = "PBR";
		break;
	case MaterialTypes::EPBR_Transparent:
		shaderType = "PBR_Transparent";
		break;
	case MaterialTypes::EPBR_Anim:
		shaderType = "PBR_Anim";
		break;
	case MaterialTypes::EPBRTransparent_Anim:
		shaderType = "PBR_Transparent_Anim";
		break;
	case MaterialTypes::EParticle_Default:
		shaderType = "Particle_Default";
		break;
	case MaterialTypes::EParticle_Glow:
		shaderType = "Particle_Glow";
		break;
	case MaterialTypes::EDecal:
		shaderType = "Decal";
		break;
	case MaterialTypes::ERenderTarget:
		shaderType = "RenderTarget";
		break;
	default:
		break;
	}
	for (unsigned short c = 0; c < 128; c++)
	{
		size_t VSSize = strlen(aExportStruct.myShaders[0]);
		if (c < VSSize)
		{
			exportStruct.myShaders[0][c] = aExportStruct.myShaders[0][c];
		}
		else
		{
			exportStruct.myShaders[0][c] = 0;
		}
		size_t GSSize = strlen(aExportStruct.myShaders[1]);
		if (c < GSSize)
		{
			exportStruct.myShaders[1][c] = aExportStruct.myShaders[1][c];
		}
		else
		{
			exportStruct.myShaders[1][c] = 0;
		}
		size_t PSSize = strlen(aExportStruct.myShaders[2]);
		if (c < PSSize)
		{
			exportStruct.myShaders[2][c] = aExportStruct.myShaders[2][c];
		}
		else
		{
			exportStruct.myShaders[2][c] = 0;
		}
	}

	//std::string modelPath = "Content/Materials/" + shaderType;// + "/" + std::string(matData.materialName.Data()) + ".gratplat";
	std::string modelPath = "Content/Materials/" + shaderType + "/";

	std::filesystem::path currentPath = std::filesystem::current_path();
	std::string outpathAbs = currentPath.u8string();
	outpathAbs.append('/' + modelPath);

	bool foundMat = false;
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(modelPath))
	{
		std::string filename = dirEntry.path().filename().replace_extension().string();
		if (dirEntry.is_regular_file() && strcmp(filename.c_str(), exportStruct.myMaterialName) == 0)
		{
			outpathAbs = dirEntry.path().relative_path().string().c_str();
			foundMat = true;
			break;
		}
	}
	if (foundMat)
	{
		if (std::filesystem::exists(outpathAbs.c_str()))
		{
			std::filesystem::permissions(outpathAbs.c_str(), std::filesystem::perms::all);
			std::filesystem::remove(outpathAbs.c_str());
		}
	}
	else
	{
		UuidCreate(&aMatToExport.myGUID);
		outpathAbs = modelPath + aMatToExport.myMaterialName.Data() + ".gratplat";
	}
	if (aMatToExport.myGUID == NIL_UUID)
	{
		UuidCreate(&aMatToExport.myGUID);
	}
	exportStruct.id = aMatToExport.myGUID;

	std::ofstream outMaterial;
	outMaterial.open(outpathAbs, std::ios::out | std::ios::binary);
	outMaterial.write((char*)&exportStruct, sizeof(MaterialExportStruct));
	outMaterial.close();
}


