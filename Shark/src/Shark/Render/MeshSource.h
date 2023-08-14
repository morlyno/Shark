#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Material.h"

namespace Shark {

	class MeshSource : public Asset
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
			std::string Name;
			glm::mat4 Transform = glm::identity<glm::mat4>();
			std::vector<uint32_t> MeshIndices;
			Node* Parent = nullptr;
			std::vector<Node> Children;
		};

	public:
		MeshSource() = default;
		MeshSource(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, Ref<MaterialTable> materialTable);
		virtual ~MeshSource() = default;

		const Ref<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		const Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		const Ref<MaterialTable> GetMaterialTable() const { return m_MaterialTable; }
		const std::vector<SubMesh>& GetSubmeshes() const { return m_SubMeshes; }
		uint32_t GetSubmeshCount() const { return m_SubMeshes.size(); }
		const Node& GetRootNode() const { return m_RootNode; }

	public:
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::MeshSource; }

		static Ref<MeshSource> Create() { return Ref<MeshSource>::Create(); }
		static Ref<MeshSource> Create(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, Ref<MaterialTable> materialTable) { return Ref<MeshSource>::Create(vertexBuffer, indexBuffer, materialTable); }

	private:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		Ref<MaterialTable> m_MaterialTable;
		std::vector<SubMesh> m_SubMeshes;
		Node m_RootNode;

		friend class MeshSourceSerializer;
	};

}
