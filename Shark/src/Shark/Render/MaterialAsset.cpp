#include "skpch.h"
#include "MaterialAsset.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Core/Memory.h"

namespace Shark {

	  //=//========================================================//=//
	 //=//  PBR Material                                          //=//
	//=//========================================================//=//

	static const std::string s_AlbedoMapName = "u_AlbedoMap";
	static const std::string s_NormalMapName = "u_NormalMap";
	static const std::string s_MetalnessMapName = "u_MetalnessMap";
	static const std::string s_RoughnessMapName = "u_RoughnessMap";
	static const std::string s_MaterialUniformsName = "u_MaterialUniforms";
	static const std::string s_AlbedoUniformName = "u_MaterialUniforms.Albedo";
	static const std::string s_MetalnessUniformName = "u_MaterialUniforms.Metalness";
	static const std::string s_RoughnessUniformName = "u_MaterialUniforms.Roughness";
	static const std::string s_UsingNormalMapUniformName = "u_MaterialUniforms.UsingNormalMap";

	PBRMaterial::PBRMaterial(const std::string& name, bool setDefault, bool bakeMaterial)
	{
		SK_CORE_VERIFY(bakeMaterial ? setDefault : true);
		auto shader = Renderer::GetShaderLibrary()->Get("SharkPBR");
		m_Material = Material::Create(shader, name);
		
		Memory::WriteZero(m_Uniforms);
		Memory::WriteZero(m_ActiveState);

		if (setDefault)
			SetDefaults();

		if (bakeMaterial)
			m_Material->Bake();
	}

	PBRMaterial::~PBRMaterial()
	{

	}

	void PBRMaterial::SetDefaults()
	{
		m_Uniforms.Albedo = glm::vec3(0.8f);
		m_Uniforms.Metalness = 0.0f;
		m_Uniforms.Roughness = 0.5f;
		m_Uniforms.UsingNormalMap = false;
		ClearAlbedoMap();
		ClearNormalMap();
		ClearMetalnessMap();
		ClearRoughnessMap();
	}

	void PBRMaterial::Bake()
	{
		SK_CORE_VERIFY(m_Material->Validate());
		m_Material->Bake();
	}

	void PBRMaterial::MT_Bake()
	{
		SK_CORE_VERIFY(m_Material->Validate());
		m_Material->MT_Bake();
	}

	void PBRMaterial::Update()
	{
		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_AlbedoMap); result.Ready)
			m_Material->Set(s_AlbedoMapName, result.Asset);

		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_NormalMap); result.Ready)
			m_Material->Set(s_NormalMapName, result.Asset);

		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_MetalnessMap); result.Ready)
			m_Material->Set(s_MetalnessMapName, result.Asset);

		if (AsyncLoadResult result = AssetManager::GetAssetAsync<Texture2D>(m_RoughnessMap); result.Ready)
			m_Material->Set(s_RoughnessMapName, result.Asset);

		if (m_Uniforms != m_ActiveState)
		{
			m_ActiveState = m_Uniforms;
			m_Material->Set(s_MaterialUniformsName, Buffer::FromValue(m_ActiveState));
		}

		m_Material->Update();
	}

	AssetHandle PBRMaterial::GetAlbedoMap()
	{
		return m_AlbedoMap;
	}

	void PBRMaterial::SetAlbedoMap(AssetHandle handle)
	{
		m_AlbedoMap = handle;
	}

	void PBRMaterial::ClearAlbedoMap()
	{
		m_AlbedoMap = AssetHandle::Invalid;
		m_Material->Set(s_AlbedoMapName, Renderer::GetWhiteTexture());
	}

	glm::vec3& PBRMaterial::GetAlbedoColor()
	{
		return m_Uniforms.Albedo;
	}

	void PBRMaterial::SetAlbedoColor(const glm::vec3& color)
	{
		m_Uniforms.Albedo = color;
	}

	AssetHandle PBRMaterial::GetNormalMap()
	{
		return m_NormalMap;
	}

	void PBRMaterial::SetNormalMap(AssetHandle handle)
	{
		m_NormalMap = handle;
	}

	void PBRMaterial::ClearNormalMap()
	{
		m_NormalMap = AssetHandle::Invalid;
		m_Material->Set(s_NormalMapName, Renderer::GetWhiteTexture());
	}

	bool PBRMaterial::IsUsingNormalMap()
	{
		return m_Uniforms.UsingNormalMap;
	}

	void PBRMaterial::SetUsingNormalMap(bool value)
	{
		m_Uniforms.UsingNormalMap = value;
	}

	AssetHandle PBRMaterial::GetMetalnessMap()
	{
		return m_MetalnessMap;
	}

	void PBRMaterial::SetMetalnessMap(AssetHandle handle)
	{
		m_MetalnessMap = handle;
	}

	void PBRMaterial::ClearMetalnessMap()
	{
		m_MetalnessMap = AssetHandle::Invalid;
		m_Material->Set(s_MetalnessMapName, Renderer::GetWhiteTexture());
	}

	float& PBRMaterial::GetMetalness()
	{
		return m_Uniforms.Metalness;
	}

	void PBRMaterial::SetMetalness(float value)
	{
		m_Uniforms.Metalness = value;
	}

	AssetHandle PBRMaterial::GetRoughnessMap()
	{
		return m_RoughnessMap;
	}

	void PBRMaterial::SetRoughnessMap(AssetHandle handle)
	{
		m_RoughnessMap = handle;
	}

	void PBRMaterial::ClearRoughnessMap()
	{
		m_RoughnessMap = AssetHandle::Invalid;
		m_Material->Set(s_RoughnessMapName, Renderer::GetWhiteTexture());
	}

	float& PBRMaterial::GetRoughness()
	{
		return m_Uniforms.Roughness;
	}

	void PBRMaterial::SetRoughness(float value)
	{
		m_Uniforms.Roughness = value;
	}


	  //=//========================================================//=//
	 //=//  Material Table                                        //=//
	//=//========================================================//=//

	MaterialTable::MaterialTable(uint32_t slots)
		: m_MaterialSlots(slots)
	{
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
