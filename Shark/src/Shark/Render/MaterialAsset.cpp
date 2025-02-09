#include "skpch.h"
#include "MaterialAsset.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"

namespace Shark {

	  //=//========================================================//=//
	 //=//  Material Asset                                        //=//
	//=//========================================================//=//

	static std::string s_AlbedoMapName = "u_AlbedoMap";
	static std::string s_NormalMapName = "u_NormalMap";
	static std::string s_MetalnessMapName = "u_MetalnessMap";
	static std::string s_RoughnessMapName = "u_RoughnessMap";
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
		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_AlbedoMap); result.Ready)
			m_Material->Set(s_AlbedoMapName, result.Asset);

		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_NormalMap); result.Ready)
			m_Material->Set(s_NormalMapName, result.Asset);

		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_MetalnessMap); result.Ready)
			m_Material->Set(s_MetalnessMapName, result.Asset);

		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_RoughnessMap); result.Ready)
			m_Material->Set(s_RoughnessMapName, result.Asset);
	}

	const std::string& MaterialAsset::GetName() const
	{
		return m_Material->GetName();
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

#if 0
		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_AlbedoMapName, texture);
		});
#endif
	}

	void MaterialAsset::ClearAlbedoMap()
	{
		m_AlbedoMap = AssetHandle::Invalid;
		m_Material->Set(s_AlbedoMapName, Renderer::GetWhiteTexture());
	}

	AssetHandle MaterialAsset::GetNormalMap()
	{
		return m_NormalMap;
	}

	void MaterialAsset::SetNormalMap(AssetHandle handle)
	{
		m_NormalMap = handle;

#if 0
		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_NormalMapName, texture);
		});
#endif
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

#if 0
		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_MetalnessMapName, texture);
		});
#endif
	}

	void MaterialAsset::ClearMetalnessMap()
	{
		m_MetalnessMap = AssetHandle::Invalid;
		m_Material->Set(s_MetalnessMapName, Renderer::GetWhiteTexture());
	}

	AssetHandle MaterialAsset::GetRoughnessMap()
	{
		return m_RoughnessMap;
	}

	void MaterialAsset::SetRoughnessMap(AssetHandle handle)
	{
		m_RoughnessMap = handle;

#if 0
		AssetManager::GetAssetFuture(handle).OnReady([material = m_Material](Ref<Asset> asset)
		{
			Ref<Texture2D> texture = asset.As<Texture2D>();
			material->Set(s_RoughnessMapName, texture);
		});
#endif
	}

	void MaterialAsset::ClearRoughnessMap()
	{
		m_RoughnessMap = AssetHandle::Invalid;
		m_Material->Set(s_RoughnessMapName, Renderer::GetWhiteTexture());
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

	MaterialTable::MaterialTable(uint32_t slots)
		: m_MaterialSlots(slots)
	{
	}

	Ref<MaterialTable> MaterialTable::Clone() const
	{
		auto materialTable = Ref<MaterialTable>::Create();
		materialTable->m_Materials = m_Materials;
		materialTable->m_MaterialSlots = m_MaterialSlots;
		return materialTable;
	}

	void MaterialTable::SetMaterial(uint32_t index, AssetHandle material)
	{
		m_Materials[index] = material;
		if (index >= m_MaterialSlots)
			m_MaterialSlots = index + 1;
	}

	void MaterialTable::ClearMaterial(uint32_t index)
	{
		if (!HasMaterial(index))
			return;
		m_Materials.erase(index);
		if (index >= m_MaterialSlots)
			m_MaterialSlots = index + 1;
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
