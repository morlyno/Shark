#include "skpch.h"
#include "MaterialAsset.h"

#include "Shark/Render/Renderer.h"

namespace Shark {

	  //=//========================================================//=//
	 //=//  Material Asset                                        //=//
	//=//========================================================//=//

	MaterialAsset::MaterialAsset(Ref<Material> material)
		: m_Material(material)
	{
		SetDefault();
	}

	MaterialAsset::~MaterialAsset()
	{
	}

	Ref<Material> MaterialAsset::GetMaterial()
	{
		return m_Material;
	}

	void MaterialAsset::SetMaterial(Ref<Material> material)
	{
		m_Material = material;
	}

	glm::vec3& MaterialAsset::GetAlbedoColor()
	{
		return m_Material->GetVec3("u_MaterialUniforms.Albedo");
	}

	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		m_Material->Set("u_MaterialUniforms.Albedo", color);
	}

	float& MaterialAsset::GetMetalness()
	{
		return m_Material->GetFloat("u_MaterialUniforms.Metalness");
	}

	void MaterialAsset::SetMetalness(float value)
	{
		m_Material->Set("u_MaterialUniforms.Metalness", value);
	}

	float& MaterialAsset::GetRoughness()
	{
		return m_Material->GetFloat("u_MaterialUniforms.Roughness");
	}

	void MaterialAsset::SetRoughness(float value)
	{
		m_Material->Set("u_MaterialUniforms.Roughness", value);
	}

	float& MaterialAsset::GetAmbientOcclusion()
	{
		return m_Material->GetFloat("u_MaterialUniforms.AmbientOcclusion");
	}

	void MaterialAsset::SetAmbientOcclusion(float value)
	{
		m_Material->Set("u_MaterialUniforms.AmbientOcclusion", value);
	}

	Ref<Texture2D> MaterialAsset::GetAlbedoMap()
	{
		return m_Material->GetTexture("u_AlbedoMap");
	}

	void MaterialAsset::SetAlbedoMap(Ref<Texture2D> texture)
	{
		SK_CORE_VERIFY(texture);
		m_Material->Set("u_AlbedoMap", texture);
	}

	void MaterialAsset::ClearAlbedoMap()
	{
		m_Material->Set("u_AlbedoMap", Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetNormalMap()
	{
		return m_Material->GetTexture("u_NormalMap");
	}

	void MaterialAsset::SetNormalMap(Ref<Texture2D> texture)
	{
		SK_CORE_VERIFY(texture);
		m_Material->Set("u_NormalMap", texture);
	}

	bool MaterialAsset::IsUsingNormalMap()
	{
		return m_Material->GetBool("u_MaterialUniforms.UsingNormalMap");
	}

	void MaterialAsset::SetUsingNormalMap(bool value)
	{
		m_Material->Set("u_MaterialUniforms.UsingNormalMap", value);
	}

	void MaterialAsset::ClearNormalMap()
	{
		m_Material->Set("u_NormalMap", Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetMetalnessMap()
	{
		return m_Material->GetTexture("u_MetalnessMap");
	}

	void MaterialAsset::SetMetalnessMap(Ref<Texture2D> texture)
	{
		m_Material->Set("u_MetalnessMap", texture);
	}

	void MaterialAsset::ClearMetalnessMap()
	{
		m_Material->Set("u_MetalnessMap", Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetRoughnessMap()
	{
		return m_Material->GetTexture("u_RoughnessMap");
	}

	void MaterialAsset::SetRoughnessMap(Ref<Texture2D> texture)
	{
		m_Material->Set("u_RoughnessMap", texture);
	}

	void MaterialAsset::ClearRoughnessMap()
	{
		m_Material->Set("u_RoughnessMap", Renderer::GetWhiteTexture());
	}

	void MaterialAsset::SetDefault()
	{
		m_Material->Set("u_MaterialUniforms.Albedo", glm::vec3(0.8f));
		m_Material->Set("u_MaterialUniforms.Metalness", 0.0f);
		m_Material->Set("u_MaterialUniforms.Roughness", 0.5f);
		m_Material->Set("u_MaterialUniforms.AmbientOcclusion", 0.0f);
		m_Material->Set("u_MaterialUniforms.UsingNormalMap", false);
		ClearAlbedoMap();
		ClearNormalMap();
		ClearMetalnessMap();
		ClearRoughnessMap();
	}

	  //=//========================================================//=//
	 //=//  Material Table                                        //=//
	//=//========================================================//=//


	void MaterialTable::SetMaterial(uint32_t index, Ref<MaterialAsset> material)
	{
		m_Materials[index] = material;
	}

	void MaterialTable::ClearMaterial(uint32_t index)
	{
		if (!HasMaterial(index))
			return;
		m_Materials.erase(index);
	}

	Ref<MaterialAsset> MaterialTable::GetMaterial(uint32_t index) const
	{
		SK_CORE_VERIFY(HasMaterial(index));
		return m_Materials.at(index);
	}

	void MaterialTable::Clear()
	{
		m_Materials.clear();
	}

}
