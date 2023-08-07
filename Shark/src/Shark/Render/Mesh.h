#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Material.h"
#include "Shark/Render/MeshSource.h"

namespace Shark {

	class Mesh : public Asset
	{
	public:
		Mesh() = default;
		virtual ~Mesh() = default;

		Ref<MeshSource> GetMeshSource() const { return m_MeshSource; }
		Ref<MaterialTable> GetMaterialTable() const { return m_MaterialTable; }
		std::vector<uint32_t> GetSubmeshIndices() const { return m_SubmeshIndices; }
		
	public:
		Ref<MeshSource> m_MeshSource;
		Ref<MaterialTable> m_MaterialTable;
		std::vector<uint32_t> m_SubmeshIndices;

		friend class MeshSerializer;
	};

}
