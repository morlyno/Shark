#include "skpch.h"
#include "AssimpMeshImporter.h"

#include "Shark/Asset/AssetThread/AssetLoadContext.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Render/MaterialAsset.h"
#include "Shark/Render/Texture.h"

#include "Shark/Animation/Animation.h"
#include "Shark/Animation/Skeleton.h"

#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/Math/Math.h"
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

	Ref<MeshSource> AssimpMeshImporter::ToMeshSourceFromFile(AssetLoadContext* context)
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

				// #TODO avoid setDefaults=true
				auto pbrMaterial = PBRMaterial::Create(materialName.data, true, false);

				meshSource->m_Materials[i] = context->AddMemoryOnlyAsset(pbrMaterial);

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

				pbrMaterial->SetAlbedoColor(albedoColor);
				pbrMaterial->SetMetalness(metalness);
				pbrMaterial->SetRoughness(roughness);

				aiString aiTexPath;
				if (aiMaterial->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &aiTexPath) == aiReturn_SUCCESS ||
					aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, true, context);
					if (textureHandle != AssetHandle::Invalid)
					{
						pbrMaterial->SetAlbedoMap(textureHandle);
						pbrMaterial->SetAlbedoColor(glm::vec3(1.0f));
					}
				}

				if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, false, context);
					if (textureHandle != AssetHandle::Invalid)
					{
						pbrMaterial->SetNormalMap(textureHandle);
						pbrMaterial->SetUsingNormalMap(true);
					}
				}

				if (aiMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, false, context);
					if (textureHandle != AssetHandle::Invalid)
					{
						pbrMaterial->SetMetalnessMap(textureHandle);
						pbrMaterial->SetMetalness(1.0f);
					}
				}
				
				if (aiMaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &aiTexPath) == aiReturn_SUCCESS)
				{
					AssetHandle textureHandle = LoadTexture(scene, aiTexPath, false, context);
					if (textureHandle != AssetHandle::Invalid)
					{
						pbrMaterial->SetRoughnessMap(textureHandle);
						pbrMaterial->SetRoughness(1.0f);
					}
				}

				pbrMaterial->MT_Bake();
			}
		}

		meshSource->m_Skeleton = ImportSkeleton(scene);
		meshSource->m_AnimationNames.reserve(scene->mNumAnimations);
		meshSource->m_Animations.reserve(scene->mNumAnimations);

		if (meshSource->m_Skeleton)
		{
			meshSource->m_BoneInfluences.resize(meshSource->m_Vertices.size());
			for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
			{
				auto* mesh = scene->mMeshes[meshIndex];
				Submesh& submesh = meshSource->m_Submeshes[meshIndex];

				if (mesh->mNumBones == 0)
					continue;

				submesh.IsRigged = true;
				for (size_t aiBoneIndex = 0; aiBoneIndex < mesh->mNumBones; aiBoneIndex++)
				{
					auto* bone = mesh->mBones[aiBoneIndex];
					const size_t boneIndex = meshSource->m_Skeleton->GetBoneIndex(bone->mName.C_Str());
					if (boneIndex == Skeleton::NullIndex)
						continue;

					size_t boneInfoIndex = Skeleton::NullIndex;
					for (size_t i = 0; i < meshSource->m_BoneInfos.size(); i++)
					{
						if (meshSource->m_BoneInfos[i].BoneIndex != boneIndex)
							continue;

						boneInfoIndex = i;
						break;
					}

					if (boneInfoIndex == Skeleton::NullIndex)
					{
						boneInfoIndex = meshSource->m_BoneInfos.size();
						meshSource->m_BoneInfos.push_back({ utils::AssimpMatrixToGLM(bone->mOffsetMatrix), boneIndex });
					}

					for (size_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++)
					{
						int vertexID = submesh.BaseVertex + bone->mWeights[weightIndex].mVertexId;
						float weight = std::clamp(bone->mWeights[weightIndex].mWeight, 0.0f, 1.0f);
						if (weight <= 0.0f)
							continue;

						auto& boneInfluence = meshSource->m_BoneInfluences[vertexID];
						for (size_t i = 0; i < 4; i++)
						{
							if (boneInfluence.Weight[i] == 0.0f)
							{
								boneInfluence.BoneInfoIndices[i] = static_cast<uint32_t>(boneInfoIndex);
								SK_CORE_ASSERT(boneInfluence.BoneInfoIndices[i] < meshSource->GetBoneInfos().size());
								boneInfluence.Weight[i] = weight;
								break;
							}
						}
					}
				}
			}

			for (auto& influence : meshSource->m_BoneInfluences)
			{
				float weight = 0.0f;
				for (size_t i = 0; i < 4; i++)
					weight += influence.Weight[i];

				if (weight <= 0.0f)
					continue;

				for (size_t i = 0; i < 4; i++)
					influence.Weight[i] /= weight;
			}


		}

		for (size_t i = 0; i < scene->mNumAnimations; i++)
		{
			meshSource->m_AnimationNames.emplace_back(scene->mAnimations[i]->mName.C_Str());
			meshSource->m_Animations.emplace_back(ImportAnimation(scene, i, *meshSource->m_Skeleton));
		}

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices);

		if (!meshSource->m_BoneInfluences.empty())
			meshSource->m_BoneInfluenceBuffer = VertexBuffer::Create({ meshSource->m_BoneInfluences.data(), meshSource->m_BoneInfluences.size() * sizeof(BoneInfluence) });

		if (!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices);

		return meshSource;
	}

	Scope<Skeleton> AssimpMeshImporter::ImportSkeleton(const aiScene* scene)
	{
		std::set<std::string_view> bones;

		for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
		{
			auto* mesh = scene->mMeshes[meshIndex];
			auto numBones = mesh->mNumBones;
			for (size_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
			{
				bones.emplace(mesh->mBones[boneIndex]->mName.C_Str());
			}
		}

		if (bones.empty())
			return nullptr;

		auto skeleton = Scope<Skeleton>::Create();
		TraverseNodes(scene->mRootNode, skeleton.Raw(), bones);
		skeleton->Initialize();

		return skeleton;
	}

	static std::vector<Channel> ImportChannels(aiAnimation* animation, const Skeleton& skeleton)
	{
		std::vector<Channel> channels;

		std::unordered_map<std::string_view, size_t> boneIndices;
		std::unordered_set<size_t> rootBoneIndices;

		for (size_t i = 0; i < skeleton.GetBoneCount(); ++i)
		{
			boneIndices[skeleton.GetBoneName(i)] = i;
			if (skeleton.GetParentBoneIndex(i) == Skeleton::NullIndex)
				rootBoneIndices.emplace(i);
		}

		std::map<size_t, aiNodeAnim*> validChannels;
		for (auto i = 0; i < animation->mNumChannels; i++)
		{
			auto* nodeAnim = animation->mChannels[i];

			const auto it = boneIndices.find(nodeAnim->mNodeName.C_Str());
			if (it != boneIndices.end())
				validChannels.emplace(it->second, nodeAnim);
		}

		channels.resize(skeleton.GetBoneCount());

		double firstFrameDelta = std::numeric_limits<double>::max();
		double animationDuration = animation->mDuration;
		for (size_t i = 0; i < channels.size(); i++)
		{
			if (!validChannels.contains(i))
				continue;

			auto* nodeAnim = validChannels.at(i);
			if (nodeAnim->mNumPositionKeys > 0)
				firstFrameDelta = std::min(firstFrameDelta, nodeAnim->mPositionKeys[0].mTime);

			if (nodeAnim->mNumRotationKeys > 0)
				firstFrameDelta = std::min(firstFrameDelta, nodeAnim->mRotationKeys[0].mTime);

			if (nodeAnim->mNumScalingKeys > 0)
				firstFrameDelta = std::min(firstFrameDelta, nodeAnim->mScalingKeys[0].mTime);
		}

		animation->mDuration -= firstFrameDelta;
		if (animation->mDuration <= 0.0)
			animationDuration = 1.0f;

		for (size_t boneIndex = 0; boneIndex < channels.size(); boneIndex++)
		{
			Channel& channel = channels[boneIndex];
			channel.Index = boneIndex;

			if (!validChannels.contains(boneIndex))
			{
				const auto translation = skeleton.GetBoneTranslation(boneIndex);
				const auto rotation = skeleton.GetBoneRotation(boneIndex);
				const auto scale = skeleton.GetBoneScale(boneIndex);

				channel.Translations = { { 0.0f, translation }, { 1.0f, translation } };
				channel.Rotations    = { { 0.0f, rotation    }, { 1.0f, rotation    } };
				channel.Scales       = { { 0.0f, scale       }, { 1.0f, scale       } };
				continue;
			}

			auto* nodeAmin = validChannels.at(boneIndex);
			channel.Translations.reserve(nodeAmin->mNumPositionKeys + 2);
			channel.Rotations.reserve(nodeAmin->mNumRotationKeys+ 2);
			channel.Scales.reserve(nodeAmin->mNumScalingKeys + 2);

			/////////////////////////////////////////////////
			//// Position

			for (size_t keyIndex = 0; keyIndex < nodeAmin->mNumPositionKeys; keyIndex++)
			{
				auto key = nodeAmin->mPositionKeys[keyIndex];
				float frameTime = std::clamp(static_cast<float>((key.mTime - firstFrameDelta) / animation->mDuration), 0.0f, 1.0f);

				if (keyIndex == 0 && frameTime > 0.0f)
				{
					channel.Translations.push_back({
						0.0f,
						glm::vec3(
							static_cast<float>(key.mValue.x),
							static_cast<float>(key.mValue.y),
							static_cast<float>(key.mValue.z)
						)
					});
				}
				
				channel.Translations.push_back({
					frameTime,
					glm::vec3(
						static_cast<float>(key.mValue.x),
						static_cast<float>(key.mValue.y),
						static_cast<float>(key.mValue.z)
					)
				});
			}

			if (channel.Translations.empty())
			{
				channel.Translations = {
					{ 0.0f, glm::vec3(0.0f) },
					{ 1.0f, glm::vec3(0.0f) }
				};
			}
			else if (channel.Translations.back().FrameTime < 1.0f)
			{
				channel.Translations.push_back({
					1.0f,
					channel.Translations.back().Value
				});
			}

			/////////////////////////////////////////////////
			//// Rotation

			for (size_t keyIndex = 0; keyIndex < nodeAmin->mNumRotationKeys; keyIndex++)
			{
				auto key = nodeAmin->mRotationKeys[keyIndex];
				float frameTime = std::clamp(static_cast<float>((key.mTime - firstFrameDelta) / animation->mDuration), 0.0f, 1.0f);

				if (keyIndex == 0 && frameTime > 0.0f)
				{
					channel.Rotations.push_back({
						0.0f,
						glm::quat(
							static_cast<float>(key.mValue.w),
							static_cast<float>(key.mValue.x),
							static_cast<float>(key.mValue.y),
							static_cast<float>(key.mValue.z)
						)
					});
				}
				
				channel.Rotations.push_back({
					frameTime,
					glm::quat(
						static_cast<float>(key.mValue.w),
						static_cast<float>(key.mValue.x),
						static_cast<float>(key.mValue.y),
						static_cast<float>(key.mValue.z)
					)
				});
			}

			if (channel.Rotations.empty())
			{
				channel.Rotations = {
					{ 0.0f, glm::identity<glm::quat>() },
					{ 1.0f, glm::identity<glm::quat>() }
				};
			}
			else if (channel.Rotations.back().FrameTime < 1.0f)
			{
				channel.Rotations.push_back({
					1.0f,
					channel.Rotations.back().Value
				});
			}

			/////////////////////////////////////////////////
			//// Scale

			for (size_t keyIndex = 0; keyIndex < nodeAmin->mNumScalingKeys; keyIndex++)
			{
				auto key = nodeAmin->mScalingKeys[keyIndex];
				float frameTime = std::clamp(static_cast<float>((key.mTime - firstFrameDelta) / animation->mDuration), 0.0f, 1.0f);

				float scale = static_cast<float>(key.mValue.x);
				if (glm::epsilonNotEqual(key.mValue.x, key.mValue.y, 0.00001f) || glm::epsilonNotEqual(key.mValue.y, key.mValue.z, 0.00001f))
				{
					scale = static_cast<float>((key.mValue.x + key.mValue.y + key.mValue.y) / 3.0);
					SK_CORE_ERROR_TAG("Animation", "Non uniform scale! [{}, {}, {}] => {}", key.mValue.x, key.mValue.y, key.mValue.z, scale);
				}

				if (keyIndex == 0 && frameTime > 0.0f)
				{
					channel.Scales.push_back({
						0.0f,
						scale
					});
				}

				channel.Scales.push_back({
					frameTime,
					scale
				});
			}

			if (channel.Scales.empty())
			{
				channel.Scales = {
					{ 0.0f, 1.0f },
					{ 1.0f, 1.0f }
				};
			}
			else if (channel.Scales.back().FrameTime < 1.0f)
			{
				channel.Scales.push_back({
					1.0f,
					channel.Scales.back().Value
				});
			}
		}

		return channels;
	}

	void SanitizeChannels(std::vector<Channel>& channels)
	{
		size_t desiredFrames = 2;
		for (const auto& channel : channels)
			desiredFrames = std::max({ desiredFrames, channel.Translations.size(), channel.Rotations.size(), channel.Scales.size() });

		const float frameInterval = 1.0f / static_cast<float>(desiredFrames - 1);

		for (auto& channel : channels)
		{
			Channel newChannel;
			newChannel.Translations.reserve(desiredFrames);
			newChannel.Rotations.reserve(desiredFrames);
			newChannel.Scales.reserve(desiredFrames);

			newChannel.Translations.emplace_back(channel.Translations.front());
			newChannel.Rotations.emplace_back(channel.Rotations.front());
			newChannel.Scales.emplace_back(channel.Scales.front());

			size_t translationIndex = 0;
			size_t rotationIndex = 0;
			size_t scaleIndex = 0;

			for (size_t i = 1; i < desiredFrames - 1; i++)
			{
				const float frameTime = i * frameInterval;

				while (translationIndex < channel.Translations.size() && channel.Translations[translationIndex].FrameTime < frameTime)
					translationIndex++;

				while (rotationIndex < channel.Rotations.size() && channel.Rotations[rotationIndex].FrameTime < frameTime)
					rotationIndex++;

				while (scaleIndex < channel.Scales.size() && channel.Scales[scaleIndex].FrameTime < frameTime)
					scaleIndex++;

				const float translationT = (frameTime - channel.Translations[translationIndex - 1].FrameTime) / (channel.Translations[translationIndex].FrameTime - channel.Translations[translationIndex - 1].FrameTime);
				const float rotationT    = (frameTime - channel.Rotations   [rotationIndex    - 1].FrameTime) / (channel.Rotations   [rotationIndex   ].FrameTime - channel.Rotations   [rotationIndex    - 1].FrameTime);
				const float scaleT       = (frameTime - channel.Scales      [scaleIndex       - 1].FrameTime) / (channel.Scales      [scaleIndex      ].FrameTime - channel.Scales      [scaleIndex       - 1].FrameTime);

				newChannel.Translations.push_back({ frameTime, glm::mix(channel.Translations[translationIndex - 1].Value, channel.Translations[translationIndex].Value, translationT) });
				newChannel.Rotations   .push_back({ frameTime, glm::mix(channel.Rotations   [rotationIndex    - 1].Value, channel.Rotations   [rotationIndex   ].Value, rotationT)    });
				newChannel.Scales      .push_back({ frameTime, glm::mix(channel.Scales      [scaleIndex       - 1].Value, channel.Scales      [scaleIndex      ].Value, scaleT)       });
			}

			newChannel.Translations.push_back(channel.Translations.back());
			newChannel.Rotations.push_back(channel.Rotations.back());
			newChannel.Scales.push_back(channel.Scales.back());

			channel = std::move(newChannel);
		}

	}

	Scope<Animation> AssimpMeshImporter::ImportAnimation(const aiScene* scene, uint32_t animationIndex, const Skeleton& skeleton)
	{
		if (!scene || animationIndex >= scene->mNumAnimations)
			return nullptr;

		aiAnimation* animation = scene->mAnimations[animationIndex];
		auto channels = ImportChannels(animation, skeleton);

		SanitizeChannels(channels);

		auto samplingRate = animation->mTicksPerSecond;
		if (samplingRate < 0.0001)
			samplingRate = 1.0;

		return Scope<Animation>::Create(&skeleton, std::move(channels), static_cast<float>(animation->mDuration / samplingRate));
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
			parentNode.Children.emplace_back(static_cast<uint32_t>(meshSource->m_Nodes.size()));

			MeshNode& child = meshSource->m_Nodes.emplace_back();
			child.Parent = nodeIndex;

			TraverseNodes(meshSource, assimpNode->mChildren[i], meshSource->m_Nodes.size() - 1, meshSource->m_Nodes[nodeIndex].Transform, level + 1);
		}
	}

	AssetHandle AssimpMeshImporter::LoadTexture(const aiScene* scene, const aiString& path, bool sRGB, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		TextureSpecification specification;
		specification.DebugName = path.C_Str();
		specification.Format = sRGB ? ImageFormat::sRGBA : ImageFormat::RGBA;
		// TODO(moro): sampler

		if (auto aiTexEmbedded = scene->GetEmbeddedTexture(path.C_Str()))
		{
			specification.DebugName = aiTexEmbedded->mFilename.C_Str();
			specification.Width = aiTexEmbedded->mWidth;
			specification.Height = aiTexEmbedded->mHeight;
			MutableBuffer imageData = MutableBuffer{ aiTexEmbedded->pcData, aiTexEmbedded->mWidth * aiTexEmbedded->mHeight * sizeof(aiTexel) };
			if (aiTexEmbedded->mHeight == 0)
			{
				imageData = TextureImporter::ToBufferFromMemory(Buffer(aiTexEmbedded->pcData, aiTexEmbedded->mWidth), specification.Format, specification.Width, specification.Height).ExtractBuffer();
			}

			Ref<Texture2D> texture = Texture2D::Create(specification, imageData);
			Renderer::MT::GenerateMips(texture->GetImage());

			if (aiTexEmbedded->mHeight == 0)
				imageData.Release();

			return context->AddMemoryOnlyAsset(texture);
		}

		// #TODO #async find a way to do this through the asset system
		const auto texturePath = m_Filepath.parent_path() / path.C_Str();
		UniqueBuffer imageData = TextureImporter::ToBufferFromFile(texturePath, specification.Format, specification.Width, specification.Height);
		if (!imageData)
		{
			// #TODO handle file not found
			return AssetHandle::Invalid;
		}

		Ref<Texture2D> texture = Texture2D::Create(specification, imageData);
		Renderer::MT::GenerateMips(texture->GetImage());

		return context->AddMemoryOnlyAsset(texture);
	}

	static bool NodeContainsBone(aiNode* node, std::set<std::string_view>& bones)
	{
		if (!node)
			return false;

		if (bones.contains(node->mName.C_Str()))
			return true;

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			if (NodeContainsBone(node->mChildren[i], bones))
			{
				return true;
			}
		}
		return false;
	}

	static void TraverseBone(aiNode* node, Skeleton* skeleton, size_t parentIndex, std::set<std::string_view>& bones)
	{
		glm::vec3 translation, scale;
		glm::quat rotation;

		{
			glm::mat4 transform = utils::AssimpMatrixToGLM(node->mTransformation);
			Math::DecomposeTransform(transform, translation, rotation, scale);
		}

		if (glm::epsilonNotEqual(scale.x, scale.y, 0.00001f) || glm::epsilonNotEqual(scale.y, scale.z, 0.00001f))
		{
			SK_CORE_ERROR_TAG("Animation", "Non uniform scale! {} => {}", scale, (scale.x + scale.y + scale.y) / 3.0f);
			scale.x = (scale.x + scale.y + scale.y) / 3.0f;
		}

		const size_t boneIndex = skeleton->AddBone(node->mName.C_Str(), parentIndex, translation, rotation, scale.x);
		for (auto i = 0; i < node->mNumChildren; i++)
		{
			auto* childNode = node->mChildren[i];
			if (NodeContainsBone(childNode, bones))
				TraverseBone(childNode, skeleton, boneIndex, bones);
		}
	}

	void AssimpMeshImporter::TraverseNodes(aiNode* node, Skeleton* skeleton, std::set<std::string_view>& bones)
	{
		uint32_t boneChildCount = 0;
		for (auto i = 0; i < node->mNumChildren; i++)
		{
			if (NodeContainsBone(node, bones))
				boneChildCount++;

			if (boneChildCount > 1)
				break;
		}

		if (bones.contains(node->mName.C_Str()) || boneChildCount > 1)
		{
			TraverseBone(node, skeleton, ~0, bones);
			return;
		}

		for (auto i = 0; i < node->mNumChildren; i++)
			TraverseNodes(node->mChildren[i], skeleton, bones);
	}

}
