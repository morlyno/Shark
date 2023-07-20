#pragma once

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Material.h"

namespace Shark {

	class MaterialTable : public RefCount
	{
	public:
		bool HasMaterial(uint32_t index) const
		{
			return m_Materials.find(index) != m_Materials.end();
		}

		Ref<Material> GetMaterial(uint32_t index) const
		{
			return m_Materials.at(index);
		}

		void AddMaterial(uint32_t index, Ref<Material> material)
		{
			SK_CORE_ASSERT(!HasMaterial(index));
			m_Materials[index] = material;
		}

	private:
		std::map<uint32_t, Ref<Material>> m_Materials;
	};

	class Mesh : public RefCount
	{
	public:
		struct SubMesh
		{
			uint32_t IndexCount = 0;
			uint32_t BaseIndex = 0;
			uint32_t BaseVertex = 0;
			uint32_t MaterialIndex = 0;
		};

		struct Node
		{
			bool HasMesh = false;
			uint32_t MeshIndex = 0;
			Node* Parent = nullptr;
			std::vector<Node> Children;
			glm::mat4 Transform;
			std::string Name;
		};

	public:
		Mesh() = default;
		~Mesh() = default;

		Ref<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		Ref<MaterialTable> GetMaterialTable() const { return m_MaterialTable; }
		const std::vector<SubMesh>& GetSubmeshes() const { return m_SubMeshes; }
		uint32_t GetSubmeshCount() const { return m_SubMeshes.size(); }

		const Node& GetRootNode() const { return m_RootNode; }
		Node& GetRootNode() { return m_RootNode; }

	private:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		Ref<MaterialTable> m_MaterialTable;

		std::vector<SubMesh> m_SubMeshes;

		Node m_RootNode;

		friend class AssimpImporter;
	};

}
