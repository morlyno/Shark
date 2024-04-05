#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Render/MaterialAsset.h"

namespace Shark {

	class Mesh : public Asset
	{
	public:
		Mesh() = default;
		Mesh(AssetHandle meshSource);
		Mesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes);
		virtual ~Mesh() = default;

		AssetHandle GetMeshSource() const { return m_MeshSource; }
		void SetMeshSource(AssetHandle meshSource) { m_MeshSource = meshSource; }

		std::vector<uint32_t>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }
		void SetSubmeshes(const std::vector<uint32_t>& submeshes) { m_Submeshes = submeshes; }

	public:
		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		AssetHandle m_MeshSource;
		std::vector<uint32_t> m_Submeshes;

		friend class MeshSerializer;
	};

}
