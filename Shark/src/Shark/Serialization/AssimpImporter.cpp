#include "skpch.h"
#include "AssimpImporter.h"

#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace Shark {

	namespace utils {

		static glm::mat4 AssimpMatrixToGLM(aiMatrix4x4 matrix)
		{
			glm::mat4 result;
			for (uint32_t i = 0; i < 4; i++)
				for (uint32_t j = 0; j < 4; j++)
					result[i][j] = matrix[i][j];
			result = glm::transpose(result);
			return result;
		}

	}

	Ref<Mesh> AssimpImporter::TryLoad(const std::filesystem::path& filePath)
	{
		if (!FileSystem::Exists(filePath))
			return nullptr;

#if 0
		std::string content = FileSystem::ReadString(filePath);

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFileFromMemory(content.c_str(), content.size(), aiProcess_Triangulate | /*aiProcess_JoinIdenticalVertices | */aiProcess_ConvertToLeftHanded);
#else
		Assimp::Importer importer;
		auto fsFilePath = FileSystem::GetAbsolute(filePath).string();
		const aiScene* scene = importer.ReadFile(fsFilePath, aiProcess_Triangulate | /*aiProcess_JoinIdenticalVertices | */aiProcess_ConvertToLeftHanded);
#endif

		if (!scene)
			return nullptr;

		VertexLayout layout = {
			{ VertexDataType::Float3, "Position" }
		};

#if 0
		aiMesh* mesh = scene->mMeshes[0];
		std::vector<glm::vec3> vertices;
		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			aiVector3D& vertex = mesh->mVertices[i];
			vertices.push_back(glm::vec3{ vertex.x, vertex.y, vertex.z });
		}

		std::vector<uint32_t> indices;
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			SK_CORE_ASSERT(face.mNumIndices == 3);
			indices.push_back(face.mIndices[0]);
			indices.push_back(face.mIndices[1]);
			indices.push_back(face.mIndices[2]);
		}

		auto vertexBuffer = VertexBuffer::Create(layout, Buffer::FromArray(vertices));
		auto indexBuffer = IndexBuffer::Create(Buffer::FromArray(indices));
		auto material = Material::Create(Renderer::GetShaderLib()->Get("DefaultMeshShader"));

		return Mesh::Create(vertexBuffer, indexBuffer, material);
#endif

		Buffer vertices;
		Buffer indices;
		std::vector<Mesh::SubMesh> submeshes;
		Ref<MaterialTable> materialTable = Ref<MaterialTable>::Create();

		{
			uint64_t vertexOffset = 0;
			uint64_t indexOffset = 0;

			for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
			{
				auto& submesh = submeshes.emplace_back();

				aiMesh* mesh = scene->mMeshes[meshIndex];
				vertices.Grow<glm::vec3>(mesh->mNumVertices);
				vertices.Write(mesh->mVertices, mesh->mNumVertices * sizeof(glm::vec3), vertexOffset * sizeof(glm::vec3));
				submesh.BaseVertex = vertexOffset;
				submesh.BaseIndex = indexOffset;
				submesh.IndexCount = mesh->mNumFaces * 3;
				submesh.MaterialIndex = meshIndex;
				materialTable->AddMaterial(meshIndex, Material::Create(Renderer::GetShaderLib()->Get("DefaultMeshShader")));

				vertexOffset += mesh->mNumVertices;

				indices.Grow<uint32_t>(submesh.IndexCount);
				for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
				{
					aiFace& face = mesh->mFaces[faceIndex];
					SK_CORE_ASSERT(face.mNumIndices == 3);
					indices.Write(face.mIndices, face.mNumIndices * sizeof(uint32_t), indexOffset * sizeof(uint32_t));
					indexOffset += 3;
				}
			}
		}

		auto vertexBuffer = VertexBuffer::Create(layout, vertices);
		auto indexBuffer = IndexBuffer::Create(indices);

		vertices.Release();
		indices.Release();

		auto mesh = Ref<Mesh>::Create();
		mesh->m_VertexBuffer = vertexBuffer;
		mesh->m_IndexBuffer = indexBuffer;
		mesh->m_MaterialTable = materialTable;
		mesh->m_SubMeshes = std::move(submeshes);

		AddNode(&mesh->m_RootNode, scene->mRootNode);

		return mesh;
	}

	void AssimpImporter::AddNode(Mesh::Node* meshNode, aiNode* node)
	{
		SK_CORE_ASSERT(!(node->mNumMeshes > 1));

		meshNode->Transform = utils::AssimpMatrixToGLM(node->mTransformation);
		meshNode->Name = node->mName.C_Str();
		if (node->mNumMeshes == 1)
		{
			meshNode->HasMesh = true;
			meshNode->MeshIndex = node->mMeshes[0];
		}

		for (uint32_t childIndex = 0; childIndex < node->mNumChildren; childIndex++)
		{
			auto& child = meshNode->Children.emplace_back();
			child.Parent = meshNode;
			AddNode(&child, node->mChildren[childIndex]);
		}

	}

}
