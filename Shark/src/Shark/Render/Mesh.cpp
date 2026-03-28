#include "skpch.h"
#include "Mesh.h"

#include "Shark/Asset/AssetManager.h"

namespace Shark {

	Mesh::Mesh(Ref<MeshSource> meshSource)
		: m_MeshSource(meshSource->Handle)
	{
		InitializeFromThis(meshSource);
	}

#if TODO

	void Mesh::SetSubmeshes(std::span<const uint32_t> submeshes, Ref<MeshSource> meshSource)
	{
		if (!submeshes.empty())
		{
			m_Submeshes = { submeshes.begin(), submeshes.end() };
			return;
		}

		if (auto meshSource = AssetManager::GetAsset<MeshSource>(m_MeshSource))
		{
			const auto& submeshes = meshSource->GetSubmeshes();
			m_Submeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < m_Submeshes.size(); i++)
				m_Submeshes[i] = i;
		}
	}
#endif

	void Mesh::InitializeFromThis(Ref<MeshSource> meshSource)
	{
		SK_CORE_VERIFY(m_MeshSource == meshSource->Handle);
		m_MaterialTable = Ref<MaterialTable>::Create();

		if (m_Submeshes.empty())
		{
			const auto& submeshes = meshSource->GetSubmeshes();
			m_Submeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < m_Submeshes.size(); i++)
				m_Submeshes[i] = i;
		}

		const auto& meshMaterials = meshSource->GetMaterials();
		for (uint32_t i = 0; i < meshMaterials.size(); i++)
			m_MaterialTable->SetMaterial(i, meshMaterials[i]);
	}

}
