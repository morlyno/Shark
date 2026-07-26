#include "skpch.h"
#include "MeshSerializers.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Asset/AssetManager/AssetUtilities.h"
#include "Shark/Render/Mesh.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"
#include "Shark/Serialization/Import/AssimpMeshImporter.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/File/Serialization/FileStream.h"
#include "Shark/File/Serialization/StringStream.h"

namespace Shark {

	namespace utils {

		static bool ValidateYamlAssetFile(const std::filesystem::path& filesystemPath, const AssetMetaData& metadata, AssetLoadContext* context, uint64_t threshold = 16 /*any file smaller than <threshold> bytes is invalid*/)
		{
			if (!FileSystem::Exists(filesystemPath))
			{
				context->OnFileNotFound(metadata);
				return false;
			}

			std::error_code error;
			const auto filesize = std::filesystem::file_size(filesystemPath, error);
			if (error || filesize < threshold)
			{
				context->OnFileEmpty(metadata);
				return false;
			}

			return true;
		}

	}

	///////////////////////////////////////////////////////////////////////////
	///// Mesh Source /////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	bool MeshSourceSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		return true;
	}

	bool MeshSourceSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		AssimpMeshImporter importer(filesystemPath);
		Ref<MeshSource> meshSource = importer.ToMeshSourceFromFile(context);

		if (!meshSource)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to Load MeshSource!");
			context->AddError(AssetLoadError::Unknown, "Assimp mesh importer failed to load file");
			return false;
		}

		asset = meshSource;
		asset->Handle = metadata.Handle;
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	///// Mesh ////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	bool MeshSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);

		std::string result = SerializeToYAML(asset.As<Mesh>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!");
			return false;
		}

		const bool success = FileSystem::WriteString(GetAssetFilesystemPath(metadata), result);
		return success;
	}

	bool MeshSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		const auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!utils::ValidateYamlAssetFile(filesystemPath, metadata, context))
			return false;

		std::string filedata = FileSystem::ReadString(filesystemPath);

		Ref<Mesh> mesh = nullptr;
		if (!DeserializeFromYAML(mesh, filedata, context))
		{
			context->OnYamlError(metadata);
			return false;
		}

		asset = mesh;
		asset->Handle = metadata.Handle;
		//context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

	std::string MeshSerializer::SerializeToYAML(Ref<Mesh> mesh)
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "MeshSource" << YAML::Value << mesh->m_MeshSource;
		out << YAML::Key << "Submeshes" << YAML::Value << mesh->m_Submeshes;
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool MeshSerializer::DeserializeFromYAML(Ref<Mesh>& mesh, const std::string& filedata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node in = YAML::Load(filedata);
		if (!in)
			return false;

		auto meshNode = in["Mesh"];
		if (!meshNode)
		{
			context->AddError(AssetLoadError::InvalidYAML, "Root node 'Mesh' missing");
			return false;
		}

		mesh = Ref<Mesh>::Create();
		SK_DESERIALIZE_PROPERTY(meshNode, "MeshSource", mesh->m_MeshSource);
		SK_DESERIALIZE_PROPERTY(meshNode, "Submeshes", mesh->m_Submeshes);

		context->AddTask([mesh = mesh](AssetLoadContext* context)
		{
			auto future = AssetManager::GetAssetFuture(mesh->GetMeshSource());
			if (!future.Valid())
			{
				context->AddError(AssetLoadError::Unknown, fmt::format("MeshSource '{}' missing!", mesh->GetMeshSource()));
				return;
			}

			future.OnReady([context, mesh](Ref<Asset> asset)
			{
				mesh->InitializeFromThis(asset.As<MeshSource>());
				context->SetStatus(AssetLoadStatus::Ready);
			});
		});

		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	///// Animation ///////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	bool AnimationSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		StringStreamWriter stream;

		if (!SerializeToYAML(asset.As<AnimationAsset>(), &stream))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to serialize YAML!");
			return false;
		}

		stream.WriteToDisc(GetAssetFilesystemPath(metadata));
		return true;
	}

	bool AnimationSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!utils::ValidateYamlAssetFile(filesystemPath, metadata, context))
		{
			context->SetErrorFallback(Ref<AnimationAsset>::Create());
			return false;
		}

		Ref<AnimationAsset> animation;
		FileStreamReader stream(filesystemPath);

		if (!DeserializeFromYAML(animation, &stream, context))
			return false;

		asset = animation;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool AnimationSerializer::SerializeToYAML(Ref<AnimationAsset> animation, StreamWriter* stream)
	{
		YAML::Emitter out(stream->GetStream());

		out << YAML::BeginMap;
		out << YAML::Key << "Animation";
		out << YAML::BeginMap;
		out << YAML::Key << "AnimationSource" << YAML::Value << animation->m_AnimationSource;
		out << YAML::Key << "SkeletonSource" << YAML::Value << animation->m_SkeletonSource;
		out << YAML::Key << "Name" << YAML::Value << animation->m_Name;
		out << YAML::EndMap;
		out << YAML::EndMap;
		return true;
	}

	bool AnimationSerializer::DeserializeFromYAML(Ref<AnimationAsset>& animation, StreamReader* stream, AssetLoadContext* context)
	{
		YAML::Node rootNode = YAML::Load(stream->GetStream());
		if (!rootNode["Animation"])
		{
			context->AddError(AssetLoadError::InvalidYAML, "Root node 'Animation' is missing");
			context->SetErrorFallback(Ref<AnimationAsset>::Create());
			return false;
		}

		auto animationNode = rootNode["Animation"];

		animation = Ref<AnimationAsset>::Create();
		YAML::DeserializeProperty(animationNode, "AnimationSource", animation->m_AnimationSource);
		YAML::DeserializeProperty(animationNode, "SkeletonSource", animation->m_SkeletonSource);
		YAML::DeserializeProperty(animationNode, "Name", animation->m_Name);
		return true;
	}

}
