#include "skpch.h"
#include "Mesh.h"

#include "Shark/Asset/AssetManager.h"

namespace Shark {

	Mesh::Mesh(Ref<MeshSource> meshSource)
		: m_MeshSource(meshSource)
	{
		InitMaterials();
	}

	Mesh::Mesh(Ref<MeshSource> meshSource, Ref<MaterialTable> materials, const std::vector<uint32_t>& submeshes)
		: m_MeshSource(meshSource), m_Submeshes(submeshes)
	{
		InitMaterials();
	}

	Ref<Mesh> Mesh::Create(Ref<MeshSource> meshSource)
	{
		return Ref<Mesh>::Create(meshSource);
	}

	Ref<Mesh> Mesh::Create(Ref<MeshSource> meshSource, Ref<MaterialTable> materials, const std::vector<uint32_t>& submeshes)
	{
		return Ref<Mesh>::Create(meshSource, materials, submeshes);
	}

	void Mesh::InitMaterials()
	{
		m_MaterialTable = Ref<MaterialTable>::Create();
		const auto& materials = m_MeshSource->GetMaterials();

		uint32_t materialIndex = 0;
		for (const auto& material : materials)
		{
			AssetHandle materialHandle = AssetManager::CreateMemoryAsset<MaterialAsset>(material, false);
			m_MaterialTable->SetMaterial(materialIndex++, materialHandle);
		}
	}

}
