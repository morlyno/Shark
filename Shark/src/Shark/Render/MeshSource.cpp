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

}
