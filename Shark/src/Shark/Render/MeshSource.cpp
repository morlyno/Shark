#include "skpch.h"
#include "MeshSource.h"

namespace Shark {

	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices)
		: m_Vertices(vertices), m_Indices(indices)
	{
		Submesh submesh;
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.IndexCount = indices.size() * 3;
		submesh.VertexCount = vertices.size();
		m_Submeshes.push_back(submesh);

		m_VertexBuffer = VertexBuffer::Create(Buffer::FromArray(vertices));
		m_IndexBuffer = IndexBuffer::Create(Buffer::FromArray(indices));
	}

	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes)
		: m_Vertices(vertices), m_Indices(indices), m_Submeshes(submeshes)
	{
		m_VertexBuffer = VertexBuffer::Create(Buffer::FromArray(vertices));
		m_IndexBuffer = IndexBuffer::Create(Buffer::FromArray(indices));
	}

}
