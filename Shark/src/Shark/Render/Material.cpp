#include "skpch.h"
#include "Material.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXMaterial.h"

namespace Shark {

	Ref<Material> Material::Create(Ref<Shader> shader)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXMaterial>::Create(shader);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	MaterialAsset::MaterialAsset()
		: m_Material(Material::Create(Renderer::GetShaderLib()->Get("DefaultMeshShader")))
	{
	}

	void MaterialAsset::UpdateMaterial()
	{
		m_Material->SetFloat3("u_PBRData.Albedo", m_AlbedoColor);
		m_Material->SetTexture("u_Albedo", m_UseAlbedo ? ResourceManager::GetAsset<Texture2D>(m_AlbedoTexture) : Renderer::GetWhiteTexture());
		SetDirty(false);
	}

}
