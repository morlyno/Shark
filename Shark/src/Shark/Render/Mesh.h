#pragma once

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Material.h"

namespace Shark {

	class Mesh : public RefCount
	{
	public:
		Mesh(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, Ref<Material> material)
			: m_VertexBuffer(vertexBuffer), m_IndexBuffer(indexBuffer), m_Material(material)
		{}

		Ref<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		Ref<Material> GetMaterial() const { return m_Material; }

	public:
		static Ref<Mesh> Create(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, Ref<Material> material)
		{
			return Ref<Mesh>::Create(vertexBuffer, indexBuffer, material);
		}

	private:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;

		Ref<Material> m_Material;
	};

}
