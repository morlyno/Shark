#include "skpch.h"
#include "MeshSource.h"

namespace Shark {

	MeshSource::MeshSource(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, Ref<MaterialTable> materialTable)
	{
		m_VertexBuffer = vertexBuffer;
		m_IndexBuffer = indexBuffer;
		m_MaterialTable = materialTable;
		auto& submesh = m_SubMeshes.emplace_back();
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.IndexCount = indexBuffer->GetCount();
		submesh.MaterialIndex = 0;
		m_RootNode.Name = "Root";
		m_RootNode.MeshIndices.emplace_back(0);
	}

	uint32_t MeshSource::GetSubmeshVertexCount(uint32_t submeshIndex) const
	{
		if (submeshIndex >= m_SubMeshes.size())
			return 0;

		const auto& submesh = m_SubMeshes[submeshIndex];
		if ((submeshIndex + 1) == m_SubMeshes.size())
			return m_VertexBuffer->GetVertexCount() - submesh.BaseVertex;

		const auto& nextSubmesh = m_SubMeshes[submeshIndex + 1];
		return nextSubmesh.BaseVertex - submesh.BaseVertex;
	}

}
