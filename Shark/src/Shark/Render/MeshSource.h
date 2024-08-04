#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/MaterialAsset.h"
#include "Shark/Math/AABB.h"

namespace Shark {

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;
		glm::vec2 Texcoord;
	};

	struct Index
	{
		uint32_t Vertex1;
		uint32_t Vertex2;
		uint32_t Vertex3;
	};

	struct Submesh
	{
		uint32_t BaseVertex = 0;
		uint32_t BaseIndex = 0;
		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		uint32_t MaterialIndex = 0;
		AABB BoundingBox;
		std::string MeshName;
	};

	struct MeshNode
	{
		uint32_t Parent = (uint32_t)-1;
		std::vector<uint32_t> Children;
		std::vector<uint32_t> Submeshes;

		std::string Name;
		glm::mat4 Transform;
		glm::mat4 LocalTransform;

		bool IsRoot() const { return Parent == (uint32_t)-1; }
	};

	class MeshSource : public Asset
	{
	public:
		MeshSource() = default;
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices);
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes);
		virtual ~MeshSource() = default;

		const std::string& GetName() const { return m_Name; }

		std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		const std::vector<Index>& GetIndices() const { return m_Indices; }

		std::vector<AssetHandle>& GetMaterials() { return m_Materials; }
		const std::vector<AssetHandle>& GetMaterials() const { return m_Materials; }

		Ref<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }

		const MeshNode& GetRootNode() const { return m_Nodes[0]; }
		const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }

		const AABB& GetBoundingBox() const { return m_BoundingBox; }

	public:
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::MeshSource; }

		static Ref<MeshSource> Create() { return Ref<MeshSource>::Create(); }
		static Ref<MeshSource> Create(const std::vector<Vertex>& vertices, const std::vector<Index>& indices) { return Ref<MeshSource>::Create(vertices, indices); }
		static Ref<MeshSource> Create(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes) { return Ref<MeshSource>::Create(vertices, indices, submeshes); }

	private:
		std::string m_Name;
		std::vector<Vertex> m_Vertices;
		std::vector<Index> m_Indices;

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		std::vector<AssetHandle> m_Materials;

		std::vector<Submesh> m_Submeshes;
		std::vector<MeshNode> m_Nodes;

		AABB m_BoundingBox;

		friend class MeshSourceSerializer;
		friend class AssimpMeshImporter;
	};

}
