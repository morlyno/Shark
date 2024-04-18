#include "skpch.h"
#include "MaterialAsset.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"

namespace Shark {

	  //=//========================================================//=//
	 //=//  Material Asset                                        //=//
	//=//========================================================//=//

	static std::string s_AlbedoMapUniformName = "u_AlbedoMap";
	static std::string s_NormalMapUniformName = "u_NormalMap";
	static std::string s_MetalnessMapUniformName = "u_MetalnessMap";
	static std::string s_RoughnessMapUniformName = "u_RoughnessMap";
	static std::string s_AlbedoUniformName = "u_MaterialUniforms.Albedo";
	static std::string s_MetalnessUniformName = "u_MaterialUniforms.Metalness";
	static std::string s_RoughnessUniformName = "u_MaterialUniforms.Roughness";
	static std::string s_UsingNormalMapUniformName = "u_MaterialUniforms.UsingNormalMap";

	MaterialAsset::MaterialAsset()
		: MaterialAsset(Material::Create(Renderer::GetShaderLibrary()->Get("SharkPBR")))
	{
	}

	MaterialAsset::MaterialAsset(Ref<Material> material, bool setDefault)
		: m_Material(material)
	{
		if (setDefault)
			SetDefault();
	}

	MaterialAsset::~MaterialAsset()
	{
	}

	void MaterialAsset::Invalidate()
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
		return m_Material->GetVec3(s_AlbedoUniformName);
	}

	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		m_Material->Set(s_AlbedoUniformName, color);
	}

	float& MaterialAsset::GetMetalness()
	{
		return m_Material->GetFloat(s_MetalnessUniformName);
	}

	void MaterialAsset::SetMetalness(float value)
	{
		m_Material->Set(s_MetalnessUniformName, value);
	}

	float& MaterialAsset::GetRoughness()
	{
		return m_Material->GetFloat(s_RoughnessUniformName);
	}

	void MaterialAsset::SetRoughness(float value)
	{
		m_Material->Set(s_RoughnessUniformName, value);
	}

	AssetHandle MaterialAsset::GetAlbedoMap()
	{
		return m_AlbedoMap;
	}

	void MaterialAsset::SetAlbedoMap(AssetHandle handle)
	{
		m_AlbedoMap = handle;

		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_AlbedoMapUniformName, texture);
		});
	}

	void MaterialAsset::ClearAlbedoMap()
	{
		m_AlbedoMap = AssetHandle::Invalid;
		m_Material->Set(s_AlbedoMapUniformName, Renderer::GetWhiteTexture());
	}

	AssetHandle MaterialAsset::GetNormalMap()
	{
		return m_NormalMap;
	}

	void MaterialAsset::SetNormalMap(AssetHandle handle)
	{
		m_NormalMap = handle;

		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_NormalMapUniformName, texture);
		});
	}

	bool MaterialAsset::IsUsingNormalMap()
	{
		return m_Material->GetBool(s_UsingNormalMapUniformName);
	}

	void MaterialAsset::SetUsingNormalMap(bool value)
	{
		m_Material->Set(s_UsingNormalMapUniformName, value);
	}

	void MaterialAsset::ClearNormalMap()
	{
		m_NormalMap = AssetHandle::Invalid;
		m_Material->Set("u_NormalMap", Renderer::GetWhiteTexture());
	}

	AssetHandle MaterialAsset::GetMetalnessMap()
	{
		return m_MetalnessMap;
	}

	void MaterialAsset::SetMetalnessMap(AssetHandle handle)
	{
		m_MetalnessMap = handle;

		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_MetalnessMapUniformName, texture);
		});
	}

	void MaterialAsset::ClearMetalnessMap()
	{
		m_MetalnessMap = AssetHandle::Invalid;
		m_Material->Set(s_MetalnessMapUniformName, Renderer::GetWhiteTexture());
	}

	AssetHandle MaterialAsset::GetRoughnessMap()
	{
		return m_RoughnessMap;
	}

	void MaterialAsset::SetRoughnessMap(AssetHandle handle)
	{
		m_RoughnessMap = handle;

		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_RoughnessMapUniformName, texture);
		});
	}

	void MaterialAsset::ClearRoughnessMap()
	{
		m_RoughnessMap = AssetHandle::Invalid;
		m_Material->Set(s_RoughnessMapUniformName, Renderer::GetWhiteTexture());
	}

	void MaterialAsset::SetDefault()
	{
		m_Material->Set(s_AlbedoUniformName, glm::vec3(0.8f));
		m_Material->Set(s_MetalnessUniformName, 0.0f);
		m_Material->Set(s_RoughnessUniformName, 0.5f);
		m_Material->Set(s_UsingNormalMapUniformName, false);
		ClearAlbedoMap();
		ClearNormalMap();
		ClearMetalnessMap();
		ClearRoughnessMap();
	}

	  //=//========================================================//=//
	 //=//  Material Table                                        //=//
	//=//========================================================//=//

	MaterialTable::MaterialTable(uint32_t count)
		: m_MaterialCount(count)
	{
	}

	void MaterialTable::SetMaterial(uint32_t index, AssetHandle material)
	{
		m_Materials[index] = material;
	}

	void MaterialTable::ClearMaterial(uint32_t index)
	{
		if (!HasMaterial(index))
			return;
		m_Materials.erase(index);
	}

	AssetHandle MaterialTable::GetMaterial(uint32_t index) const
	{
		SK_CORE_VERIFY(HasMaterial(index));
		return m_Materials.at(index);
	}

	void MaterialTable::Clear()
	{
		m_Materials.clear();
	}

}
