#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Render/MaterialAsset.h"

#include <span>

namespace Shark {

	class Mesh : public Asset
	{
	public:
		Mesh(AssetHandle meshSource);
		Mesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes);
		virtual ~Mesh() = default;

		AssetHandle GetMeshSource() const { return m_MeshSource; }
		Ref<MaterialTable> GetMaterials() const { return m_MaterialTable; }

		std::vector<uint32_t>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }
		void SetSubmeshes(std::span<const uint32_t> submeshes, Ref<MeshSource> meshSource);

	public:
		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		AssetHandle m_MeshSource;
		std::vector<uint32_t> m_Submeshes;
		Ref<MaterialTable> m_MaterialTable;

		friend class MeshSerializer;
	};

}
