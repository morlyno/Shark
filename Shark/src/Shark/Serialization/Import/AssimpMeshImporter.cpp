#include "skpch.h"
#include "AssimpMeshImporter.h"

#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/Import/TextureImporter.h"
#include "Shark/Debug/Profiler.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

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

	static unsigned int s_AIProcessFlags =
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_GenNormals |
		aiProcess_GenUVCoords |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ValidateDataStructure |
		aiProcess_GlobalScale |
		aiProcess_ConvertToLeftHanded;

	AssimpMeshImporter::AssimpMeshImporter(const std::filesystem::path& filepath)
		: m_Filepath(filepath), m_Extension(filepath.extension().string())
	{

	}

	Ref<MeshSource> AssimpMeshImporter::ToMeshSourceFromFile()
	{
		Ref<MeshSource> meshSource = MeshSource::Create();

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(m_Filepath.string(), s_AIProcessFlags);
		if (!scene)
		{
			std::string errorMsg = importer.GetErrorString();
			SK_CORE_ERROR("Failed to load mesh file: {}\n\tError: {}", m_Filepath, errorMsg);
			return nullptr;
		}

		if (scene->HasMeshes())
		{
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;

			meshSource->m_Submeshes.reserve(scene->mNumMeshes);
			for (uint32_t m = 0; m < scene->mNumMeshes; m++)
			{
				aiMesh* mesh = scene->mMeshes[m];

				Submesh& submesh = meshSource->m_Submeshes.emplace_back();
				submesh.BaseVertex = vertexCount;
				submesh.BaseIndex = indexCount;
				submesh.MaterialIndex = mesh->mMaterialIndex;
				submesh.VertexCount = mesh->mNumVertices;
				submesh.IndexCount = mesh->mNumFaces * 3;
				submesh.MeshName = mesh->mName.C_Str();

				vertexCount += mesh->mNumVertices;
				indexCount += mesh->mNumFaces * 3;

				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
				{
					Vertex vertex;
					vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
					vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
					vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
					vertex.Bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };

					if (mesh->HasTextureCoords(0))
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

					meshSource->m_Vertices.push_back(vertex);
				}

				for (uint32_t i = 0; i < mesh->mNumFaces; i++)
				{
					SK_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices");
					Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
					meshSource->m_Indices.push_back(index);
				}
			}

			meshSource->m_VertexBuffer = VertexBuffer::Create(Buffer::FromArray(meshSource->m_Vertices));
			meshSource->m_IndexBuffer = IndexBuffer::Create(Buffer::FromArray(meshSource->m_Indices));

			MeshNode& rootNode = meshSource->m_Nodes.emplace_back();
			TraverseNodes(meshSource, scene->mRootNode, 0);
		}

		if (scene->HasMaterials())
		{
			Ref<MaterialAsset> materialAsset = Ref<MaterialAsset>::Create();
			meshSource->m_Materials.reserve(scene->mNumMaterials);
			for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			{
				auto aiMaterial = scene->mMaterials[i];
				Ref<Material> material = Material::Create(Renderer::GetShaderLibrary()->Get("SharkPBR"), aiMaterial->GetName().C_Str());
				meshSource->m_Materials.push_back(material);
				materialAsset->SetMaterial(material);
				materialAsset->SetDefault();

				glm::vec3 albedoColor(0.8f);
				float emission = 0.0f;
				aiColor3D aiColor, aiEmission;
				if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == aiReturn_SUCCESS)
					albedoColor = { aiColor.r, aiColor.g, aiColor.b };

				if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == aiReturn_SUCCESS)
					emission = aiEmission.r;

				float roughness, metalness;
				if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
					roughness = 0.5f;

				if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
					metalness = 0.0f;

				material->Set("u_MaterialUniforms.Albedo", albedoColor);
				material->Set("u_MaterialUniforms.Metalness", metalness);
				material->Set("u_MaterialUniforms.Roughness", roughness);
				material->Set("u_MaterialUniforms.AmbientOcclusion", 0.0f);

				aiString aiTexPath;
				if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath);
					if (textureHandle != AssetHandle::Invalid)
					{
						Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
						material->Set("u_AlbedoMap", texture);
						material->Set("u_MaterialUniforms.Albedo", glm::vec3(1.0f));
					}
				}

				if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath);
					if (textureHandle != AssetHandle::Invalid)
					{
						Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
						material->Set("u_NormalMap", texture);
						material->Set("u_MaterialUniforms.UsingNormalMap", true);
					}
				}

				if (aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath);
					if (textureHandle != AssetHandle::Invalid)
					{
						Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
						material->Set("u_MetalnessMap", texture);
						material->Set("u_MaterialUniforms.Metalness", 1.0f);
					}
				}
				
				if (aiMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath);
					if (textureHandle != AssetHandle::Invalid)
					{
						Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
						material->Set("u_RoughnessMap", texture);
						material->Set("u_MaterialUniforms.Roughness", 1.0f);
					}
				}
			}
		}

		return meshSource;
	}

	void AssimpMeshImporter::TraverseNodes(Ref<MeshSource> meshSource, aiNode* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform, uint32_t level)
	{
		MeshNode& node = meshSource->m_Nodes[nodeIndex];
		node.Name = assimpNode->mName.C_Str();
		node.LocalTransform = utils::AssimpMatrixToGLM(assimpNode->mTransformation);
		node.Transform = parentTransform * node.LocalTransform;

		node.Submeshes.reserve(assimpNode->mNumMeshes);
		for (uint32_t i = 0; i < assimpNode->mNumMeshes; i++)
			node.Submeshes.emplace_back(assimpNode->mMeshes[i]);

		node.Children.reserve(assimpNode->mNumChildren);
		for (uint32_t i = 0; i < assimpNode->mNumChildren; i++)
		{
			MeshNode& parentNode = meshSource->m_Nodes[nodeIndex];
			parentNode.Children.emplace_back(meshSource->m_Nodes.size());

			MeshNode& child = meshSource->m_Nodes.emplace_back();
			child.Parent = nodeIndex;

			TraverseNodes(meshSource, assimpNode->mChildren[i], meshSource->m_Nodes.size() - 1, meshSource->m_Nodes[nodeIndex].Transform, level + 1);
		}
	}

	AssetHandle AssimpMeshImporter::LoadTexture(const aiScene* scene, const aiString& path)
	{
		TextureSpecification specification;
		specification.GenerateMips = true;
		specification.DebugName = path.C_Str();
		// TODO(moro): sampler

		if (auto aiTexEmbedded = scene->GetEmbeddedTexture(path.C_Str()))
		{
			specification.Format = ImageFormat::RGBA8;
			specification.Width = aiTexEmbedded->mWidth;
			specification.Height = aiTexEmbedded->mHeight;
			return AssetManager::CreateMemoryAsset<Texture2D>(specification, Buffer{ aiTexEmbedded->pcData, aiTexEmbedded->mWidth * aiTexEmbedded->mHeight * sizeof(aiTexel) });
		}

		const auto texturePath = m_Filepath.parent_path() / path.C_Str();
		Ref<TextureSource> textureSource = TextureImporter::ToTextureSourceFromFile(texturePath);
		if (textureSource)
		{
			return AssetManager::CreateMemoryAsset<Texture2D>(specification, textureSource);
		}

		SK_CORE_ERROR("Failed to load texture: {}", path.C_Str());
		return AssetHandle::Invalid;
	}

}
