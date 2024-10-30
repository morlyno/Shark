#include "skpch.h"
#include "AssimpMeshImporter.h"

#include "Shark/Asset/AssetManager.h"

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

#if 0
#define SK_MESH_LOG(...) SK_CORE_TRACE_TAG("Mesh", __VA_ARGS__)
#else
#define SK_MESH_LOG(...) (void)0
#endif

	class AssimpLogStream : public Assimp::LogStream
	{
	public:
		virtual void write(const char* message) override
		{
			std::string_view msg = message;
			if (msg.ends_with('\n'))
				msg.remove_suffix(1);

			if (msg.starts_with("Debug"))
				SK_CORE_TRACE_TAG("Assimp", msg);
			else if (msg.starts_with("Info"))
				SK_CORE_INFO_TAG("Assimp", msg);
			else if (msg.starts_with("Warn"))
				SK_CORE_WARN_TAG("Assimp", msg);
			else
				SK_CORE_ERROR_TAG("Assimp", msg);
		}

	};

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

		aiProcess_FindInstances |
		aiProcess_OptimizeMeshes |
		aiProcess_SortByPType |

		aiProcess_ConvertToLeftHanded;

	AssimpMeshImporter::AssimpMeshImporter(const std::filesystem::path& filepath)
		: m_Filepath(filepath), m_Extension(filepath.extension().string())
	{
		if (Assimp::DefaultLogger::isNullLogger())
		{
			auto logger = Assimp::DefaultLogger::create("AssimpImporter", Assimp::Logger::VERBOSE, 0);
			logger->attachStream(new Shark::AssimpLogStream(), Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Warn | Assimp::Logger::Err);
		}
	}

	Ref<MeshSource> AssimpMeshImporter::ToMeshSourceFromFile()
	{
		SK_PROFILE_FUNCTION();
		Ref<MeshSource> meshSource = MeshSource::Create();

		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(m_Filepath.string(), s_AIProcessFlags);
		if (!scene)
		{
			std::string errorMsg = importer.GetErrorString();
			SK_CORE_ERROR_TAG("Assimp", "Failed to load mesh file: {}\n\tError: {}", m_Filepath, errorMsg);
			return nullptr;
		}

		meshSource->m_Name = scene->mName.C_Str();

		if (scene->HasMeshes())
		{
			SK_PROFILE_SCOPED("AssimpMeshImporter::ToMeshSourceFromFile [Load Meshes]");
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;

			meshSource->m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			meshSource->m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

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
				//submesh.BoundingBox.Min = { mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z };
				//submesh.BoundingBox.Max = { mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z };

				//meshSource->m_BoundingBox.Min = glm::min(meshSource->m_BoundingBox.Min, submesh.BoundingBox.Min);
				//meshSource->m_BoundingBox.Max = glm::max(meshSource->m_BoundingBox.Max, submesh.BoundingBox.Max);

				vertexCount += mesh->mNumVertices;
				indexCount += mesh->mNumFaces * 3;

				auto& aabb = submesh.BoundingBox;
				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
				{
					Vertex vertex = {};
					vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
					vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

					aabb.Min = glm::min(vertex.Position, aabb.Min);
					aabb.Max = glm::max(vertex.Position, aabb.Max);

					if (mesh->HasTangentsAndBitangents())
					{
						vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
						vertex.Bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
					}

					if (mesh->HasTextureCoords(0))
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

					meshSource->m_Vertices.push_back(vertex);
				}

				meshSource->m_BoundingBox.Min = glm::min(meshSource->m_BoundingBox.Min, submesh.BoundingBox.Min);
				meshSource->m_BoundingBox.Max = glm::max(meshSource->m_BoundingBox.Max, submesh.BoundingBox.Max);

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
			SK_MESH_LOG("----- Materials {} -----", m_Filepath);

			SK_PROFILE_SCOPED("AssimpMeshImporter::ToMeshSourceFromFile [Load Materials]");
			meshSource->m_Materials.resize(scene->mNumMaterials);
			for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			{
				auto aiMaterial = scene->mMaterials[i];
				auto materialName = aiMaterial->GetName();
				auto material = Material::Create(Renderer::GetShaderLibrary()->Get("SharkPBR"), materialName.data);
				auto materialAsset = Ref<MaterialAsset>::Create(material);
				meshSource->m_Materials[i] = AssetManager::AddMemoryAsset(materialAsset);

				SK_MESH_LOG("  {} (Index = {})", materialName.data, i);

#if 0
				for (uint32_t p = 0; p < aiMaterial->mNumProperties; p++)
				{
					auto prop = aiMaterial->mProperties[p];

					SK_MESH_LOG("Material Property:");
					SK_MESH_LOG("  Name = {0}", prop->mKey.data);
					SK_MESH_LOG("  Type = {0}", prop->mType);
					SK_MESH_LOG("  Size = {0}", prop->mDataLength);
					switch (prop->mType)
					{
						case aiPTI_Float:
							SK_MESH_LOG("  Value = {0}", *(float*)prop->mData);
							break;
						case aiPTI_Double:
							SK_MESH_LOG("  Value = {0}", *(double*)prop->mData);
							break;
						case aiPTI_String:
							SK_MESH_LOG("  Value = {0}", std::string_view(prop->mData, prop->mDataLength));
							break;
						case aiPTI_Integer:
							if (prop->mDataLength == 1)
								SK_MESH_LOG("  Value = {0}", *(uint8_t*)prop->mData);
							else if (prop->mDataLength == 2)
								SK_MESH_LOG("  Value = {0}", *(uint16_t*)prop->mData);
							else if (prop->mDataLength == 4)
								SK_MESH_LOG("  Value = {0}", *(uint32_t*)prop->mData);
							else if (prop->mDataLength == 8)
								SK_MESH_LOG("  Value = {0}", *(uint64_t*)prop->mData);
							else
								SK_CORE_VERIFY(false);
							break;
						case aiPTI_Buffer:
						{
							std::vector<byte> buffer;
							buffer.resize(prop->mDataLength);
							memcpy(buffer.data(), prop->mData, prop->mDataLength);
							SK_MESH_LOG("  Value = {0}", buffer);
							break;
						}
					}
					float data = *(float*)prop->mData;
					SK_MESH_LOG("  Semantic = {}", (aiTextureType)prop->mSemantic);
				}
#endif


				glm::vec3 albedoColor(0.8f);
				float emission = 0.0f;
				float roughness, metalness;

				aiColor3D aiColor, aiEmission;
				if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == aiReturn_SUCCESS)
					albedoColor = { aiColor.r, aiColor.g, aiColor.b };

				if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == aiReturn_SUCCESS)
					emission = aiEmission.r;

				if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
					roughness = 0.5f;

				// AI_MATKEY_METALLIC_FACTOR
				if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
					metalness = 0.0f;

				materialAsset->SetAlbedoColor(albedoColor);
				materialAsset->SetMetalness(metalness);
				materialAsset->SetRoughness(roughness);

				aiString aiTexPath;
				if (aiMaterial->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &aiTexPath) == aiReturn_SUCCESS ||
					aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, true);
					if (textureHandle != AssetHandle::Invalid)
					{
						materialAsset->SetAlbedoMap(textureHandle);
						materialAsset->SetAlbedoColor(glm::vec3(1.0f));
					}
				}

				if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, false);
					if (textureHandle != AssetHandle::Invalid)
					{
						materialAsset->SetNormalMap(textureHandle);
						materialAsset->SetUsingNormalMap(true);
					}
				}

				if (aiMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, false);
					if (textureHandle != AssetHandle::Invalid)
					{
						materialAsset->SetMetalnessMap(textureHandle);
						materialAsset->SetMetalness(1.0f);
					}
				}
				
				if (aiMaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, false);
					if (textureHandle != AssetHandle::Invalid)
					{
						materialAsset->SetRoughnessMap(textureHandle);
						materialAsset->SetRoughness(1.0f);
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

	AssetHandle AssimpMeshImporter::LoadTexture(const aiScene* scene, const aiString& path, bool sRGB)
	{
		SK_PROFILE_FUNCTION();
		TextureSpecification specification;
		specification.DebugName = path.C_Str();
		specification.Format = sRGB ? ImageFormat::sRGBA : ImageFormat::RGBA8UNorm;
		// TODO(moro): sampler

		if (auto aiTexEmbedded = scene->GetEmbeddedTexture(path.C_Str()))
		{
			specification.Width = aiTexEmbedded->mWidth;
			specification.Height = aiTexEmbedded->mHeight;
			Buffer imageData = Buffer{ aiTexEmbedded->pcData, aiTexEmbedded->mWidth * aiTexEmbedded->mHeight * sizeof(aiTexel) };
			if (specification.Height == 0)
			{
				imageData = TextureImporter::ToBufferFromMemory(Buffer(aiTexEmbedded->pcData, aiTexEmbedded->mWidth), specification.Format, specification.Width, specification.Height);
			}
			// TODO(moro): race-condition! This line can be called from both the main thread and the asset thread!
			AssetHandle handle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(specification, imageData);

			if (specification.Height == 0)
				imageData.Release();

			return handle;
		}

		if (sRGB)
		{
			// TODO(moro): find a way to do this through the asset system
			const auto texturePath = m_Filepath.parent_path() / path.C_Str();
			ScopedBuffer imageData = TextureImporter::ToBufferFromFile(texturePath, specification.Format, specification.Width, specification.Height);
			return AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(specification, imageData);
		}

		const auto texturePath = m_Filepath.parent_path() / path.C_Str();
		return Project::GetActiveEditorAssetManager()->GetAssetHandleFromFilepath(texturePath);
	}

}
