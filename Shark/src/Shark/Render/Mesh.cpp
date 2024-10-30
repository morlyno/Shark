#include "skpch.h"
#include "Mesh.h"

#include "Shark/Asset/AssetManager.h"

namespace Shark {

	Mesh::Mesh(AssetHandle meshSource)
		: m_MeshSource(meshSource)
	{
		m_MaterialTable = Ref<MaterialTable>::Create();

		if (auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource))
		{
			SetSubmeshes({}, meshSourceAsset);

			const auto& meshMaterials = meshSourceAsset->GetMaterials();
			for (uint32_t i = 0; i < meshMaterials.size(); i++)
				m_MaterialTable->SetMaterial(i, meshMaterials[i]);
		}
	}

	Mesh::Mesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes)
		: m_MeshSource(meshSource)
	{
		m_MaterialTable = Ref<MaterialTable>::Create();

		if (auto meshSourceAsset = AssetManager::GetAsset<MeshSource>(meshSource))
		{
			SetSubmeshes(submeshes, meshSourceAsset);

			const auto& meshMaterials = meshSourceAsset->GetMaterials();
			for (uint32_t i = 0; i < meshMaterials.size(); i++)
				m_MaterialTable->SetMaterial(i, meshMaterials[i]);
		}
	}

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

}
