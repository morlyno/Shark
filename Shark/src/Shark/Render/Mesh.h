#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Material.h"
#include "Shark/Render/MeshSource.h"

namespace Shark {

	class Mesh : public Asset
	{
	public:
		Mesh() = default;
		Mesh(Ref<MeshSource> meshSource)
			: m_MeshSource(meshSource), m_MaterialTable(Ref<MaterialTable>::Create()), m_SubmeshIndices({})
		{}
		Mesh(Ref<MeshSource> meshSource, Ref<MaterialTable> materialTable, const std::vector<uint32_t> submeshIndices)
			: m_MeshSource(meshSource), m_MaterialTable(materialTable), m_SubmeshIndices(submeshIndices)
		{}
		virtual ~Mesh() = default;

		Ref<MeshSource> GetMeshSource() const { return m_MeshSource; }
		Ref<MaterialTable> GetMaterialTable() const { return m_MaterialTable; }
		std::vector<uint32_t> GetSubmeshIndices() const { return m_SubmeshIndices; }

	public:
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::Mesh; }

		static Ref<Mesh> Create() { return Ref<Mesh>::Create(); }
		static Ref<Mesh> Create(Ref<MeshSource> meshSource) { return Ref<Mesh>::Create(meshSource); }
		static Ref<Mesh> Create(Ref<MeshSource> meshSource, Ref<MaterialTable> materialTable, const std::vector<uint32_t> submeshIndices) { return Ref<Mesh>::Create(meshSource, materialTable, submeshIndices); }

	public:
		Ref<MeshSource> m_MeshSource;
		Ref<MaterialTable> m_MaterialTable;
		std::vector<uint32_t> m_SubmeshIndices;

		friend class MeshSerializer;
	};

}
