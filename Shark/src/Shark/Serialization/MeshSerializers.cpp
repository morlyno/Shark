#include "skpch.h"
#include "MeshSerializers.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Animation/Graph/AnimationGraphAsset.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"
#include "Shark/Serialization/Import/AssimpMeshImporter.h"
#include "Shark/Serialization/SerializerUtilities.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/File/Serialization/FileStream.h"
#include "Shark/File/Serialization/StringStream.h"

namespace Shark {

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

		const bool success = FileSystem::WriteString(Utilities::GetAssetFilesystemPath(metadata), result);
		return success;
	}

	bool MeshSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		const auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!Utilities::ValidateYamlAssetFile(filesystemPath, metadata, context))
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

		stream.WriteToDisc(Utilities::GetAssetFilesystemPath(metadata));
		return true;
	}

	bool AnimationSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!Utilities::ValidateYamlAssetFile(filesystemPath, metadata, context))
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
		YAML::DeserializeProperty(animationNode, "Name", animation->m_Name);
		return true;
	}

	bool AnimationGraphSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		StringStreamWriter stream;

		if (!SerializeToYAML(asset.As<AnimationGraphAsset>(), &stream))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to serialize YAML!");
			return false;
		}

		stream.WriteToDisc(Utilities::GetAssetFilesystemPath(metadata));
		return true;
	}

	bool AnimationGraphSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!Utilities::ValidateYamlAssetFile(filesystemPath, metadata, context))
		{
			context->SetErrorFallback(Ref<AnimationGraphAsset>::Create());
			return false;
		}

		Ref<AnimationGraphAsset> animationGraph;
		FileStreamReader stream(filesystemPath);

		if (!DeserializeFromYAML(animationGraph, &stream, context))
			return false;

		asset = animationGraph;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool AnimationGraphSerializer::SerializeToYAML(Ref<AnimationGraphAsset> animationGraph, StreamWriter* stream)
	{
		YAML::Emitter out(stream->GetStream());

		out << YAML::BeginMap;
		out << YAML::Key << "AnimationGraph";
		out << YAML::BeginMap;
		SerializeGraphToYAML(animationGraph, out);
		out << YAML::EndMap;
		return true;
	}

	bool AnimationGraphSerializer::DeserializeFromYAML(Ref<AnimationGraphAsset>& animationGraph, StreamReader* stream, AssetLoadContext* context)
	{
		YAML::Node rootNode = YAML::Load(stream->GetStream());
		if (!rootNode["AnimationGraph"])
		{
			context->AddError(AssetLoadError::InvalidYAML, "Root node 'AnimationGraph' is missing");
			context->SetErrorFallback(Ref<AnimationGraphAsset>::Create());
			return false;
		}

		animationGraph = Ref<AnimationGraphAsset>::Create();
		return DeserializeGraphFromYAML(animationGraph, rootNode["AnimationGraph"], context);
	}

	bool AnimationGraphSerializer::SerializeGraphToYAML(Ref<AnimationGraphAsset> animationGraph, YAML::Emitter& emitter)
	{
		emitter << YAML::Key << "SkeletonMesh" << YAML::Value << animationGraph->GetSkeletonMesh();
		return true;
	}

	bool AnimationGraphSerializer::DeserializeGraphFromYAML(Ref<AnimationGraphAsset> animationGraph, const YAML::Node& animationGraphNode, AssetLoadContext* context)
	{
		AssetHandle skeletonMesh;
		YAML::DeserializeProperty(animationGraphNode, "SkeletonMesh", skeletonMesh);
		animationGraph->SetSkeletonMesh(skeletonMesh);

		return true;
	}

}
