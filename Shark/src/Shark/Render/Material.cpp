#include "skpch.h"
#include "Material.h"

#include "Shark/Asset/AssetManager.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXMaterial.h"

namespace Shark {

	Ref<Material> Material::Create(Ref<Shader> shader)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXMaterial>::Create(shader);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	MaterialAsset::MaterialAsset()
		: m_Material(Material::Create(Renderer::GetShaderLibrary()->Get("SharkPBR")))
	{
	}

	void MaterialAsset::UpdateMaterial()
	{
		m_Material->SetFloat3("u_PBR.Albedo", m_AlbedoColor);
		m_Material->SetTexture("u_Albedo", m_UseAlbedo ? AssetManager::GetAsset<Texture2D>(m_AlbedoTexture) : Renderer::GetWhiteTexture());
		m_Material->SetFloat("u_PBR.Metallic", m_Metallic);
		m_Material->SetFloat("u_PBR.Roughness", m_Roughness);
		m_Material->SetFloat("u_PBR.AO", m_AO);
		SetDirty(false);
	}

}
