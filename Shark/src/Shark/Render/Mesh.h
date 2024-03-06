#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Render/MaterialAsset.h"

namespace Shark {

	class Mesh : public Asset
	{
	public:
		Mesh() = default;
		Mesh(Ref<MeshSource> meshSource);
		Mesh(Ref<MeshSource> meshSource, Ref<MaterialTable> materials, const std::vector<uint32_t>& submeshes);
		virtual ~Mesh() = default;

		Ref<MeshSource> GetMeshSource() const { return m_MeshSource; }
		void SetMeshSource(Ref<MeshSource> meshSource) { m_MeshSource = meshSource; }

		std::vector<uint32_t>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }
		void SetSubmeshes(const std::vector<uint32_t>& submeshes) { m_Submeshes = submeshes; }

		Ref<MaterialTable> GetMaterialTable() const { return m_MaterialTable; }

	public:
		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		static Ref<Mesh> Create(Ref<MeshSource> meshSource);
		static Ref<Mesh> Create(Ref<MeshSource> meshSource, Ref<MaterialTable> materials, const std::vector<uint32_t>& submeshes);

	private:
		void InitMaterials();

	public:
		Ref<MeshSource> m_MeshSource;
		std::vector<uint32_t> m_Submeshes;
		Ref<MaterialTable> m_MaterialTable;

		friend class MeshSerializer;
	};

}
