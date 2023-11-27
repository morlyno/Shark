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
		return m_Material->GetVec3("u_PBR.Albedo");
	}

	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		m_Material->Set("u_PBR.Albedo", color);
	}

	float& MaterialAsset::GetMetallic()
	{
		return m_Material->GetFloat("u_PBR.Metallic");
	}

	void MaterialAsset::SetMetallic(float value)
	{
		m_Material->Set("u_PBR.Metallic", value);
	}

	float& MaterialAsset::GetRoughness()
	{
		return m_Material->GetFloat("u_PBR.Roughness");
	}

	void MaterialAsset::SetRoughness(float value)
	{
		m_Material->Set("u_PBR.Roughness", value);
	}

	float& MaterialAsset::GetAmbientOcclusion()
	{
		return m_Material->GetFloat("u_PBR.AO");
	}

	void MaterialAsset::SetAmbientOcclusion(float value)
	{
		m_Material->Set("u_PBR.AO", value);
	}

	Ref<Texture2D> MaterialAsset::GetAlbedoMap()
	{
		return m_Material->GetTexture("u_Albedo");
	}

	void MaterialAsset::SetAlbedoMap(Ref<Texture2D> texture)
	{
		m_Material->Set("u_Albedo", texture);
	}

	bool MaterialAsset::UsingAlbedoMap()
	{
		return true;
	}

	void MaterialAsset::SetUsingAlbedoMap(bool value)
	{
		SK_CORE_WARN("MaterialAsset::SetUsingAlbedoMap not implemented!");
	}

	void MaterialAsset::ClearAlbedoMap()
	{
		m_Material->Set("u_Albedo", Renderer::GetWhiteTexture());
	}

	void MaterialAsset::SetDefault()
	{
		m_Material->Set("u_PBR.Albedo", glm::vec3(0.8f));
		m_Material->Set("u_PBR.Metallic", 0.0f);
		m_Material->Set("u_PBR.Roughness", 0.5f);
		m_Material->Set("u_PBR.AO", 0.0f);
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
