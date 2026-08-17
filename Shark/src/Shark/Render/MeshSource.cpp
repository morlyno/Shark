#include "skpch.h"
#include "MeshSource.h"

#include "Shark/Animation/Skeleton.h"
#include "Shark/Animation/Animation.h"

namespace Shark {

	MeshSource::MeshSource()
	{

	}

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

	MeshSource::~MeshSource()
	{

	}

	bool MeshSource::HasSkeleton() const
	{
		return m_Skeleton != nullptr;
	}

	const Skeleton& MeshSource::GetSkeleton() const
	{
		return *m_Skeleton;
	}

	Animation& MeshSource::GetAnimation(size_t index)
	{
		return *m_Animations[index];
	}

	const Animation& MeshSource::GetAnimation(size_t index) const
	{
		return *m_Animations[index];
	}

	std::optional<size_t> MeshSource::FindAnimation(std::string_view animationName) const
	{
		for (size_t i = 0; i < m_AnimationNames.size(); i++)
			if (m_AnimationNames[i] == animationName)
				return i;
		return std::nullopt;
	}

}
