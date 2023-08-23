#include "skpch.h"
#include "MeshSourceSerializer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Asset/ResourceManager.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace Shark {

	MeshSourceSerializerSettings g_MeshSourceSerializerSettings;

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

	bool MeshSourceSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing MeshSource to {}", metadata.FilePath);

		ScopedTimer timer("Serializing MeshSource");
		return true;
	}

	bool MeshSourceSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading MeshSource to {}", metadata.FilePath);

		ScopedTimer timer("Loading MeshSource");

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", metadata.FilePath);
			return false;
		}

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		if (!TryLoad(meshSource, ResourceManager::GetFileSystemPath(metadata)))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to Load MeshSource!");
			return false;
		}

		asset = meshSource;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool MeshSourceSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset)
		SK_CORE_INFO_TAG("Serialization", "Loading MeshSource to {}", assetPath);

		ScopedTimer timer("Loading MeshSource");

		if (!FileSystem::Exists(assetPath))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", assetPath);
			return false;
		}

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		if (!TryLoad(meshSource, FileSystem::GetAbsolute(assetPath)))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to Load MeshSource!");
			return false;
		}

		return true;
	}

	bool MeshSourceSerializer::TryLoad(Ref<MeshSource> meshSource, const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();
		if (!std::filesystem::exists(filepath))
			return false;

		Assimp::Importer importer;
		const aiScene* scene = nullptr;

		{
			SK_PROFILE_SCOPED("Assimp::Importer::ReadFile");
			scene = importer.ReadFile(filepath.string(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);
		}

		if (!scene)
			return false;

		LoadBuffersAndSubMeshes(scene, meshSource->m_VertexBuffer, meshSource->m_IndexBuffer, meshSource->m_SubMeshes);
		meshSource->m_MaterialTable = LoadMaterialTable(scene, filepath.parent_path());
		UpdateMaterials(meshSource->m_MaterialTable);
		AddNode(&meshSource->m_RootNode, scene->mRootNode);
		return true;
	}

	void MeshSourceSerializer::LoadBuffersAndSubMeshes(const aiScene* scene, Ref<VertexBuffer>& outVertexBuffer, Ref<IndexBuffer>& outIndexBuffer, std::vector<MeshSource::SubMesh>& outSubMeshes)
	{
		SK_PROFILE_FUNCTION();
		VertexLayout layout = {
			{ VertexDataType::Float3, "Position" },
			{ VertexDataType::Float3, "Normal" },
			{ VertexDataType::Float2, "UV" }
		};

		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec2 UV;
		};

		std::vector<Vertex> vertices;
		Buffer indices;
		auto& submeshes = outSubMeshes;

		{
			uint64_t vertexOffset = 0;
			uint64_t indexOffset = 0;

			for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
			{
				auto& submesh = submeshes.emplace_back();

				aiMesh* mesh = scene->mMeshes[meshIndex];
				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
				{
					auto& vertex = vertices.emplace_back();
					vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
					vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
					if (mesh->HasTextureCoords(0))
						vertex.UV = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
				}

				submesh.BaseVertex = vertexOffset;
				submesh.BaseIndex = indexOffset;
				submesh.IndexCount = mesh->mNumFaces * 3;
				submesh.MaterialIndex = mesh->mMaterialIndex;

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

		outVertexBuffer = VertexBuffer::Create(layout, Buffer::FromArray(vertices));
		outIndexBuffer = IndexBuffer::Create(indices);

		indices.Release();
	}

	Ref<MaterialTable> MeshSourceSerializer::LoadMaterialTable(const aiScene* scene, const std::filesystem::path& rootPath)
	{
		SK_PROFILE_FUNCTION();
		Ref<MaterialTable> materialTable = Ref<MaterialTable>::Create();

		for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++)
		{
			aiMaterial* material = scene->mMaterials[materialIndex];
			Ref<MaterialAsset> materialAsset = MaterialAsset::Create();
			materialTable->AddMaterial(materialIndex, materialAsset);

			aiString textureFileName;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName) == aiReturn_SUCCESS)
			{
				auto texturePath = rootPath / textureFileName.C_Str();
				if (FileSystem::Exists(texturePath))
				{
					AssetHandle sourceHandle = ResourceManager::GetAssetHandleFromFilePath(texturePath);
					if (!sourceHandle)
						sourceHandle = ResourceManager::ImportAsset(texturePath);

					if (ResourceManager::IsValidAssetHandle(sourceHandle))
					{
						Ref<TextureSource> source = ResourceManager::GetAsset<TextureSource>(sourceHandle);
						TextureSpecification specification;
						specification.GenerateMips = g_MeshSourceSerializerSettings.GenerateMips;
						specification.Sampler.Anisotropy = g_MeshSourceSerializerSettings.Anisotropy;
						specification.Sampler.MaxAnisotropy = g_MeshSourceSerializerSettings.MaxAnisotropy;
						Ref<Texture2D> texture = ResourceManager::CreateMemoryAsset<Texture2D>(specification, source);
						materialAsset->SetAlbedoTexture(texture->Handle);
						materialAsset->SetUseAlbedo(true);
					}
				}
			}

			aiVector3D diffuseColor;

			// TODO(moro): Test If Diffuse color can be removed. PBR Materials should have the color stored in Base Color
			if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == aiReturn_SUCCESS)
				materialAsset->SetAlbedoColor({ diffuseColor.x, diffuseColor.y, diffuseColor.z });

			if (material->Get(AI_MATKEY_BASE_COLOR, diffuseColor) == aiReturn_SUCCESS)
				materialAsset->SetAlbedoColor({ diffuseColor.x, diffuseColor.y, diffuseColor.z });

			float metallic;
			if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
				materialAsset->SetMetallic(metallic);

			float roughness;
			if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
				materialAsset->SetRoughness(roughness);

		}

		return materialTable;
	}

	void MeshSourceSerializer::UpdateMaterials(Ref<MaterialTable> materialTable)
	{
		SK_PROFILE_FUNCTION();
		for (const auto& [index, materialAsset] : *materialTable)
			materialAsset->UpdateMaterial();
	}

	void MeshSourceSerializer::AddNode(MeshSource::Node* meshNode, aiNode* node)
	{
		SK_PROFILE_FUNCTION();
		meshNode->Name = node->mName.C_Str();
		meshNode->Transform = utils::AssimpMatrixToGLM(node->mTransformation);
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
			meshNode->MeshIndices.emplace_back(node->mMeshes[0]);

		for (uint32_t childIndex = 0; childIndex < node->mNumChildren; childIndex++)
		{
			auto& child = meshNode->Children.emplace_back();
			child.Parent = meshNode;
			AddNode(&child, node->mChildren[childIndex]);
		}
	}

}
