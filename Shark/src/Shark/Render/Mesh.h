#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {
	class MeshSource;
	class MaterialTable;
}

namespace Shark {

	class Mesh : public Asset
	{
	public:
		Mesh();
		Mesh(Ref<MeshSource> meshSource);
		~Mesh();

		AssetHandle GetMeshSource() const { return m_MeshSource; }
		RefArg<MaterialTable> GetMaterials() const { return m_MaterialTable; }

#if TODO
		std::vector<uint32_t>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return m_Submeshes; }
		void SetSubmeshes(std::span<const uint32_t> submeshes, Ref<MeshSource> meshSource);
#endif

	public:
		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		void InitializeFromThis(Ref<MeshSource> meshSource);

	private:
		AssetHandle m_MeshSource;
		std::vector<uint32_t> m_Submeshes;
		Ref<MaterialTable> m_MaterialTable;

		friend class MeshSerializer;
	};

}
